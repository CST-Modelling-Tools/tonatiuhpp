#!/usr/bin/env python3
# Tonatiuh++ dependency builder (incremental)
# - Builds dependencies defined in third_party/deps.yaml
# - Installs under third_party/_install
# - Verifies with header/lib checks and an optional compile-check
# - Skips already-verified deps via .ok markers (use --force to rebuild)
# - Windows: forces x64 generator and ensures MSVC x64 env (via VsDevCmd if available,
#             otherwise by deriving VC/SDK lib/include paths)
# - Writes cmake/LocalDepsHints.cmake with discovered prefixes (Qt, Eigen, Boost, _install)
# - Doctor shows global vs effective PATH and warns only on effective PATH
#
# IMPROVEMENTS (requested):
# - Add explicit overrides for Qt/Boost/Eigen roots (CLI + env), keeping auto-detect fallback.
# - Fix Eigen detection to support non-3.x layouts (e.g. C:\eigen-5.0.0).
# - Make VsDevCmd discovery work for newer VS versions (not hardcoded to 2022 paths).
# - Avoid adding ARM64 Qt prefixes when targeting x64 (Windows).
# - Improve validation and error messages for overridden roots.

import argparse
import fnmatch
import glob
import os
import platform
import re
import shutil
import subprocess
import sys
from pathlib import Path

try:
    import yaml
except Exception:
    print("PyYAML not found. Install with: pip install pyyaml", file=sys.stderr)
    sys.exit(1)

# Paths
ROOT = Path(__file__).resolve().parents[1]
TP = ROOT / "third_party"
BUILD = TP / "_build"
PREFIX = TP / "_install"

# User overrides (set in main()).
# Keys: qt_root, boost_root, eigen_root
USER_OVERRIDES: dict[str, str | None] = {"qt_root": None, "boost_root": None, "eigen_root": None}


# ----------------------------
# Utility helpers
# ----------------------------

def run(cmd, cwd=None, env=None):
    """Run a command with logging and error propagation."""
    print("$", " ".join(map(str, cmd)))
    subprocess.check_call(cmd, cwd=cwd, env=env or os.environ.copy())


def cmake_generator():
    if platform.system() == "Windows":
        if shutil.which("ninja"):
            return ["-G", "Ninja"]
        return []
    if shutil.which("ninja"):
        return ["-G", "Ninja"]
    return []


def header_exists(rel_path: str) -> bool:
    return (PREFIX / rel_path).exists()


def find_lib_files(lib_base: str):
    """Return list of matching library files under PREFIX/lib for given base."""
    libdir = PREFIX / "lib"
    if not libdir.exists():
        return []
    if platform.system() == "Windows":
        patterns = [f"{lib_base}*.lib", f"{lib_base}*.dll"]
    elif platform.system() == "Darwin":
        patterns = [f"lib{lib_base}*.dylib", f"{lib_base}*.a"]
    else:
        patterns = [f"lib{lib_base}*.so*", f"{lib_base}*.a"]

    matches = []
    for fn in os.listdir(libdir):
        if any(fnmatch.fnmatch(fn, pat) for pat in patterns):
            matches.append(libdir / fn)

    # Prefer link libraries over runtime DLLs when both found
    matches.sort(key=lambda p: (p.suffix.lower() in [".dll"], str(p)))
    return matches


# ----------------------------
# Root overrides + validation
# ----------------------------

def _norm(p: str) -> str:
    return os.path.normpath(p)


def _validate_qt_root(qt_root: str) -> str:
    qt_root = _norm(qt_root)
    qt6_dir = os.path.join(qt_root, "lib", "cmake", "Qt6")
    if not os.path.isdir(qt_root):
        raise SystemExit(f"[error] --qt-root points to a non-existing directory: {qt_root}")
    if not os.path.isdir(qt6_dir):
        raise SystemExit(
            f"[error] --qt-root does not look like a Qt prefix (missing {qt6_dir}).\n"
            f"        Expected something like: C:\\Qt\\6.x.x\\msvc2022_64"
        )
    return qt_root


def _validate_boost_root(boost_root: str) -> str:
    boost_root = _norm(boost_root)
    hdr = os.path.join(boost_root, "boost", "version.hpp")
    if not os.path.isdir(boost_root):
        raise SystemExit(f"[error] --boost-root points to a non-existing directory: {boost_root}")
    if not os.path.isfile(hdr):
        # Some installs might be /usr with include/boost/..., but on Windows we expect headers at root.
        hdr2 = os.path.join(boost_root, "include", "boost", "version.hpp")
        if not os.path.isfile(hdr2):
            raise SystemExit(
                f"[error] --boost-root does not contain boost headers:\n"
                f"        missing {hdr}\n"
                f"        (also checked {hdr2})"
            )
    return boost_root


def _validate_eigen_root(eigen_root: str) -> str:
    eigen_root = _norm(eigen_root)
    if not os.path.isdir(eigen_root):
        raise SystemExit(f"[error] --eigen-root points to a non-existing directory: {eigen_root}")

    # Accept multiple layouts:
    # 1) <root>/Eigen/Core
    # 2) <root>/include/eigen3/Eigen/Core
    # 3) <root>/eigen3/Eigen/Core
    ok = (
        os.path.isfile(os.path.join(eigen_root, "Eigen", "Core")) or
        os.path.isfile(os.path.join(eigen_root, "include", "eigen3", "Eigen", "Core")) or
        os.path.isfile(os.path.join(eigen_root, "eigen3", "Eigen", "Core"))
    )
    if not ok:
        raise SystemExit(
            f"[error] --eigen-root does not look like Eigen (cannot find Eigen/Core).\n"
            f"        Checked:\n"
            f"          {os.path.join(eigen_root, 'Eigen', 'Core')}\n"
            f"          {os.path.join(eigen_root, 'include', 'eigen3', 'Eigen', 'Core')}\n"
            f"          {os.path.join(eigen_root, 'eigen3', 'Eigen', 'Core')}\n"
        )
    return eigen_root


