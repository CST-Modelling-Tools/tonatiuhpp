#!/usr/bin/env python3
"""Uncommitted Tonatiuh++ performance comparison. Python standard library only.

Run two already-built binaries in alternating ABBA order. Never opens a canonical
reference for writing; reduced-ray results are compared only against each other.
"""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import sys
import time
from datetime import datetime, timezone

SEED = 123456789
SCIENTIFIC_FIELDS = (
    'total_power_mw', 'minimum_flux_mw_m2', 'average_flux_mw_m2',
    'maximum_flux_mw_m2', 'flux_grid_sha256',
)


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def positive_int(value: str) -> int:
    number = int(value)
    if number <= 0:
        raise argparse.ArgumentTypeError('must be positive')
    return number


def read_json(path: Path) -> dict:
    with path.open('r', encoding='utf-8-sig') as f:
        obj = json.load(f)
    if not isinstance(obj, dict):
        raise ValueError(f'expected a JSON object: {path}')
    return obj


def process_command(exe: Path, legacy: bool, config_path: Path) -> list[str]:
    return [str(exe), *(['--headless'] if legacy else []), 'benchmark', str(config_path)]


def git_info(repo: Path) -> dict:
    def git(*args: str) -> str:
        result = subprocess.run(['git', '-C', str(repo), *args], capture_output=True, text=True, check=False)
        return result.stdout.strip() if result.returncode == 0 else 'unavailable'
    return {'head': git('rev-parse', 'HEAD'), 'status': git('status', '--short'), 'branch': git('branch', '--show-current')}


def guarded_output_root(root: Path) -> None:
    # Refuse reusing or deleting any path; no accidental overwrites of prior measurements.
    if root.exists():
        raise ValueError(f'Output directory already exists: {root}. Use a new -OutputDir.')
    root.mkdir(parents=True, exist_ok=False)


def build_config(scene: Path, rays: int, result_name: str) -> dict:
    # Intentionally excludes reference_file: the 500M reference is NOT a reference for reduced rays.
    return {
        'benchmark': 'benchmark_v1', 'scene_file': str(scene).replace('\\', '/'),
        'rays': rays, 'seed': SEED, 'target_side_id': 1,
        'target_bounds': {'x_min': -2.0, 'x_max': 2.0, 'y_min': -2.0, 'y_max': 2.0},
        'target_grid': {'width': 100, 'height': 100},
        'photon_export': False, 'output_file': result_name,
    }


