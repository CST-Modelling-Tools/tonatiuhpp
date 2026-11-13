"""Bounded scientific outputs through Tonatiuh++'s existing headless services.

Interception power and flux values are calculated only by the native C++ CLI.
Python validates and records the returned files; it does not trace photons.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path


@dataclass(frozen=True)
class OpticalRequest:
    """Request independent native interception and flux traces.

    The interception API currently uses its established default master seed 0;
    the flux API accepts the study case's explicit master seed. Both update
    trackers. The two outputs come from separate traces, not the CLI trace.
    """
    surface_url: str
    interception_rays: int = 1024
    flux_rays: int = 1024
    height_bins: int = 8
    width_bins: int = 8
    surface_side: str = "front"


@dataclass(frozen=True)
class OpticalResult:
    intercepted_power_watts: float
    flux_grid: Path
    flux_min_w_m2: float
    flux_max_w_m2: float
    flux_mean_w_m2: float
    flux_sha256: str
    measurements_json: Path
    summary_json: Path


def _validate_optical(request: OpticalRequest) -> None:
    if not isinstance(request, OpticalRequest):
        raise ValueError("optical must be an OpticalRequest")
    if not isinstance(request.surface_url, str) or not request.surface_url.strip() or "\x00" in request.surface_url:
        raise ValueError("optical surface_url must be a nonempty scene shape URL")
    if request.surface_side not in ("front", "back"):
        raise ValueError("optical surface_side must be 'front' or 'back'")
    for name, value, lower, upper in (
        ("interception_rays", request.interception_rays, 1, 2**32 - 1),
        ("flux_rays", request.flux_rays, 1, 200000),
        ("height_bins", request.height_bins, 2, 1000000),
        ("width_bins", request.width_bins, 2, 1000000),
    ):
        if type(value) is not int or not lower <= value <= upper:
            raise ValueError(f"optical {name} must be an integer between {lower} and {upper}")
    if request.height_bins * request.width_bins > 1000000:
        raise ValueError("optical flux grid may contain at most 1,000,000 cells")


def _js(value: object) -> str:
    return json.dumps(value, ensure_ascii=True, allow_nan=False)


def _measurement_script(scene: Path, request: OpticalRequest, seed: int,
                        grid: Path, measurements: Path) -> str:
    # All paths and the shape URL are JSON-encoded JavaScript string literals.
    # Named native methods are verified by the repository's headless tests.
    return "\n".join([
        '"use strict";',
        f"if (!tn.loadScene({_js(str(scene))})) throw new Error('optical scene load failed');",
        f"var interceptedPower = tn.FindInterception({_js(request.surface_url)}, {request.interception_rays});",
        "if (typeof interceptedPower !== 'number' || !isFinite(interceptedPower) || interceptedPower < 0) throw new Error('invalid intercepted power');",
        f"tn.RunFluxAnalysis({_js(request.surface_url)}, {_js(request.surface_side)}, {request.flux_rays}, {request.height_bins}, {request.width_bins}, {_js(str(grid))}, false, {seed});",
        f"if (!tn.writeJson({_js(str(measurements))}, " + "{"
        + f"schema: 'tonatiuhpp.python.optical-measurements.v1', intercepted_power_watts: interceptedPower, surface_url: {_js(request.surface_url)}, "
        + f"interception_rays: {request.interception_rays}, interception_master_seed: 0, flux_rays: {request.flux_rays}, "
        + f"flux_master_seed: {seed}, height_bins: {request.height_bins}, width_bins: {request.width_bins}, "
        + f"surface_side: {_js(request.surface_side)}"
        + "})) throw new Error('optical results JSON write failed');",
        "",
    ])


def _flux_numbers(grid: Path, request: OpticalRequest) -> tuple[list[float], str]:
    # Legacy no-coordinate TSV: one row per height bin, tab-separated columns,
    # values are native FluxAnalysisTsvWriter outputs in W/m2.
    raw = grid.read_bytes()
    rows = raw.decode("utf-8-sig").splitlines()
    if len(rows) != request.height_bins:
        raise ValueError(f"flux grid row count {len(rows)} != {request.height_bins}")
    numbers: list[float] = []
    for index, row in enumerate(rows):
        cells = row.split()
        if len(cells) != request.width_bins:
            raise ValueError(f"flux grid row {index} width {len(cells)} != {request.width_bins}")
        for cell in cells:
            try:
                value = float(cell)
            except ValueError as exc:
                raise ValueError("flux grid contains nonnumeric data") from exc
            if not math.isfinite(value):
                raise ValueError("flux grid contains nonfinite data")
            numbers.append(value)
    return numbers, hashlib.sha256(raw).hexdigest()


def _measure_optics(client, prepared: Path, folder: Path, request: OpticalRequest,
                    seed: int, timeout: float | None) -> OpticalResult:
    script = folder / "measure_optics.tnhpps"
    grid = folder / "flux_grid_w_m2.tsv"
    measurements = folder / "optical_measurements.json"
    summary = folder / "optical_summary.json"
    script.write_text(_measurement_script(prepared, request, seed, grid, measurements), encoding="utf-8")
    client.run_script(script, timeout=timeout)
    if not measurements.is_file() or not grid.is_file():
        raise RuntimeError("native optical analysis did not produce both expected result files")
    data = json.loads(measurements.read_text(encoding="utf-8"))
    expected = {
        "schema": "tonatiuhpp.python.optical-measurements.v1",
        "surface_url": request.surface_url,
        "interception_rays": request.interception_rays,
        "interception_master_seed": 0,
        "flux_rays": request.flux_rays,
        "flux_master_seed": seed,
        "height_bins": request.height_bins,
        "width_bins": request.width_bins,
        "surface_side": request.surface_side,
    }
    for key, value in expected.items():
        if data.get(key) != value or (type(value) is int and type(data.get(key)) is not int):
            raise ValueError(f"unexpected optical measurement field: {key}")
    power = data.get("intercepted_power_watts")
    if type(power) not in (int, float) or not math.isfinite(power) or power < 0:
        raise ValueError("native intercepted power must be a finite nonnegative number")
    values, digest = _flux_numbers(grid, request)
    record = {
        "schema": "tonatiuhpp.python.optical-summary.v1",
        "surface_url": request.surface_url,
        "intercepted_power_watts": power,
        "flux_units": "W/m2",
        "flux_grid_file": grid.name,
        "flux_min_w_m2": min(values),
        "flux_max_w_m2": max(values),
        "flux_mean_w_m2": math.fsum(values) / len(values),
        "flux_sha256": digest,
        "interception_rays": request.interception_rays,
        "interception_master_seed": 0,
        "flux_rays": request.flux_rays,
        "flux_master_seed": seed,
        "height_bins": request.height_bins,
        "width_bins": request.width_bins,
        "surface_side": request.surface_side,
        "note": "Interception, flux, and CLI execution counters are separate native traces. Do not sum or equate their estimates.",
    }
    summary.write_text(json.dumps(record, indent=2, allow_nan=False) + "\n", encoding="utf-8")
    return OpticalResult(float(power), grid, record["flux_min_w_m2"],
                         record["flux_max_w_m2"], record["flux_mean_w_m2"],
                         digest, measurements, summary)