def _apply_overrides_from_env_and_args(args: argparse.Namespace) -> None:
    """
    Populate USER_OVERRIDES from:
      - CLI flags (highest priority)
      - Env vars TONATIUH_QT_ROOT / TONATIUH_BOOST_ROOT / TONATIUH_EIGEN_ROOT
    Values are validated and normalized if present.
    """
    qt = args.qt_root or os.environ.get("TONATIUH_QT_ROOT")
    boost = args.boost_root or os.environ.get("TONATIUH_BOOST_ROOT")
    eigen = args.eigen_root or os.environ.get("TONATIUH_EIGEN_ROOT")

    if qt:
        USER_OVERRIDES["qt_root"] = _validate_qt_root(qt)
        print(f"[overrides] Qt root   : {USER_OVERRIDES['qt_root']}")
    if boost:
        USER_OVERRIDES["boost_root"] = _validate_boost_root(boost)
        print(f"[overrides] Boost root: {USER_OVERRIDES['boost_root']}")
    if eigen:
        USER_OVERRIDES["eigen_root"] = _validate_eigen_root(eigen)
        print(f"[overrides] Eigen root: {USER_OVERRIDES['eigen_root']}")


# ----------------------------
# Compiler selection
# ----------------------------

def choose_cxx():
    r"""
    Pick a C++ compiler that matches our target.
    - Windows: prefer 64-bit cl.exe explicitly if available; fallback to cl on PATH.
    - Others: try c++/g++/clang++ in that order.
    """
    if platform.system() == "Windows":
        # Prefer whatever cl.exe is active in the current dev shell
        cl = shutil.which("cl")
        if cl:
            return ("cl", cl)

        # Fallback: try vswhere to find an installed MSVC toolchain
        cl_from_vs = _find_cl_via_vswhere()
        if cl_from_vs:
            return ("cl", cl_from_vs)

    for c in ["c++", "g++", "clang++"]:
        p = shutil.which(c)
        if p:
            return (c, p)
    return (None, None)


# ---------- Windows env discovery & normalization ----------

def _find_vswhere() -> str | None:
    if platform.system() != "Windows":
        return None
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    return vswhere if os.path.exists(vswhere) else None


def _find_vs_installation_path_latest() -> str | None:
    vswhere = _find_vswhere()
    if not vswhere:
        return None
    try:
        out = subprocess.check_output(
            [
                vswhere,
                "-latest",
                "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property", "installationPath",
            ],
            text=True
        ).strip()
        return out or None
    except Exception:
        return None


def _find_vsdevcmd():
    """Locate VsDevCmd.bat via vswhere or common paths. Return path or None."""
    if platform.system() != "Windows":
        return None

    inst = _find_vs_installation_path_latest()
    if inst:
        cand = os.path.join(inst, "Common7", "Tools", "VsDevCmd.bat")
        if os.path.exists(cand):
            return cand

    # Fallback: scan a few common locations (do NOT hardcode a specific year)
    candidates = []
    for base in [r"C:\Program Files\Microsoft Visual Studio", r"C:\Program Files (x86)\Microsoft Visual Studio"]:
        if not os.path.isdir(base):
            continue
        for year in os.listdir(base):
            ydir = os.path.join(base, year)
            if not os.path.isdir(ydir):
                continue
            for edition in ("BuildTools", "Community", "Professional", "Enterprise"):
                candidates.append(os.path.join(ydir, edition, "Common7", "Tools", "VsDevCmd.bat"))

    for p in candidates:
        if os.path.exists(p):
            return p
    return None


def _find_cl_via_vswhere() -> str | None:
    """Try to locate HostX64/x64 cl.exe in the latest VS instance."""
    if platform.system() != "Windows":
        return None
    inst = _find_vs_installation_path_latest()
    if not inst:
        return None
    vc_tools_root = os.path.join(inst, "VC", "Tools", "MSVC")
    if not os.path.isdir(vc_tools_root):
        return None
    versions = sorted([d for d in os.listdir(vc_tools_root) if os.path.isdir(os.path.join(vc_tools_root, d))], reverse=True)
    for ver in versions:
        cand = os.path.join(vc_tools_root, ver, "bin", "Hostx64", "x64", "cl.exe")
        if os.path.exists(cand):
            return cand
    return None


def _split_paths(val: str):
    return [p for p in (val or "").split(os.pathsep) if p]


def _join_paths(paths):
    return os.pathsep.join(paths)


def _probe_sdk_lib_paths():
    """
    Try to locate Windows 10/11 SDK lib dirs (ucrt/um x64). Return list of LIB paths (may be empty).
    """
    roots = [
        r"C:\Program Files (x86)\Windows Kits\10\Lib",
        r"C:\Program Files\Windows Kits\10\Lib",
        r"C:\Program Files (x86)\Windows Kits\11\Lib",
        r"C:\Program Files\Windows Kits\11\Lib",
    ]
    candidates = []
    for root in roots:
        if not os.path.isdir(root):
            continue
        try:
            versions = sorted([d for d in os.listdir(root) if os.path.isdir(os.path.join(root, d))], reverse=True)
        except Exception:
            versions = []
        for ver in versions:
            base = os.path.join(root, ver)
            ucrt = os.path.join(base, "ucrt", "x64")
            um = os.path.join(base, "um", "x64")
            if os.path.isdir(ucrt):
                candidates.append(ucrt)
            if os.path.isdir(um):
                candidates.append(um)
            if candidates:
                return candidates
    return []


def _derive_vc_include_lib_from_cl(cl_path: str):
    r"""
    Given ...\VC\Tools\MSVC\<ver>\bin\Hostx64\x64\cl.exe,
    derive include + lib directories for x64.
    """
    vc_tools = Path(cl_path).resolve().parents[3]  # ...\VC\Tools\MSVC\<ver>
    vc_include = vc_tools / "include"
    vc_lib_x64 = vc_tools / "lib" / "x64"
    inc = [str(vc_include)] if vc_include.exists() else []
    lib = [str(vc_lib_x64)] if vc_lib_x64.exists() else []
    return inc, lib