def run_once(label: str, index: int, args: argparse.Namespace, root: Path, *, warmup: bool = False, block: int = 0) -> dict:
    is_baseline = label == 'baseline'
    exe = args.baseline_exe if is_baseline else args.candidate_exe
    legacy = args.baseline_legacy if is_baseline else args.candidate_legacy
    scene = args.baseline_scene if is_baseline else args.candidate_scene
    run_dir = root / f'{index:02d}_{label}'
    run_dir.mkdir()
    config_path = run_dir / 'config.json'
    result_path = run_dir / 'result.json'
    log_path = run_dir / 'console.log'
    config_path.write_text(json.dumps(build_config(scene, args.rays, result_path.name), indent=2) + '\n', encoding='utf-8')
    cmd = process_command(exe, legacy, config_path)
    print(f'[{index:02d}] {label}: rays={args.rays:,} exe={exe.name}', flush=True)
    # TONATIUHPP_EXACT_RUN_BOUNDARIES_V1
    # Observes the subprocess call boundaries (includes process startup and exit).
    # This does not change benchmark timing, seed, computation or report logic.
    events_path = root / 'run-events.jsonl'
    def record_event(event, **extra):
        payload = {'schema': 'tonatiuhpp.run-event.v1', 'event': event,
                   'index': index, 'version': label, 'warmup': warmup,
                   'block': block, 'utc': datetime.now(timezone.utc).isoformat(),
                   'monotonic_ns': time.perf_counter_ns(), **extra}
        with events_path.open('a', encoding='utf-8') as handle:
            handle.write(json.dumps(payload, sort_keys=True) + '\n')
            handle.flush()
            os.fsync(handle.fileno())
    start = time.perf_counter()
    with log_path.open('w', encoding='utf-8', errors='replace') as log:
        record_event('process_start')
        try:
            child = subprocess.run(cmd, cwd=run_dir, stdout=log, stderr=subprocess.STDOUT, check=False)
        finally:
            record_event('process_end')
    wall = time.perf_counter() - start
    if child.returncode != 0:
        raise RuntimeError(f'{label} exit code {child.returncode}; inspect {log_path}')
    if not result_path.is_file():
        raise RuntimeError(f'{label} produced no result at {result_path}')
    result = read_json(result_path)
    if result.get('benchmark_pass') is False:
        raise RuntimeError(f'{label} benchmark reports benchmark_pass=false: {result_path}')
    if int(result.get('rays', -1)) != args.rays or int(result.get('seed', -1)) != SEED:
        raise RuntimeError(f'{label} returned unexpected rays or seed: {result_path}')
    # BenchmarkRunner prints rays_traced on stdout but does not put it in result.json.
    output_text = log_path.read_text(encoding='utf-8', errors='replace')
    match = re.search(r'(?m)^rays_traced:\s*(\d+)\s*$', output_text)
    if not match or int(match.group(1)) != args.rays:
        raise RuntimeError(f'{label} did not report all rays traced: {log_path}')
    if int(result.get('chunk_size', -1)) != 10000:
        raise RuntimeError(f'{label} unexpected chunk size: {result_path}')
    elapsed = float(result['elapsed_seconds'])
    if not math.isfinite(elapsed) or elapsed <= 0:
        raise RuntimeError(f'{label} invalid elapsed time: {elapsed}')
    for key in SCIENTIFIC_FIELDS:
        if key not in result:
            raise RuntimeError(f'{label} missing scientific field: {key}')
    entry = {
        'index': index, 'version': label, 'warmup': warmup, 'block': block,
        'elapsed_seconds': elapsed, 'wall_seconds': round(wall, 4),
        'rays_per_second': float(result['rays_per_second']), 'worker_count': int(result['worker_count']),
        'chunk_count': int(result['chunk_count']), 'chunk_size': int(result['chunk_size']),
        'result_path': str(result_path), 'log_path': str(log_path),
        'scientific': {key: result[key] for key in SCIENTIFIC_FIELDS},
    }
    print(f'      {elapsed:.3f}s  {entry["rays_per_second"]:.0f} rays/s  workers={entry["worker_count"]}', flush=True)
    return entry


def compare_science(entries: list[dict], strict: bool) -> list[str]:
    # Use deterministic exact comparisons for this same-machine, same-build-toolchain use case.
    # No claim of cross-platform bitwise reproducibility.
    anchor = entries[0]['scientific']
    mismatches = []
    for e in entries[1:]:
        for key in SCIENTIFIC_FIELDS:
            value = e['scientific'][key]
            if value != anchor[key]:
                mismatches.append(f'{e["index"]:02d} {e["version"]} {key}: {value!r} != {anchor[key]!r}')
    if strict and mismatches:
        raise RuntimeError('Scientific mismatch; performance acceptance BLOCKED:\n' + '\n'.join(mismatches[:25]))
    return mismatches


def percent_change(after: float, before: float) -> float:
    return (after / before - 1.0) * 100.0


def span_percent(values: list[float]) -> float:
    return percent_change(max(values), min(values)) if len(values) > 1 else 0.0


