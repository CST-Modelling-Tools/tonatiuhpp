#!/usr/bin/env python3
"""Snapshot and compare a configured CTest inventory (no native tests executed).

No dependencies outside the Python standard library. Intentionally conservative:
all runtime tiers default to regression, not the overloaded legacy smoke label.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import sys
from typing import Any


REQUIRED_SENTINELS = (
    "architecture.gui_cli_runtime_boundary",
    "headless.cli_independent_process_equivalence",
    "headless.large_file_export_preflight",
)


def property_map(props: list[dict[str, Any]]) -> dict[str, Any]:
    return {str(item["name"]): item.get("value") for item in props if "name" in item}


def classify(test: dict[str, Any]) -> tuple[str, str, str]:
    name = str(test.get("name", ""))
    raw_labels = property_map(test.get("properties", [])).get("LABELS", [])
    if isinstance(raw_labels, str):
        raw_labels = raw_labels.split(";")
    cmd = [str(v) for v in test.get("command", [])]
    joined = " ".join(cmd).lower()
    gtest = any(arg.startswith("--gtest_filter=") for arg in cmd)
    if gtest:
        interface = "cpp-google-test"
        level = "unit" if "unit" in raw_labels else "component"
    elif name.startswith("architecture."):
        level, interface = "system", "process-architecture"
    elif name.startswith("headless."):
        level, interface = "integration", "cli"
    else:
        level, interface = "unknown", "unknown"
    if name.startswith("scientific.") or "scientific" in property_map(test.get("properties", [])).get("LABELS", []):
        domain = "scientific"
    elif name.startswith("architecture."):
        domain = "architecture"
    elif "random" in name.lower():
        domain = "random"
    elif "math" in name.lower() or "box2d" in name.lower():
        domain = "math"
    elif "export" in name.lower() or "file" in name.lower():
        domain = "export"
    elif "script" in name.lower():
        domain = "scripting"
    else:
        domain = "unclassified"
    return level, domain, interface


def normalize(raw: dict[str, Any]) -> dict[str, Any]:
    if raw.get("kind") != "ctestInfo" or raw.get("version", {}).get("major") != 1:
        raise ValueError("Input is not CTest --show-only=json-v1 information")
    tests = []
    for item in raw.get("tests", []):
        props = property_map(item.get("properties", []))
        labels = props.get("LABELS", [])
        if isinstance(labels, str):
            labels = labels.split(";")
        elif not isinstance(labels, list):
            labels = [str(labels)]
        level, domain, interface = classify(item)
        tests.append({
            "name": item["name"],
            "command": [str(v) for v in item.get("command", [])],
            "labels": sorted(str(x) for x in labels),
            "timeout_seconds": props.get("TIMEOUT"),
            "working_directory": props.get("WORKING_DIRECTORY"),
            "level_inferred": level,
            "domain_inferred": domain,
            "interface_inferred": interface,
            "tier": "regression (default; not runtime-measured)",
        })
    names = [v["name"] for v in tests]
    if len(names) != len(set(names)):
        from collections import Counter
        dupes = [name for name, count in Counter(names).items() if count > 1]
        raise ValueError(f"Duplicate CTest names: {dupes}")
    tests.sort(key=lambda x: x["name"])
    return {"schema": "tonatiuhpp.ctest-inventory.v1", "count": len(tests), "tests": tests}


def markdown(data: dict[str, Any]) -> str:
    header = [
        "# Configured Tonatiuh++ CTest inventory", "",
        f"Registered tests: **{data['count']}**. Labels can overlap; totals are not additive.",
        "Inferred categories are preliminary, not evidence of a GUI or reference comparison.",
        "", "| Test | Labels | Level (inferred) | Domain (inferred) | Interface (inferred) | Timeout (s) |",
        "| --- | --- | --- | --- | --- | --- |",
    ]
    for t in data["tests"]:
        fields = [t["name"], ", ".join(t["labels"]), t["level_inferred"],
                  t["domain_inferred"], t["interface_inferred"], str(t["timeout_seconds"] or "-")]
        fields = [v.replace("|", r"\|").replace("\n", " ") for v in fields]
        header.append("| " + " | ".join(fields) + " |")
    return "\n".join(header) + "\n"


def baseline_names(path: Path) -> set[str]:
    obj = json.loads(path.read_text(encoding="utf-8"))
    if obj.get("schema") == "tonatiuhpp.ctest-inventory.v1":
        return {str(t["name"]) for t in obj["tests"]}
    if obj.get("kind") == "ctestInfo":
        return {str(t["name"]) for t in obj["tests"]}
    raise ValueError("Baseline must be a saved inventory or raw CTest JSON output")


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--build-dir", required=True, type=Path)
    p.add_argument("--config", default="Release")
    p.add_argument("--json-out", type=Path)
    p.add_argument("--markdown-out", type=Path)
    p.add_argument("--baseline", type=Path)
    p.add_argument("--expect-count", type=int)
    p.add_argument("--require-sentinels", action="store_true")
    args = p.parse_args(argv)
    command = ["ctest", "--test-dir", str(args.build_dir), "-C", args.config, "--show-only=json-v1"]
    try:
        pcall = subprocess.run(command, text=True, encoding="utf-8", errors="replace",
                               capture_output=True, timeout=90, check=False)
        if pcall.returncode != 0:
            raise RuntimeError(f"CTest discovery failed: {pcall.stderr[-3000:]}")
        data = normalize(json.loads(pcall.stdout))
        names = {t["name"] for t in data["tests"]}
        print(f"Discovered {data['count']} unique CTests in {args.build_dir}")
        if args.expect_count is not None and data["count"] != args.expect_count:
            raise ValueError(f"Expected {args.expect_count} CTests, got {data['count']}")
        if args.require_sentinels:
            missing = sorted(set(REQUIRED_SENTINELS) - names)
            if missing:
                raise ValueError(f"Missing mandatory sentinel registrations: {missing}")
        if args.baseline:
            before = baseline_names(args.baseline)
            missing = sorted(before - names)
            added = sorted(names - before)
            if missing or added:
                raise ValueError(f"CTest name-set changed; missing={missing}; added={added}")
            print(f"CTest names match baseline {args.baseline} ({len(before)} names)")
        for output, body in [(args.json_out, json.dumps(data, indent=2, ensure_ascii=False) + "\n"),
                             (args.markdown_out, markdown(data))]:
            if output:
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_text(body, encoding="utf-8")
                print(f"Wrote {output}")
        return 0
    except (ValueError, OSError, RuntimeError, subprocess.TimeoutExpired, json.JSONDecodeError) as exc:
        print(f"INVENTORY FAILED: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
