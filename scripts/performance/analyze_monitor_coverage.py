#!/usr/bin/env python3
"""Analyze Windows PDH CSV sampling and exact Tonatiuh++ subprocess windows.

Standard library only. Scientific/benchmark timing is unaffected.  Samples are
point observations, not time-weighted telemetry.  Never infer thermal throttle.
"""
import argparse
import csv
from datetime import datetime, timezone, timedelta
import json
from pathlib import Path
import re
import statistics


def parse_pdh_date(text, tz):
    value = text.strip().strip('"')
    for fmt in ('%m/%d/%Y %H:%M:%S.%f', '%m/%d/%Y %H:%M:%S',
                '%d/%m/%Y %H:%M:%S.%f', '%d/%m/%Y %H:%M:%S'):
        try:
            return datetime.strptime(value, fmt).replace(tzinfo=tz).astimezone(timezone.utc)
        except ValueError:
            continue
    raise ValueError('Unsupported Windows PDH timestamp: ' + value)


def load_samples(paths):
    result = []
    headers = None
    for path in sorted(paths):
        with path.open('r', encoding='utf-8-sig', newline='', errors='replace') as stream:
            rows = csv.reader(stream)
            names = next(rows)
            if not names or not names[0].startswith('(PDH-CSV'):
                raise ValueError('Not a PDH CSV file: ' + str(path))
            # PDH header convention: (-60) represents 60 minutes west of UTC,
            # i.e. UTC+01:00.  Preserve raw zone information for audit.
            match = re.search(r'\((-?\d+)\)$', names[0])
            if not match:
                raise ValueError('PDH header lacks UTC offset: ' + names[0])
            tz = timezone(timedelta(minutes=-int(match.group(1))))
            if headers is None:
                headers = names
            elif names != headers:
                raise ValueError('PDH CSV segment headers differ')
            for row in rows:
                if not row or not row[0].strip():
                    continue
                try:
                    stamp = parse_pdh_date(row[0], tz)
                except ValueError:
                    continue
                result.append((stamp, row))
    result.sort(key=lambda item: item[0])
    unique = {}
    for stamp, row in result:
        unique[stamp] = row
    return headers, sorted(unique.items())


def load_windows(path):
    starts = {}
    ends = {}
    for ln, line in enumerate(path.read_text(encoding='utf-8-sig').splitlines(), 1):
        if not line.strip():
            continue
        event = json.loads(line)
        key = (int(event['index']), event['version'])
        t = datetime.fromisoformat(event['utc'].replace('Z', '+00:00')).astimezone(timezone.utc)
        if event['event'] == 'process_start':
            if key in starts:
                raise ValueError('Duplicate start: ' + str(key))
            starts[key] = (t, event)
        elif event['event'] == 'process_end':
            if key in ends:
                raise ValueError('Duplicate end: ' + str(key))
            ends[key] = t
    output = []
    for key in sorted(set(starts) | set(ends)):
        if key not in starts or key not in ends:
            raise ValueError('Unpaired event: ' + str(key))
        start, event = starts[key]
        end = ends[key]
        if end <= start:
            raise ValueError('Nonpositive run duration: ' + str(key))
        output.append({'run': key[0], 'version': key[1], 'warmup': bool(event['warmup']),
                       'block': int(event['block']), 'start_utc': start.isoformat(),
                       'end_utc': end.isoformat(), 'duration_seconds': (end-start).total_seconds()})
    if not output:
        raise ValueError('No paired run events')
    return output


def metrics(headers, rows):
    chosen = {}
    for k, header in enumerate(headers):
        if re.search(r'\\Processor Information\(_Total\)\\', header, flags=re.I):
            for suffix, key in (('\\% Processor Utility', 'utility_pct'),
                                ('\\% Processor Performance', 'performance_pct'),
                                ('\\Processor Frequency', 'frequency_mhz'),
                                ('\\% of Maximum Frequency', 'maximum_frequency_pct')):
                if header.lower().endswith(suffix.lower()):
                    chosen[key] = k
        if re.search(r'\\System\\Processor Queue Length$', header, re.I):
            chosen['queue_length'] = k
    out = {}
    for name, index in chosen.items():
        values = []
        for _, row in rows:
            if len(row) <= index:
                continue
            try:
                values.append(float(row[index].strip().replace(',', '.')))
            except ValueError:
                continue
        if values:
            out[name + '_sample_mean'] = round(statistics.mean(values), 3)
    return out