def analyze(entries: list[dict], threshold: float, stability_limit: float,
            agreement_limit: float) -> dict:
    """Use ABBA-local effects; check heating/drift independently of measured effect.

    A block has baseline, candidate, candidate, baseline. Endpoint drift is
    measured using the identical baseline binary before and after both candidate
    runs. This is not a statistical confidence interval or a causal proof.
    """
    timed = [e for e in entries if not e.get('warmup')]
    base = [e['elapsed_seconds'] for e in timed if e['version'] == 'baseline']
    candidate = [e['elapsed_seconds'] for e in timed if e['version'] == 'candidate']
    if not base or len(base) != len(candidate):
        raise ValueError('Unbalanced measured runs')
    mb, mc = statistics.median(base), statistics.median(candidate)
    global_change = percent_change(mc, mb)
    blocks = []
    for i in range(0, len(timed), 4):
        block = timed[i:i + 4]
        if len(block) != 4 or [e['version'] for e in block] != [
                'baseline', 'candidate', 'candidate', 'baseline']:
            raise ValueError('Expected ordered baseline,candidate,candidate,baseline block')
        b1, c1, c2, b2 = (e['elapsed_seconds'] for e in block)
        bb, cc = (statistics.mean([b1, b2]), statistics.mean([c1, c2]))
        blocks.append({
            'block': len(blocks) + 1,
            'baseline_mean_seconds': bb,
            'candidate_mean_seconds': cc,
            'effect_percent': percent_change(cc, bb),
            'baseline_endpoint_drift_percent': percent_change(b2, b1),
            'candidate_pair_drift_percent': percent_change(c2, c1),
        })
    effects = [b['effect_percent'] for b in blocks]
    baseline_means = [b['baseline_mean_seconds'] for b in blocks]
    candidate_means = [b['candidate_mean_seconds'] for b in blocks]
    # Equal work (same ray count), so unweighted block-level percent changes
    # are meaningful. Median gives a robust central effect estimate.
    paired_change = statistics.median(effects)
    baseline_range = span_percent(base)
    candidate_range = span_percent(candidate)
    block_baseline_spread = span_percent(baseline_means)
    block_candidate_spread = span_percent(candidate_means)
    effect_spread = max(effects) - min(effects)
    endpoint_drift = max(abs(b['baseline_endpoint_drift_percent']) for b in blocks)
    candidate_drift = max(abs(b['candidate_pair_drift_percent']) for b in blocks)
    reasons = []
    if endpoint_drift > stability_limit:
        reasons.append(f'Baseline within-block endpoint drift {endpoint_drift:.2f}% > {stability_limit:.2f}%')
    if candidate_drift > stability_limit:
        reasons.append(f'Candidate within-block drift {candidate_drift:.2f}% > {stability_limit:.2f}%')
    if block_baseline_spread > stability_limit:
        reasons.append(f'Baseline block-to-block mean spread {block_baseline_spread:.2f}% > {stability_limit:.2f}%')
    if block_candidate_spread > stability_limit:
        reasons.append(f'Candidate block-to-block mean spread {block_candidate_spread:.2f}% > {stability_limit:.2f}%')
    if effect_spread > agreement_limit:
        reasons.append(f'ABBA effects disagree by {effect_spread:.2f} percentage points > {agreement_limit:.2f}')
    if len(blocks) < 2:
        reasons.append('At least two measured ABBA blocks required for an acceptance verdict')
    if reasons:
        verdict = 'INCONCLUSIVE / UNSTABLE ENVIRONMENT'
    elif paired_change > threshold:
        verdict = 'POTENTIAL REGRESSION / CONFIRM BEFORE ACCEPTANCE'
    elif paired_change < -threshold:
        verdict = 'POTENTIAL IMPROVEMENT / CONFIRM BEFORE ACCEPTANCE'
    else:
        verdict = 'NO CHANGE RESOLVED / NOT PROVEN EQUIVALENT'
    return {
        'baseline_median_seconds': mb, 'candidate_median_seconds': mc,
        'elapsed_change_percent': global_change,
        'throughput_change_percent_from_medians': percent_change(mb, mc),
        'paired_effect_median_percent': paired_change,
        'baseline_range_percent': baseline_range, 'candidate_range_percent': candidate_range,
        'abba_block_percent_changes': effects,
        'abba_blocks': blocks,
        'baseline_block_mean_spread_percent': block_baseline_spread,
        'candidate_block_mean_spread_percent': block_candidate_spread,
        'abba_effect_spread_percentage_points': effect_spread,
        'max_baseline_endpoint_drift_percent': endpoint_drift,
        'max_candidate_pair_drift_percent': candidate_drift,
        'stability_reasons': reasons,
        'stability_limit_percent': stability_limit,
        'agreement_limit_percentage_points': agreement_limit,
        'threshold_percent': threshold,
        'verdict': verdict,
    }