def load_msvc_env_x64(env_in: dict) -> dict:
    """
    Load VS (MSVC) x64 dev environment by invoking VsDevCmd.bat -arch=x64 -host_arch=x64 (if available),
    capture its environment via `set`, and merge into env_in. Otherwise return env_in unchanged.

    NOTE: We do NOT print a scary error if VsDevCmd is missing; we just fall back.
    """
    if platform.system() != "Windows":
        return env_in.copy()

    vsdev = _find_vsdevcmd()
    if not vsdev:
        return env_in.copy()

    # Important: quote correctly for cmd.exe
    cmd = [
        "cmd.exe", "/s", "/c",
        f"\"\"{vsdev}\" -arch=x64 -host_arch=x64 >nul && set\""
    ]
    try:
        out = subprocess.check_output(cmd, shell=False, text=True)
    except Exception:
        return env_in.copy()

    new_env = env_in.copy()
    for line in out.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            new_env[k.upper()] = v
    return new_env


def ensure_msvc_x64_env(env_in: dict) -> dict:
    r"""
    Ensure MSVC environment variables point to x64 toolchain/libs.
    Strategy:
      1) Try VsDevCmd -arch=x64 -host_arch=x64 (if available).
      2) Sanitize LIB/PATH/INCLUDE to prefer x64 and drop x86 entries.
      3) If CRT libs still missing, derive VC/SDK lib/include paths and prepend them.
    """
    env = env_in.copy()
    if platform.system() != "Windows":
        return env

    # 1) Try to import full VS x64 env
    env = load_msvc_env_x64(env)

    # 2) Prefer x64 over x86 in LIB/PATH/INCLUDE
    def prefer_x64(paths):
        x64 = []
        neutral = []
        for p in paths:
            low = p.replace("/", "\\").lower()
            if "\\lib\\x86" in low or "\\hostx86\\x86" in low:
                continue
            if "\\lib\\x64" in low or "\\hostx64\\x64" in low:
                x64.append(p)
            else:
                neutral.append(p)
        return x64 + neutral

    for key in ("LIB", "PATH", "INCLUDE"):
        vals = _split_paths(env.get(key, ""))
        if vals:
            env[key] = _join_paths(prefer_x64(vals))

    # 3) If LIB still lacks VC/SDK x64 libs, derive and prepend
    cc_name, cc_path = choose_cxx()
    if cc_name == "cl" and cc_path:
        vc_inc, vc_lib = _derive_vc_include_lib_from_cl(cc_path)
        sdk_lib = _probe_sdk_lib_paths()

        if vc_lib:
            current = _split_paths(env.get("LIB", ""))
            env["LIB"] = _join_paths(list(dict.fromkeys(vc_lib + current)))

        if sdk_lib:
            current = _split_paths(env.get("LIB", ""))
            env["LIB"] = _join_paths(list(dict.fromkeys(sdk_lib + current)))

        if vc_inc:
            current = _split_paths(env.get("INCLUDE", ""))
            env["INCLUDE"] = _join_paths(list(dict.fromkeys(vc_inc + current)))

    return env


def resolve_lib_paths(lib_basenames: list[str], prefixes: list[str]) -> list[str]:
    """
    Given library base names like ["Qt6Core","Qt6Gui"], return absolute paths
    to library files by searching <prefix>/lib across all prefixes.
    """
    exts_win = [".lib", ".dll"]
    exts_macos = [".dylib", ".a"]
    exts_lin = [".so", ".so.6", ".a"]

    is_win = platform.system() == "Windows"
    is_mac = platform.system() == "Darwin"
    exts = exts_win if is_win else (exts_macos if is_mac else exts_lin)

    results = []
    multiarch_lib_subdirs = [
        "lib",
        "lib64",
        "lib/x86_64-linux-gnu",
        "lib/aarch64-linux-gnu",
        "lib/arm-linux-gnueabihf",
    ]
    for base in lib_basenames or []:
        found = None
        for p in prefixes:
            for sub in multiarch_lib_subdirs:
                libdir = os.path.join(p, sub)
                if not os.path.isdir(libdir):
                    continue
                if is_win:
                    candidates = [os.path.join(libdir, f"{base}{ext}") for ext in exts]
                else:
                    candidates = [os.path.join(libdir, f"lib{base}{ext}") for ext in exts]
                for cand in candidates:
                    if os.path.exists(cand):
                        found = cand
                        break
                if found:
                    break
            if found:
                break
        if found:
            results.append(found)
    return results


# ----------------------------
# CMake hints
# ----------------------------

def _normalize_to_cmake_path(p: str) -> str:
    return p.replace("\\", "/")


def _qt6_dir_from_prefix(prefix: str) -> str | None:
    qt6_dir = os.path.join(prefix, "lib", "cmake", "Qt6")
    return qt6_dir if os.path.isdir(qt6_dir) else None


def _detect_qt_prefixes() -> list[str]:
    """
    Auto-detect Qt prefixes.
    Windows: only pick msvc*_64 (avoid msvc_arm64 when building x64).
    """
    found: list[str] = []

    # 1) Respect explicit override first
    if USER_OVERRIDES.get("qt_root"):
        return [USER_OVERRIDES["qt_root"]]  # type: ignore[return-value]

    try:
        if platform.system() == "Windows":
            roots = [r"C:\Qt"]
            patterns = []
            for root in roots:
                if os.path.isdir(root):
                    patterns += [os.path.join(root, "6.*", "msvc*_*")]
            candidates = []
            for pat in patterns:
                candidates += glob.glob(pat)

            # Filter: must contain Qt6 config, and prefer x64 kits
            candidates = [c for c in candidates if _qt6_dir_from_prefix(c)]
            candidates = [c for c in candidates if "arm64" not in c.lower()]

            def ver_key(p):
                parts = Path(p).parts
                for part in parts:
                    if part.startswith("6."):
                        try:
                            return tuple(int(x) for x in part.split(".") if x.isdigit())
                        except Exception:
                            return (0,)
                return (0,)

            candidates.sort(key=ver_key, reverse=True)
            found = candidates
        else:
            home = os.path.expanduser("~")
            base = os.path.join(home, "Qt")
            if os.path.isdir(base):
                candidates = glob.glob(os.path.join(base, "6.*", "gcc_64")) + \
                             glob.glob(os.path.join(base, "6.*", "clang_64"))
                candidates = [c for c in candidates if _qt6_dir_from_prefix(c)]

                def ver_key(p):
                    parts = Path(p).parts
                    for part in parts:
                        if part.startswith("6."):
                            try:
                                return tuple(int(x) for x in part.split(".") if x.isdigit())
                            except Exception:
                                return (0,)
                    return (0,)

                candidates.sort(key=ver_key, reverse=True)
                found = candidates
    except Exception:
        pass
    return found


