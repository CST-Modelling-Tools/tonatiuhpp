"""External, standard-library-only automation of the production Tonatiuh++ CLI."""
from __future__ import annotations
from dataclasses import dataclass
from pathlib import Path
import os
import subprocess
from typing import Mapping, Sequence

@dataclass(frozen=True)
class CliResult:
    """Captured process result; console text is not machine-readable science data."""
    command: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str

class CliError(RuntimeError):
    def __init__(self, message: str, *, result: CliResult | None = None):
        super().__init__(message)
        self.result = result

class TonatiuhClient:
    """Run an existing CLI executable without embedding Python in Tonatiuh++."""
    def __init__(self, executable: str | os.PathLike[str], *,
                 timeout: float | None = 300.0,
                 cwd: str | os.PathLike[str] | None = None,
                 env: Mapping[str, str] | None = None):
        if timeout is not None and timeout <= 0:
            raise ValueError("timeout must be positive or None")
        self.executable = Path(executable).expanduser().resolve()
        if not self.executable.is_file():
            raise FileNotFoundError(f"Tonatiuh++ CLI executable not found: {self.executable}")
        self.timeout = timeout
        self.cwd = Path(cwd).expanduser().resolve() if cwd is not None else None
        if self.cwd is not None and not self.cwd.is_dir():
            raise NotADirectoryError(self.cwd)
        self.env = dict(env) if env is not None else None

    @staticmethod
    def _existing_file(path: str | os.PathLike[str]) -> str:
        target = Path(path).expanduser().resolve()
        if not target.is_file():
            raise FileNotFoundError(target)
        return str(target)

    def _run(self, args: Sequence[str], *, timeout: float | None = None) -> CliResult:
        command = (str(self.executable), *args)
        try:
            completed = subprocess.run(command, cwd=self.cwd, env=self.env,
                                       capture_output=True, text=True, encoding="utf-8",
                                       errors="replace", timeout=self.timeout if timeout is None else timeout,
                                       check=False)
        except subprocess.TimeoutExpired as exc:
            raise CliError(f"Tonatiuh++ CLI timed out: {args[0]}") from exc
        except OSError as exc:
            raise CliError(f"Cannot launch Tonatiuh++ CLI: {exc}") from exc
        result = CliResult(command, completed.returncode, completed.stdout, completed.stderr)
        if completed.returncode != 0:
            raise CliError(f"Tonatiuh++ CLI {args[0]} failed with exit code {completed.returncode}: "
                           f"{completed.stderr.strip() or completed.stdout.strip()}", result=result)
        return result

    def validate_scene(self, scene: str | os.PathLike[str]) -> CliResult:
        return self._run(("validate-scene", self._existing_file(scene)))

    def run_script(self, script: str | os.PathLike[str], *,
                   timeout: float | None = None) -> CliResult:
        """Execute an existing .tnhpps JavaScript file through the production CLI."""
        return self._run(("run-script", self._existing_file(script)), timeout=timeout)

    def trace_scene(self, scene: str | os.PathLike[str], *, rays: int, seed: int,
                    update_trackers: bool = False,
                    photon_export_dir: str | os.PathLike[str] | None = None,
                    photon_file_name: str | None = None,
                    photon_file_size: int | None = None,
                    photon_surfaces: Sequence[str] = (),
                    timeout: float | None = None) -> CliResult:
        """Execute CLI tracing. Export requires a caller-created, empty directory."""
        if type(rays) is not int or not (0 < rays <= 2**63 - 1):
            raise ValueError("rays must be a positive 64-bit signed integer")
        if type(seed) is not int or not (0 <= seed <= 2**64 - 1):
            raise ValueError("seed must be a nonnegative 64-bit unsigned integer")
        args = ["trace-scene", self._existing_file(scene), "--rays", str(rays), "--seed", str(seed)]
        if photon_export_dir is None:
            if photon_file_name is not None or photon_file_size is not None or photon_surfaces:
                raise ValueError("photon File options require photon_export_dir")
            args.append("--no-export")
        else:
            target = Path(photon_export_dir).expanduser().resolve()
            if not target.is_dir():
                raise NotADirectoryError(f"Photon export directory must exist: {target}")
            if any(target.iterdir()):
                raise ValueError(f"Photon export directory must be empty: {target}")
            args.extend(("--photon-export-dir", str(target)))
            if photon_file_name is not None:
                if not photon_file_name or Path(photon_file_name).name != photon_file_name:
                    raise ValueError("photon_file_name must be a nonempty basename")
                args.extend(("--photon-file-name", photon_file_name))
            if photon_file_size is not None:
                if type(photon_file_size) is not int or (photon_file_size != -1 and photon_file_size <= 0):
                    raise ValueError("photon_file_size must be -1 or a positive integer")
                args.extend(("--photon-file-size", str(photon_file_size)))
            for surface in photon_surfaces:
                if not surface:
                    raise ValueError("photon surface URL must be nonempty")
                args.extend(("--photon-surface", surface))
        if update_trackers:
            args.append("--update-trackers")
        return self._run(args, timeout=timeout)