def analyze(csv_files, events_path):
    headers, samples = load_samples(csv_files)
    windows = load_windows(events_path)
    stamps = [t for t, _ in samples]
    intervals = [(right-left).total_seconds() for left, right in zip(stamps, stamps[1:])]
    report = {'schema': 'tonatiuhpp.monitor-coverage.v1',
              'counter_files': [str(p) for p in csv_files],
              'events_file': str(events_path),
              'sample_count': len(samples),
              'median_sample_interval_seconds': round(statistics.median(intervals), 3) if intervals else None,
              'maximum_sample_interval_seconds': round(max(intervals), 3) if intervals else None,
              'intervals_above_3_seconds': sum(i > 3 for i in intervals),
              'run_coverage': []}
    for run in windows:
        begin = datetime.fromisoformat(run['start_utc'])
        end = datetime.fromisoformat(run['end_utc'])
        inside = [(t, row) for t, row in samples if begin <= t <= end]
        inside_stamps = [t for t, _ in inside]
        inside_intervals = [(b-a).total_seconds() for a,b in zip(inside_stamps,inside_stamps[1:])]
        n = len(inside)
        duration = run['duration_seconds']
        entry = {**run, 'samples': n,
                 'sample_density_per_second': round(n / duration, 3),
                 'max_interior_sample_gap_seconds': round(max(inside_intervals),3) if inside_intervals else None,
                 'sparse': n < max(5, int(duration * 0.70)),
                 **metrics(headers, inside)}
        report['run_coverage'].append(entry)
    return report


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--monitor-dir', type=Path, required=True)
    p.add_argument('--comparison-dir', type=Path, required=True)
    args = p.parse_args()
    csv_files = sorted(args.monitor_dir.glob('hardware-counters*.csv'))
    if not csv_files:
        raise SystemExit('No hardware-counters*.csv file in monitoring directory')
    event_path = args.comparison_dir / 'run-events.jsonl'
    if not event_path.exists():
        raise SystemExit('No run-events.jsonl found: instrument the v2 harness first')
    report = analyze(csv_files, event_path)
    json_out = args.monitor_dir / 'sample-coverage.json'
    text_out = args.monitor_dir / 'sample-coverage.md'
    json_out.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    lines = ['# Windows counter sample coverage', '',
             'Exact run boundaries derive from Python immediately around subprocess.run; '
             'they include process startup/shutdown, not just native tracing.', '',
             'Sampling is irregular; means are **unweighted sample means** and are not '
             'independent proof of thermal throttling.', '',
             f"Samples: **{report['sample_count']}**; median interval: "
             f"**{report['median_sample_interval_seconds']} s**; maximum interval: "
             f"**{report['maximum_sample_interval_seconds']} s**; gaps >3 s: "
             f"**{report['intervals_above_3_seconds']}**.", '',
             '| Run | Version | Duration (s) | Samples | Density (/s) | Sparse? |',
             '|---|---|---:|---:|---:|---|']
    for item in report['run_coverage']:
        lines.append(f"| {item['run']:02d}{' (warm-up)' if item['warmup'] else ''} | "
                     f"{item['version']} | {item['duration_seconds']:.2f} | "
                     f"{item['samples']} | {item['sample_density_per_second']:.2f} | "
                     f"{'Yes' if item['sparse'] else 'No'} |")
    lines.append('')
    text_out.write_text('\n'.join(lines), encoding='utf-8')
    print('Coverage report:', text_out)
    print('JSON:', json_out)
    print('Sparse runs:', sum(x['sparse'] for x in report['run_coverage']), '/', len(report['run_coverage']))

if __name__ == '__main__':
    main()