def _qt_prefixes_from_env(env: dict) -> list[str]:
    prefixes: list[str] = []

    # 1) Override wins
    if USER_OVERRIDES.get("qt_root"):
        return [USER_OVERRIDES["qt_root"]]  # type: ignore[return-value]

    for p in _split_paths((env.get("CMAKE_PREFIX_PATH", "") or "").replace(";", os.pathsep)):
        if _qt6_dir_from_prefix(p):
            prefixes.append(p)

    q6 = env.get("Qt6_DIR") or env.get("QT6_DIR")
    if q6 and os.path.isdir(q6):
        pp = Path(q6).resolve()
        try:
            prefix = str(pp.parents[2])  # .../lib/cmake/Qt6 -> prefix
            if _qt6_dir_from_prefix(prefix):
                prefixes.append(prefix)
        except Exception:
            pass

    # De-dupe
    seen = set()
    out = []
    for p in prefixes:
        if p not in seen:
            out.append(p)
            seen.add(p)
    return out


def _derive_eigen_include_from_prefixes(prefixes: list[str]) -> str | None:
    """
    Return an include root that makes <Eigen/Core> work.
    Accepted layouts:
      - <p>/Eigen/Core
      - <p>/include/eigen3/Eigen/Core  -> return <p>/include/eigen3
      - <p>/eigen3/Eigen/Core          -> return <p>/eigen3
    """
    for p in prefixes:
        if os.path.isfile(os.path.join(p, "Eigen", "Core")):
            return p
        inc_eigen3 = os.path.join(p, "include", "eigen3", "Eigen", "Core")
        if os.path.isfile(inc_eigen3):
            return os.path.join(p, "include", "eigen3")
        if os.path.isfile(os.path.join(p, "eigen3", "Eigen", "Core")):
            return os.path.join(p, "eigen3")
    return None


def _probe_boost_root(env: dict) -> str | None:
    """
    Try to find a usable Boost root:
      1) Explicit override (CLI/env)
      2) Respect BOOST_ROOT / Boost_ROOT if set and valid.
      3) Otherwise, look in a few common locations.
    Returns a path or None.
    """
    # 1) override
    if USER_OVERRIDES.get("boost_root"):
        return USER_OVERRIDES["boost_root"]

    import glob as _glob

    for key in ("BOOST_ROOT", "Boost_ROOT"):
        val = env.get(key)
        if val:
            header = Path(val) / "boost" / "version.hpp"
            if header.exists():
                return val
            header2 = Path(val) / "include" / "boost" / "version.hpp"
            if header2.exists():
                return val

    if platform.system() == "Windows":
        candidates = []
        for pat in (r"C:\boost_1_*", r"C:\local\boost_1_*"):
            candidates.extend(_glob.glob(pat))
        valid = [c for c in candidates if (Path(c) / "boost" / "version.hpp").exists()]
        if valid:
            def ver_key(p: str):
                base = os.path.basename(p)
                nums = [int(x) for x in base.split("_") if x.isdigit()]
                return tuple(nums) if nums else (0,)
            valid.sort(key=ver_key, reverse=True)
            return valid[0]
    else:
        for cand in ("/usr", "/usr/local"):
            if (Path(cand) / "include" / "boost" / "version.hpp").exists():
                return cand
    return None


def write_local_hints(prefixes: list[str]) -> None:
    cmake_dir = ROOT / "cmake"
    cmake_dir.mkdir(parents=True, exist_ok=True)
    hints_path = cmake_dir / "LocalDepsHints.cmake"

    ordered = []
    seen = set()
    for p in prefixes:
        sp = _normalize_to_cmake_path(p)
        if sp not in seen:
            ordered.append(sp)
            seen.add(sp)

    qt6_dir = None
    for p in ordered:
        q = _qt6_dir_from_prefix(p)
        if q:
            qt6_dir = _normalize_to_cmake_path(q)
            break

    eigen_inc = _derive_eigen_include_from_prefixes(prefixes)
    if eigen_inc:
        eigen_inc = _normalize_to_cmake_path(eigen_inc)

    boost_root = _probe_boost_root(os.environ.copy())
    if boost_root:
        boost_root = _normalize_to_cmake_path(boost_root)

    lines = []
    lines.append("# Auto-generated by scripts/build_deps.py")
    lines.append("# Do not edit by hand; this file may be regenerated.")
    lines.append("")
    if ordered:
        lines.append(f'set(CMAKE_PREFIX_PATH "{(";".join(ordered))}" CACHE PATH "" FORCE)')
    if qt6_dir:
        lines.append(f'set(Qt6_DIR "{qt6_dir}" CACHE PATH "" FORCE)')
    if eigen_inc:
        lines.append(f'set(EIGEN3_INCLUDE_DIR "{eigen_inc}" CACHE PATH "" FORCE)')
    if boost_root:
        lines.append(f'set(BOOST_ROOT "{boost_root}" CACHE PATH "" FORCE)')
    lines.append("")

    hints_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[hints] Wrote {hints_path}")


# ----------------------------
# Verification (file + compile)
# ----------------------------