def write_reports(root: Path, meta: dict, entries: list[dict], summary: dict, mismatches: list[str]) -> None:
    data = {'metadata': meta, 'summary': summary, 'scientific_matches': not mismatches,
            'scientific_mismatches': mismatches, 'runs': entries}
    (root / 'report.json').write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    with (root / 'runs.csv').open('w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=['index','version','warmup','block','elapsed_seconds','wall_seconds','rays_per_second','worker_count','chunk_count','chunk_size'])
        w.writeheader()
        for e in entries:
            w.writerow({k: e.get(k, '') for k in w.fieldnames})
    lines = [
        '# Tonatiuh++ pre-commit performance comparison', '',
        f'- Created: {meta["timestamp_utc"]}', f'- Rays per run: {meta["rays"]:,}; seed: {SEED}',
        f'- Scene SHA-256: `{meta["scene_sha256"]}`',
        f'- Baseline binary SHA-256: `{meta["baseline_exe_sha256"]}`',
        f'- Candidate binary SHA-256: `{meta["candidate_exe_sha256"]}`',
        f'- Scientific results: {"MATCH" if not mismatches else "MISMATCH - BLOCK"}', '',
        '| Metric | Value |', '|---|---:|',
        f'| Baseline median (s) | {summary["baseline_median_seconds"]:.3f} |',
        f'| Candidate median (s) | {summary["candidate_median_seconds"]:.3f} |',
        f'| Candidate elapsed-time difference | {summary["elapsed_change_percent"]:+.3f}% |',
        f'| Candidate throughput difference (overall medians) | {summary["throughput_change_percent_from_medians"]:+.3f}% |',
        f'| **Paired median effect (decision metric)** | **{summary["paired_effect_median_percent"]:+.3f}%** |',
        f'| Baseline within-version spread | {summary["baseline_range_percent"]:.3f}% |',
        f'| Candidate within-version spread | {summary["candidate_range_percent"]:.3f}% |',
        f'| Baseline block endpoint drift (maximum absolute) | {summary["max_baseline_endpoint_drift_percent"]:.3f}% |',
        f'| Candidate within-block drift (maximum absolute) | {summary["max_candidate_pair_drift_percent"]:.3f}% |',
        f'| Baseline between-block spread | {summary["baseline_block_mean_spread_percent"]:.3f}% |',
        f'| Candidate between-block spread | {summary["candidate_block_mean_spread_percent"]:.3f}% |',
        f'| ABBA effect disagreement (percentage points) | {summary["abba_effect_spread_percentage_points"]:.3f} |',
        f'| ABBA block differences | {[round(x, 4) for x in summary["abba_block_percent_changes"]]} |', '',
        f'**Verdict:** {summary["verdict"]}', '',
        '## Measurements', '', '| # | Version | Warmup | Block | Time (s) | Workers |', '|---:|---|---|---:|---:|---:|',
    ]
    for e in entries:
        lines.append(f'| {e["index"]} | {e["version"]} | {"yes" if e.get("warmup") else "no"} | {e.get("block", 0)} | {e["elapsed_seconds"]:.3f} | {e["worker_count"]} |')
    lines += ['', '## Block diagnostics', '',
              '| Block | Candidate effect | Baseline endpoint drift | Candidate pair drift |',
              '|---:|---:|---:|---:|']
    for b in summary['abba_blocks']:
        lines.append(f'| {b["block"]} | {b["effect_percent"]:+.3f}% | {b["baseline_endpoint_drift_percent"]:+.3f}% | {b["candidate_pair_drift_percent"]:+.3f}% |')
    lines += ['', '## Stability diagnostics', '']
    if summary['stability_reasons']:
        lines += [f'- {reason}' for reason in summary['stability_reasons']]
    else:
        lines += ['- All configured drift and block-consistency checks passed.']
    lines += ['', '## Interpretation', '',
              'Scientific equality is required. Warm-up results are validated but excluded from timing analysis.',
              'The paired median ABBA-block effect is the decision metric. Overall independent medians are reported for continuity only.',
              'Stability thresholds are heuristics, not statistical confidence intervals or proof of causation.',
              'The elapsed metric includes benchmark preparation and worker execution; it excludes later flux-grid serialization.',
              'Run under similar power, CPU-temperature, clock and background-load conditions.',
              'This harness never modifies the 500M frozen scientific reference or its companion files.', '']
    (root / 'report.md').write_text('\n'.join(lines), encoding='utf-8')


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--baseline-exe', type=Path, required=True)
    p.add_argument('--candidate-exe', type=Path, required=True)
    p.add_argument('--baseline-scene', type=Path, required=True)
    p.add_argument('--candidate-scene', type=Path, required=True)
    p.add_argument('--baseline-legacy', action='store_true', help='use old GUI --headless benchmark syntax')
    p.add_argument('--candidate-legacy', action='store_true')
    p.add_argument('--rays', type=positive_int, default=5_000_000)
    p.add_argument('--abba-blocks', type=positive_int, default=2, help='four measured runs each')
    p.add_argument('--warmup', action='store_true', help='legacy synonym for at least one warmup pair')
    p.add_argument('--warmup-pairs', type=int, default=2, help='unmeasured baseline+candidate warmup pairs (default 2)')
    p.add_argument('--stability-limit-percent', type=float, default=3.0, help='max absolute drift and block mean spread')
    p.add_argument('--agreement-limit-percentage-points', type=float, default=2.0, help='max ABBA effect spread')
    p.add_argument('--threshold-percent', type=float, default=2.0)
    p.add_argument('--output-dir', type=Path, required=True)
    p.add_argument('--repo', type=Path, default=Path.cwd())
    return p.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    if not math.isfinite(args.threshold_percent) or args.threshold_percent <= 0:
        raise ValueError('--threshold-percent must be finite and >0')
    if args.warmup_pairs < 0:
        raise ValueError('--warmup-pairs must be >= 0')
    if args.warmup:
        args.warmup_pairs = max(1, args.warmup_pairs)
    for name in ('stability_limit_percent', 'agreement_limit_percentage_points'):
        val = getattr(args, name)
        if not math.isfinite(val) or val <= 0:
            raise ValueError(f'Invalid stability parameter: {name}')
    for field in ['baseline_exe','candidate_exe','baseline_scene','candidate_scene']:
        path = getattr(args, field).resolve()
        if not path.is_file():
            raise FileNotFoundError(f'{field}: {path}')
        setattr(args, field, path)
    if args.baseline_exe == args.candidate_exe:
        raise ValueError('Baseline and candidate executables must be different paths')
    scene_hash = sha256(args.baseline_scene)
    if scene_hash != sha256(args.candidate_scene):
        raise ValueError('Scene SHA-256 mismatch: cannot compare different workloads')
    out = args.output_dir.resolve()
    # Protect both scene inputs, executables and user-supplied canonical reference tree.
    guarded_output_root(out)
    meta = {
        'harness_version': 2,
        'timestamp_utc': datetime.now(timezone.utc).isoformat(),
        'rays': args.rays, 'seed': SEED, 'scene_sha256': scene_hash,
        'warmup_pairs': args.warmup_pairs, 'abba_blocks': args.abba_blocks,
        'stability_limit_percent': args.stability_limit_percent,
        'agreement_limit_percentage_points': args.agreement_limit_percentage_points,
        'baseline_exe': str(args.baseline_exe), 'candidate_exe': str(args.candidate_exe),
        'baseline_exe_sha256': sha256(args.baseline_exe), 'candidate_exe_sha256': sha256(args.candidate_exe),
        'python': sys.version, 'platform': platform.platform(), 'cpu_count': os.cpu_count(),
        'git': git_info(args.repo.resolve()),
        'order': ('baseline,candidate,candidate,baseline') * args.abba_blocks,
    }
    (out / 'metadata.json').write_text(json.dumps(meta, indent=2) + '\n', encoding='utf-8')
    entries: list[dict] = []
    idx = 0
    try:
        for pair in range(args.warmup_pairs):
            print(f'WARM-UP pair {pair + 1}/{args.warmup_pairs} (not timed for comparison)', flush=True)
            for label in ('baseline', 'candidate'):
                idx += 1
                entries.append(run_once(label, idx, args, out, warmup=True))
        for block in range(1, args.abba_blocks + 1):
            for label in ('baseline', 'candidate', 'candidate', 'baseline'):
                idx += 1
                entries.append(run_once(label, idx, args, out, block=block))
        mismatches = compare_science(entries, strict=True)
        measured = [e for e in entries if not e.get('warmup')]
        worker_counts = {e['worker_count'] for e in measured}
        if len(worker_counts) != 1:
            raise RuntimeError(f'Worker counts differ across measured runs: {worker_counts}')
        summary = analyze(entries, args.threshold_percent, args.stability_limit_percent,
                          args.agreement_limit_percentage_points)
        write_reports(out, meta, entries, summary, mismatches)
        print('\n' + summary['verdict'], flush=True)
        print(f'Candidate time change (overall medians): {summary["elapsed_change_percent"]:+.3f}%')
        print(f'Candidate time change (paired blocks): {summary["paired_effect_median_percent"]:+.3f}%')
        if summary['stability_reasons']:
            for reason in summary['stability_reasons']:
                print(f'  STABILITY: {reason}')
        print(f'Reports: {out / "report.md"} and {out / "report.json"}')
        return 3 if 'POTENTIAL REGRESSION' in summary['verdict'] else 0
    except Exception as exc:
        # Preserve any successfully completed measurements for diagnosis.
        (out / 'FAILED.txt').write_text(str(exc) + '\n', encoding='utf-8')
        (out / 'partial_runs.json').write_text(json.dumps(entries, indent=2) + '\n', encoding='utf-8')
        raise


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f'ERROR: {error}', file=sys.stderr)
        sys.exit(2)