def compile_check(dep: dict):
    v = dep.get("verify", {}) or {}
    cc_name, cc = choose_cxx()
    if not cc:
        raise RuntimeError("No C++ compiler found for compile_check (cl, c++, g++, or clang++).")

    cc_opts = (v.get("compile_check", {}) or {})

    inc_lines = "\n".join(cc_opts.get("include_lines", []))
    code = cc_opts.get("code", "int main(){return 0;}")
    cc_defs = list(cc_opts.get("defines", []))
    extra_libs = list((cc_opts.get("link_libs") or []))

    work = BUILD / dep["name"] / "probe"
    work.mkdir(parents=True, exist_ok=True)
    src = work / "probe.cpp"
    exe = work / ("probe.exe" if platform.system() == "Windows" else "probe")

    src.write_text(inc_lines + "\n" + code, encoding="utf-8")

    include_dir = PREFIX / "include"
    lib_dir = PREFIX / "lib"
    bin_dir = PREFIX / "bin"

    lib_base = v.get("lib_name")
    lib_files = find_lib_files(lib_base) if lib_base else []
    if lib_base and not lib_files:
        raise RuntimeError(f"compile_check: could not locate any library matching '{lib_base}' under {lib_dir}")
    link_lib = lib_files[0] if lib_files else None

    env = os.environ.copy()
    if platform.system() == "Windows":
        env = ensure_msvc_x64_env(env)
        env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")

    prefixes = get_cmake_prefix_paths(env)
    extra_lib_paths = resolve_lib_paths(extra_libs, prefixes)

    if platform.system() == "Windows":
        for p in prefixes:
            bindir = os.path.join(p, "bin")
            if os.path.isdir(bindir):
                env["PATH"] = bindir + os.pathsep + env.get("PATH", "")

    print(f"[compile-check] compiler: {cc} (name={cc_name})")
    print(f"[compile-check] source:   {src}")
    print(f"[compile-check] include:  {include_dir}")
    print(f"[compile-check] libdir:   {lib_dir}")
    if cc_defs:
        print(f"[compile-check] defines:  {', '.join(cc_defs)}")
    if link_lib:
        print(f"[compile-check] linklib:  {link_lib}")

    if cc_name == "cl":
        cc_norm = cc.replace("/", "\\").lower()
        # Best-effort check; allow if cl was found on PATH (dev shell).
        if "hostx64\\x64" not in cc_norm and "hostx64\\x86" in cc_norm:
            raise RuntimeError("MSVC cl.exe appears to be x86. Use an x64 Native Tools prompt.")

        cmd = [
            cc, "/nologo", "/EHsc",
            "/std:c++17",
            "/Zc:__cplusplus",
            "/permissive-",
        ]
        for d in cc_defs:
            cmd.append(f"/D{d}")

        cmd.append(f"/I{include_dir}")

        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=True)

        cmd += [
            str(src),
            "/link",
            f"/LIBPATH:{lib_dir}",
            "/MACHINE:X64",
        ]

        if link_lib:
            cmd.append(str(link_lib))
        for p in extra_lib_paths:
            cmd.append(p)

        cmd += [f"/OUT:{exe}"]
        run(cmd, cwd=str(work), env=env)

    else:
        cmd = [cc, "-std=c++17"]
        for d in cc_defs:
            cmd.append(f"-D{d}")

        cmd += [str(src), f"-I{include_dir}", f"-L{lib_dir}"]

        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=False)

        runtime_lib_dirs = set()
        runtime_lib_dirs.add(str(lib_dir))

        for p in prefixes:
            pr_lib = Path(p) / "lib"
            if pr_lib.is_dir():
                runtime_lib_dirs.add(str(pr_lib))

        if link_lib:
            runtime_lib_dirs.add(str(Path(link_lib).parent))
        for p in extra_lib_paths:
            runtime_lib_dirs.add(str(Path(p).parent))

        if platform.system() == "Darwin" and dep.get("name") == "SoQt":
            qt_framework_dirs = set()
            for p in prefixes:
                libpath = Path(p) / "lib"
                if (libpath / "QtCore.framework").exists():
                    qt_framework_dirs.add(str(libpath))
            for fdir in sorted(qt_framework_dirs):
                cmd.append(f"-F{fdir}")
                runtime_lib_dirs.add(fdir)
            cmd.extend(["-framework", "QtCore"])

        for rdir in sorted(runtime_lib_dirs):
            cmd.extend(["-Wl,-rpath," + rdir])

        cmd += ["-o", str(exe)]

        if link_lib:
            cmd.append(str(link_lib))
        for p in extra_lib_paths:
            cmd.append(p)

        run(cmd, cwd=str(work), env=env)

    print(f"[compile-check] run:      {exe}")

    env_run = env.copy()
    if platform.system() == "Darwin":
        ld_var = "DYLD_LIBRARY_PATH"
    else:
        ld_var = "LD_LIBRARY_PATH"

    existing = env_run.get(ld_var, "")
    rt_paths = os.pathsep.join(sorted(runtime_lib_dirs)) if "runtime_lib_dirs" in locals() else ""
    env_run[ld_var] = (rt_paths + (os.pathsep + existing if existing else "")) if rt_paths else existing

    run([str(exe)], cwd=str(work), env=env_run)
    print("[compile-check] OK")


def get_cmake_prefix_paths(env: dict) -> list[str]:
    prefixes: list[str] = [str(PREFIX)]

    raw = env.get("CMAKE_PREFIX_PATH", "") or ""
    parts = [p for p in raw.replace(";", os.pathsep).split(os.pathsep) if p]
    prefixes += parts

    # Qt prefixes (override > env > autodetect)
    detected_qt = _qt_prefixes_from_env(env)
    if not detected_qt:
        detected_qt = _detect_qt_prefixes()
    for p in detected_qt:
        if p not in prefixes:
            prefixes.append(p)

    # Eigen:
    #  1) explicit override
    if USER_OVERRIDES.get("eigen_root"):
        er = USER_OVERRIDES["eigen_root"]
        if er and er not in prefixes:
            prefixes.append(er)
    else:
        #  2) common patterns (support eigen-5.* etc)
        eigen_candidates = [
            r"C:\eigen-*",
            r"C:\eigen3",
            os.path.expanduser("~/eigen-*"),
            "/usr/include/eigen3",
            "/usr/local/include/eigen3",
        ]
        for pattern in eigen_candidates:
            for path in glob.glob(pattern):
                if not os.path.isdir(path):
                    continue
                if os.path.isfile(os.path.join(path, "Eigen", "Core")):
                    if path not in prefixes:
                        prefixes.append(path)
                    continue
                if os.path.isfile(os.path.join(path, "include", "eigen3", "Eigen", "Core")):
                    if path not in prefixes:
                        prefixes.append(path)
                    continue
                if os.path.isfile(os.path.join(path, "eigen3", "Eigen", "Core")):
                    p2 = os.path.join(path, "eigen3")
                    if p2 not in prefixes:
                        prefixes.append(p2)

    # Boost root as an extra prefix so compile-checks can see its headers
    boost_root = _probe_boost_root(env)
    if boost_root and boost_root not in prefixes:
        prefixes.append(boost_root)

    # De-dupe preserving order
    seen = set()
    ordered = []
    for p in prefixes:
        if p not in seen:
            ordered.append(p)
            seen.add(p)
    return ordered


def add_extra_includes_and_libs_from_prefixes(cmd_list: list[str],
                                             prefixes: list[str],
                                             msvc: bool) -> None:
    """
    Add reasonable -I/-L (or /I /LIBPATH:) flags based on prefixes.

    - Special-cases Qt, Eigen and Boost includes.
    - On macOS with Qt frameworks, also adds -F<libdir> and framework Headers.
    """
    includes: list[str] = []
    libpaths: list[str] = []
    framework_roots: list[str] = []

    qt_modules = ["QtCore", "QtGui", "QtWidgets", "QtOpenGL", "QtOpenGLWidgets"]
    multiarch_lib_subdirs = [
        "lib",
        "lib64",
        "lib/x86_64-linux-gnu",
        "lib/aarch64-linux-gnu",
        "lib/arm-linux-gnueabihf",
    ]

    for p in prefixes:
        if not p:
            continue

        inc_root = os.path.join(p, "include")
        lib_roots = [os.path.join(p, sub) for sub in multiarch_lib_subdirs]

        if os.path.isdir(inc_root):
            includes.append(inc_root)

            for mod in qt_modules:
                mod_inc = os.path.join(inc_root, mod)
                if os.path.isdir(mod_inc):
                    includes.append(mod_inc)

            eigen_inc = os.path.join(inc_root, "eigen3")
            if os.path.isdir(eigen_inc):
                includes.append(eigen_inc)

            if os.path.isfile(os.path.join(inc_root, "boost", "version.hpp")):
                includes.append(inc_root)

        if os.path.isfile(os.path.join(p, "Eigen", "Core")):
            includes.append(p)

        eigen3_root = os.path.join(p, "eigen3")
        if os.path.isfile(os.path.join(eigen3_root, "Eigen", "Core")):
            includes.append(eigen3_root)

        if os.path.isfile(os.path.join(p, "boost", "version.hpp")):
            includes.append(p)

        for lr in lib_roots:
            if not os.path.isdir(lr):
                continue
            libpaths.append(lr)

            if sys.platform == "darwin" and not msvc:
                try:
                    entries = os.listdir(lr)
                except PermissionError:
                    entries = []
                has_framework = False
                for name in entries:
                    if not name.endswith(".framework"):
                        continue
                    has_framework = True
                    fw_path = os.path.join(lr, name)
                    headers_dir = os.path.join(fw_path, "Headers")
                    if os.path.isdir(headers_dir):
                        includes.append(headers_dir)
                if has_framework:
                    framework_roots.append(lr)

    def _dedupe(seq: list[str]) -> list[str]:
        seen = set()
        out: list[str] = []
        for x in seq:
            if x not in seen:
                seen.add(x)
                out.append(x)
        return out

    includes = _dedupe(includes)
    libpaths = _dedupe(libpaths)
    framework_roots = _dedupe(framework_roots)

    if msvc:
        for inc in includes:
            cmd_list.append(f"/I{inc}")
        for lp in libpaths:
            cmd_list.append(f"/LIBPATH:{lp}")
    else:
        for inc in includes:
            cmd_list.append(f"-I{inc}")
        for lp in libpaths:
            cmd_list.extend(["-L", lp])
        if framework_roots:
            for fw_root in framework_roots:
                cmd_list.append(f"-F{fw_root}")


# ----------------------------
# Install verification
# ----------------------------

def verify_install(dep: dict) -> None:
    v = dep.get("verify", {}) or {}

    header = v.get("header")
    if header:
        p = PREFIX / header
        if not p.exists():
            raise RuntimeError(f"Verification failed: header not found: {p}")

    lib_spec = v.get("lib_name")
    candidates = []
    if isinstance(lib_spec, str):
        candidates = [lib_spec]
    elif isinstance(lib_spec, (list, tuple)):
        candidates = list(lib_spec)

    if candidates:
        for base in candidates:
            matches = find_lib_files(base)
            if matches:
                break
        else:
            raise RuntimeError(
                f"Verification failed: none of the libraries {candidates!r} "
                f"found under {PREFIX/'lib'}"
            )

    if v.get("compile_check"):
        compile_check(dep)


# ----------------------------
# Doctor helpers
# ----------------------------

def _read_local_hints_qt_bin() -> str | None:
    hints = ROOT / "cmake" / "LocalDepsHints.cmake"
    if not hints.exists():
        return None

    txt = hints.read_text(encoding="utf-8")

    m = re.search(r'set\(\s*Qt6_DIR\s+"([^"]+)"', txt)
    if m:
        qdir = Path(m.group(1))  # .../lib/cmake/Qt6
        qt_prefix = qdir.parent.parent
        qt_bin = qt_prefix / "bin"
        if qt_bin.exists():
            return str(qt_bin)

    m2 = re.search(r'set\(\s*CMAKE_PREFIX_PATH\s+"([^"]+)"', txt)
    if m2:
        for entry in m2.group(1).split(";"):
            p = Path(entry)
            if (p / "lib" / "cmake" / "Qt6").exists():
                cand = p / "bin"
                if cand.exists():
                    return str(cand)

    return None


def _effective_project_path(env: dict) -> str:
    sys_dirs = [r"C:\Windows\System32", r"C:\Windows"] if platform.system() == "Windows" else ["/usr/bin", "/bin"]
    entries = list(sys_dirs)

    tp_bin = str(PREFIX / "bin")
    if os.path.isdir(tp_bin):
        entries.append(tp_bin)

    qt_bin = _read_local_hints_qt_bin()
    if not qt_bin:
        if USER_OVERRIDES.get("qt_root"):
            cand = os.path.join(USER_OVERRIDES["qt_root"], "bin")  # type: ignore[arg-type]
            if os.path.isdir(cand):
                qt_bin = cand
    if qt_bin:
        entries.append(qt_bin)

    return _join_paths(entries)


def cmd_doctor():
    print("=== Environment Doctor ===")
    print(f"Platform: {platform.system()}")

    def ok(msg): print(f"  ✔ {msg}")
    def warn(msg): print(f"  ⚠ {msg}")

    git = shutil.which("git")
    if git:
        try:
            out = subprocess.check_output([git, "--version"], text=True).strip()
            ok(out)
        except Exception:
            warn("git found but failed to run --version")
    else:
        warn("git not found in PATH")

    cmake = shutil.which("cmake")
    if cmake:
        try:
            first = subprocess.check_output([cmake, "--version"], text=True).splitlines()[0].strip()
            ok(first + " (OK ≥ 3.20)")
        except Exception:
            warn("cmake found but failed to run --version")
    else:
        warn("cmake not found")

    ok(f"Python {platform.python_version()} (OK ≥ 3.9)")

    try:
        (PREFIX / ".probe").write_text("ok", encoding="utf-8")
        (PREFIX / ".probe").unlink(missing_ok=True)
        ok(f"Install prefix writable: {PREFIX}")
    except Exception:
        warn(f"Install prefix NOT writable: {PREFIX}")

    qt_bin = _read_local_hints_qt_bin()
    if platform.system() == "Windows":
        if qt_bin and os.path.isfile(os.path.join(qt_bin, "windeployqt.exe")):
            ok(f"windeployqt: {os.path.join(qt_bin, 'windeployqt.exe')}")
        else:
            wdq = shutil.which("windeployqt")
            if wdq:
                ok(f"windeployqt: {wdq}")
            else:
                warn("windeployqt not found (needed only for Windows packaging)")
    else:
        if qt_bin and os.path.isdir(qt_bin):
            ok(f"Qt bin (hints): {qt_bin}")
        else:
            warn("Qt bin not found via LocalDepsHints.cmake (ok if not generated yet)")

    if platform.system() == "Windows":
        cc = shutil.which("cl")
        if cc:
            ok(f"MSVC compiler on PATH: {cc}")
        else:
            warn("MSVC cl.exe not found on PATH (use x64 Native Tools Prompt)")

        vsdev = _find_vsdevcmd()
        if vsdev:
            ok(f"VsDevCmd.bat: {vsdev}")
        else:
            warn("VsDevCmd.bat not found (fallback mode will be used)")

        sdk = _probe_sdk_lib_paths()
        if sdk:
            ok(f"Windows SDK libs detected: {sdk[0]} …")
        else:
            warn("Windows SDK libraries not detected")

    prefixes = get_cmake_prefix_paths(os.environ.copy())
    eigen_inc = _derive_eigen_include_from_prefixes(prefixes)
    if eigen_inc:
        ok(f"Eigen include root: {eigen_inc}")
    else:
        warn("Eigen not detected in prefixes (set --eigen-root or TONATIUH_EIGEN_ROOT)")

    boost_root = _probe_boost_root(os.environ.copy())
    if boost_root:
        ok(f"Boost root: {boost_root}")
    else:
        warn("Boost root not detected (set --boost-root / TONATIUH_BOOST_ROOT)")

    found = None
    if platform.system() == "Windows":
        cand = PREFIX / "bin" / "simage1.dll"
        if cand.exists():
            found = cand
    elif platform.system() == "Darwin":
        dylibs = sorted((PREFIX / "lib").glob("libsimage*.dylib"))
        found = dylibs[0] if dylibs else None
    else:
        so_files = sorted((PREFIX / "lib").glob("libsimage.so*"))
        found = so_files[0] if so_files else None

    if found:
        ok(f"simage runtime present: {found}")
    else:
        warn("simage runtime not present (optional for image I/O)")

    global_len = len(os.environ.get("PATH", ""))
    effective = _effective_project_path(os.environ.copy())
    effective_len = len(effective)

    print(f"  Global PATH length   : {global_len} chars")
    print(f"  Effective PATH length: {effective_len} chars")

    threshold = 4096 if platform.system() == "Windows" else 8192
    if effective_len > threshold:
        warn(f"Effective PATH is long (> {threshold}). Consider trimming.")
    print("=== Doctor complete ===")


# ----------------------------
# Build step (CMake + Git)
# ----------------------------

def build_cmake_git(dep: dict, config: str = "Release", native_flags: bool = False):
    name = dep["name"]
    step_dir = BUILD / name
    src_dir = step_dir / "src"
    bld_dir = step_dir / "build"
    ok_marker = step_dir / ".ok"

    TP.mkdir(exist_ok=True)
    BUILD.mkdir(parents=True, exist_ok=True)
    PREFIX.mkdir(parents=True, exist_ok=True)
    step_dir.mkdir(parents=True, exist_ok=True)

    if not src_dir.exists():
        run(["git", "clone", "--recurse-submodules", dep["repo"], str(src_dir)])
        if dep.get("tag"):
            run(["git", "fetch", "--tags"], cwd=str(src_dir))
            run(["git", "checkout", dep["tag"]], cwd=str(src_dir))

    gen = cmake_generator()
    cmake_cmd = [
        "cmake",
        "-S", str(src_dir),
        "-B", str(bld_dir),
        f"-DCMAKE_INSTALL_PREFIX={PREFIX}",
    ] + gen + dep.get("cmake_options", [])

    env = os.environ.copy()
    if platform.system() == "Windows":
        env = ensure_msvc_x64_env(env)

    # Boost
    boost_root = _probe_boost_root(env)
    if boost_root:
        cmake_cmd.append(f"-DBOOST_ROOT={boost_root}")
        print(f"[deps] Using Boost from: {boost_root}")
    else:
        print("[deps] Warning: Boost not detected automatically.", file=sys.stderr)

    cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={config}")

    cmake_prefixes = get_cmake_prefix_paths(env)
    if cmake_prefixes:
        cmake_cmd.append(f"-DCMAKE_PREFIX_PATH={';'.join(cmake_prefixes)}")

    # Qt6_DIR (optional but helpful)
    qt6_dir_env = env.get("Qt6_DIR") or env.get("QT6_DIR")
    if not qt6_dir_env:
        if USER_OVERRIDES.get("qt_root"):
            q6 = _qt6_dir_from_prefix(USER_OVERRIDES["qt_root"])  # type: ignore[arg-type]
            if q6:
                cmake_cmd.append(f"-DQt6_DIR={q6}")
        else:
            for p in cmake_prefixes:
                q6 = _qt6_dir_from_prefix(p)
                if q6:
                    cmake_cmd.append(f"-DQt6_DIR={q6}")
                    break

    if native_flags:
        if platform.system() == "Windows":
            cmake_cmd += [
                "-DCMAKE_C_FLAGS_RELEASE=/O2 /DNDEBUG /arch:AVX2",
                "-DCMAKE_CXX_FLAGS_RELEASE=/O2 /DNDEBUG /arch:AVX2",
            ]
        else:
            cmake_cmd += [
                "-DCMAKE_C_FLAGS_RELEASE=-O3 -DNDEBUG -march=native",
                "-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -march=native",
            ]

    run(cmake_cmd, env=env)

    if platform.system() == "Windows":
        run(["cmake", "--build", str(bld_dir), "--config", config], env=env)
        run(["cmake", "--install", str(bld_dir), "--config", config], env=env)
    else:
        run(["cmake", "--build", str(bld_dir)], env=env)
        run(["cmake", "--install", str(bld_dir)], env=env)

    write_local_hints(get_cmake_prefix_paths(env))
    verify_install(dep)
    ok_marker.write_text("ok", encoding="utf-8")


# ----------------------------
# Main
# ----------------------------

def main():
    ap = argparse.ArgumentParser(description="Build third-party deps for Tonatiuh++ (incremental)")

    ap.add_argument("--config", default="Release",
                    choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
                    help="CMake configuration")

    ap.add_argument("--only", help="Build only the named dependency")
    ap.add_argument("--from", dest="from_name", help="Start from this dependency (inclusive)")
    ap.add_argument("--force", action="store_true", help="Force rebuild even if .ok exists")
    ap.add_argument("--native", action="store_true", help="Enable CPU-tuned optimizations (-march:native or /arch:AVX2)")
    ap.add_argument("--clean", action="store_true", help="Delete the dep's build directory before configuring")
    ap.add_argument("--doctor", action="store_true", help="Check env and print diagnostics")

    # NEW: explicit roots (predictable, but optional)
    ap.add_argument("--qt-root", help="Qt prefix root (e.g. C:\\Qt\\6.10.1\\msvc2022_64). Overrides auto-detect.")
    ap.add_argument("--boost-root", help="Boost root containing boost/version.hpp (e.g. C:\\boost_1_89_0).")
    ap.add_argument("--eigen-root", help="Eigen root (e.g. C:\\eigen-5.0.0 or a folder containing Eigen/Core).")

    args = ap.parse_args()

    _apply_overrides_from_env_and_args(args)

    if args.doctor:
        cmd_doctor()
        return

    manifest = TP / "deps.yaml"
    if not manifest.exists():
        print("Manifest not found at third_party/deps.yaml", file=sys.stderr)
        sys.exit(1)

    write_local_hints(get_cmake_prefix_paths(os.environ.copy()))

    spec = yaml.safe_load(manifest.read_text(encoding="utf-8")) or {}
    deps = spec.get("deps", [])
    print(f"Loaded {len(deps)} dependencies:")
    for d in deps:
        print(f" - {d.get('name','<unnamed>')}")

    if not deps:
        print("Manifest is empty.")
        return

    start = args.from_name is None
    for dep in deps:
        name = dep["name"]

        if args.only and name != args.only:
            continue

        if args.from_name and not start:
            start = (name == args.from_name)
            if not start:
                continue

        print(f"\n=== [{name}] ===")
        ok_marker = (BUILD / name / ".ok")
        if ok_marker.exists() and not args.force:
            print(f"Skipping {name}: already verified (.ok). Use --force to rebuild.")
            continue

        bld_dir = BUILD / name / "build"
        if args.clean and bld_dir.exists():
            print(f"[clean] Removing {bld_dir}")
            shutil.rmtree(bld_dir)

        kind = dep.get("kind", "cmake")

        if kind == "cmake":
            build_cmake_git(dep, config=args.config, native_flags=args.native)
            print(f"=== [{name}] OK (installed to {PREFIX}) ===")
            continue

        if kind == "check":
            print(f"[check] Verifying presence of {name} via compile-check…")
            verify_install(dep)
            step_dir = BUILD / name
            step_dir.mkdir(parents=True, exist_ok=True)
            (step_dir / ".ok").write_text("ok", encoding="utf-8")
            write_local_hints(get_cmake_prefix_paths(os.environ.copy()))
            print(f"=== [{name}] OK (presence verified) ===")
            continue

        raise SystemExit(f"Unsupported dep kind '{kind}' for {name} in this script.")

if __name__ == "__main__":
    main()
