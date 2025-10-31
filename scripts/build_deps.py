#!/usr/bin/env python3
# Tonatiuh++ dependency builder (incremental)
# - Builds dependencies defined in third_party/deps.yaml
# - Installs under third_party/_install
# - Verifies with header/lib checks and an optional compile-check
# - Skips already-verified deps via .ok markers (use --force to rebuild)
# - Windows: forces x64 generator and ensures MSVC x64 env (via VsDevCmd if available,
#             otherwise by deriving VC/SDK lib/include paths)
# - Optional --native for CPU-tuned optimizations
# - NEW: Writes cmake/LocalDepsHints.cmake with discovered prefixes (Qt, Eigen, _install)

import argparse
import fnmatch
import os
import platform
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

# ----------------------------
# Utility helpers
# ----------------------------

def run(cmd, cwd=None, env=None):
    """Run a command with logging and error propagation."""
    print("$", " ".join(map(str, cmd)))
    subprocess.check_call(cmd, cwd=cwd, env=env or os.environ.copy())

def cmake_generator():
    r"""
    Choose a generator:
    - Windows: force Visual Studio 2022 x64 (avoid x86)
    - Elsewhere: prefer Ninja if available
    """
    if platform.system() == "Windows":
        return ["-G", "Visual Studio 17 2022", "-A", "x64"]
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
    patterns = []
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

def choose_cxx():
    r"""
    Pick a C++ compiler that matches our target.
    - Windows: prefer 64-bit cl.exe explicitly if available; fallback to cl on PATH.
    - Others: try c++/g++/clang++ in that order.
    """
    if platform.system() == "Windows":
        import glob
        patterns = [
            r"C:\Program Files\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
            r"C:\Program Files (x86)\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe",
        ]
        for pat in patterns:
            for p in sorted(glob.glob(pat), reverse=True):
                if os.path.exists(p):
                    return ("cl", p)
        cl = shutil.which("cl")
        if cl:
            return ("cl", cl)
    # Non-Windows
    for c in ["c++", "g++", "clang++"]:
        p = shutil.which(c)
        if p:
            return (c, p)
    return (None, None)

# ---------- Windows env discovery & normalization ----------

def _find_vsdevcmd():
    """Locate VsDevCmd.bat via vswhere or common paths. Return path or None."""
    if platform.system() != "Windows":
        return None
    vswhere = r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if os.path.exists(vswhere):
        try:
            out = subprocess.check_output(
                [vswhere, "-latest", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                 "-property", "installationPath"],
                text=True
            ).strip()
            if out:
                cand = os.path.join(out, "Common7", "Tools", "VsDevCmd.bat")
                if os.path.exists(cand):
                    return cand
        except Exception:
            pass
    candidates = [
        r"C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\Tools\VsDevCmd.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat",
    ]
    for p in candidates:
        if os.path.exists(p):
            return p
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
            um   = os.path.join(base, "um", "x64")
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
    this function ensures we’re targeting the 64-bit compiler environment.
    """
    vc_tools = Path(cl_path).resolve().parents[3]  # ...\VC\Tools\MSVC\<ver>
    vc_include = vc_tools / "include"
    vc_lib_x64 = vc_tools / "lib" / "x64"
    inc = [str(vc_include)] if vc_include.exists() else []
    lib = [str(vc_lib_x64)] if vc_lib_x64.exists() else []
    return inc, lib

def load_msvc_env_x64(env_in: dict) -> dict:
    """
    Load VS (MSVC) x64 dev environment by invoking VsDevCmd.bat -arch=x64 (if available),
    capture its environment via `set`, and merge into env_in. Otherwise return env_in unchanged.
    """
    if platform.system() != "Windows":
        return env_in.copy()
    vsdev = _find_vsdevcmd()
    if not vsdev:
        return env_in.copy()
    cmd = ['cmd.exe', '/s', '/c', f'"{vsdev}" -arch:x64 >nul && set']
    try:
        out = subprocess.check_output(cmd, shell=False, text=True)
    except Exception:
        return env_in.copy()
    new_env = env_in.copy()
    for line in out.splitlines():
        if '=' in line:
            k, v = line.split('=', 1)
            new_env[k.upper()] = v
    return new_env

def ensure_msvc_x64_env(env_in: dict) -> dict:
    r"""
    Ensure MSVC environment variables point to x64 toolchain/libs.
    Strategy:
      1) Try VsDevCmd -arch:x64 (if available).
      2) Always sanitize LIB/PATH/INCLUDE to prefer x64 and drop x86.
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
        # (we drop x86 entries entirely)
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
        # Prepend derived paths if not already present
        if vc_lib:
            current_lib = _split_paths(env.get("LIB", ""))
            env["LIB"] = _join_paths(list(dict.fromkeys(vc_lib + current_lib)))
        if sdk_lib:
            current_lib = _split_paths(env.get("LIB", ""))
            env["LIB"] = _join_paths(list(dict.fromkeys(sdk_lib + current_lib)))
        if vc_inc:
            current_inc = _split_paths(env.get("INCLUDE", ""))
            env["INCLUDE"] = _join_paths(list(dict.fromkeys(vc_inc + current_inc)))

    return env

def resolve_lib_paths(lib_basenames: list[str], prefixes: list[str]) -> list[str]:
    """
    Given library base names like ["Qt6Core","Qt6Gui"], return absolute paths
    to library files by searching <prefix>/lib across all prefixes.
    Works on Windows (.lib), macOS (.dylib), Linux (.so*).
    """
    exts_win   = [".lib", ".dll"]         # prefer .lib for link
    exts_macos = [".dylib", ".a"]
    exts_lin   = [".so", ".so.6", ".a"]

    is_win  = platform.system() == "Windows"
    is_mac  = platform.system() == "Darwin"
    exts    = exts_win if is_win else (exts_macos if is_mac else exts_lin)

    results = []
    for base in lib_basenames or []:
        found = None
        for p in prefixes:
            libdir = os.path.join(p, "lib")
            if not os.path.isdir(libdir):
                continue
            # try exact filenames in order of preference
            candidates = []
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
            results.append(found)
    return results

# ----------------------------
# CMake hints (NEW)
# ----------------------------

def _normalize_to_cmake_path(p: str) -> str:
    # Forward slashes are friendlier in CMake even on Windows
    return p.replace("\\", "/")

def _qt6_dir_from_prefix(prefix: str) -> str | None:
    r"""
    Given a Qt prefix (e.g., C:\Qt\6.9.3\msvc2022_64 or ~/Qt/6.7.2/gcc_64),
    return the Qt6_DIR (prefix/lib/cmake/Qt6) if it exists.
    """
    qt6_dir = os.path.join(prefix, "lib", "cmake", "Qt6")
    return qt6_dir if os.path.isdir(qt6_dir) else None

def _detect_qt_prefixes() -> list[str]:
    r"""
    Auto-discover Qt6 prefixes without any env vars.
    - Windows: scan C:\Qt\6.*\msvc*\_64
    - Linux/macOS: scan ~/Qt/6.*/* (gcc_64 or clang_64)
    Returns list of candidate prefixes; newest version first when possible.
    """
    found: list[str] = []
    try:
        import glob
        if platform.system() == "Windows":
            roots = [r"C:\Qt"]
            patterns = []
            for root in roots:
                if os.path.isdir(root):
                    # Typical layout: C:\Qt\6.9.3\msvc2022_64
                    patterns += [os.path.join(root, "6.*", "msvc*_*")]
            candidates = []
            for pat in patterns:
                candidates += glob.glob(pat)
            candidates = [c for c in candidates if _qt6_dir_from_prefix(c)]
            # Sort by version descending
            def ver_key(p):
                parts = Path(p).parts
                for part in parts:
                    if part.startswith("6."):
                        return tuple(int(x) for x in part.split(".") if x.isdigit())
                return (0,)
            candidates.sort(key=ver_key, reverse=True)
            found = candidates

        else:
            # Linux/macOS typical: ~/Qt/6.x.y/gcc_64 or clang_64
            home = os.path.expanduser("~")
            base = os.path.join(home, "Qt")
            if os.path.isdir(base):
                import glob as _glob
                candidates = _glob.glob(os.path.join(base, "6.*", "gcc_64")) + \
                             _glob.glob(os.path.join(base, "6.*", "clang_64"))
                candidates = [c for c in candidates if _qt6_dir_from_prefix(c)]
                def ver_key(p):
                    parts = Path(p).parts
                    for part in parts:
                        if part.startswith("6."):
                            return tuple(int(x) for x in part.split(".") if x.isdigit())
                    return (0,)
                candidates.sort(key=ver_key, reverse=True)
                found = candidates
    except Exception:
        pass
    return found

def _qt_prefixes_from_env(env: dict) -> list[str]:
    r"""
    Extract Qt prefixes from environment, if provided:
      - CMAKE_PREFIX_PATH entries that contain lib/cmake/Qt6
      - Qt6_DIR itself (convert to a prefix)
    """
    prefixes: list[str] = []
    # From CMAKE_PREFIX_PATH
    for p in _split_paths(env.get("CMAKE_PREFIX_PATH", "").replace(";", os.pathsep)):
        if _qt6_dir_from_prefix(p):
            prefixes.append(p)
    # From Qt6_DIR
    q6 = env.get("Qt6_DIR") or env.get("QT6_DIR")
    if q6 and os.path.isdir(q6):
        pp = Path(q6).resolve()
        try:
            prefix = str(pp.parents[2])  # .../lib/cmake/Qt6 -> prefix
            if _qt6_dir_from_prefix(prefix):
                prefixes.append(prefix)
        except Exception:
            pass
    # Dedup preserve order
    seen = set()
    out = []
    for p in prefixes:
        if p not in seen:
            out.append(p)
            seen.add(p)
    return out

def _derive_eigen_include_from_prefixes(prefixes: list[str]) -> str | None:
    """
    From a list of prefixes, try to find an Eigen include root:
      - <prefix>/include/eigen3, or
      - <prefix> itself if it contains Eigen/Core (unzipped tarball style), or
      - <prefix>/eigen3 if that contains Eigen/Core.
    Return the include directory to add, or None.
    """
    for p in prefixes:
        # package layout: <p>/include/eigen3/Eigen/Core
        inc_eigen3 = os.path.join(p, "include", "eigen3", "Eigen", "Core")
        if os.path.exists(inc_eigen3):
            return os.path.dirname(os.path.dirname(inc_eigen3))  # .../include/eigen3

        # unzipped layout: <p>/Eigen/Core
        if os.path.exists(os.path.join(p, "Eigen", "Core")):
            return p

        # nested eigen3: <p>/eigen3/Eigen/Core
        if os.path.exists(os.path.join(p, "eigen3", "Eigen", "Core")):
            return os.path.join(p, "eigen3")
    return None

def write_local_hints(prefixes: list[str]) -> None:
    """
    Write (or update) cmake/LocalDepsHints.cmake to help the project's CMake find deps
    without users having to pass any -D variables. This file is intentionally idempotent
    and safe to regenerate after each dep build.
    """
    cmake_dir = ROOT / "cmake"
    cmake_dir.mkdir(parents=True, exist_ok=True)
    hints_path = cmake_dir / "LocalDepsHints.cmake"

    # Compose CMAKE_PREFIX_PATH starting with our local _install prefix
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

    lines = []
    lines.append("# Auto-generated by scripts/build_deps.py")
    lines.append("# Do not edit by hand; this file may be regenerated.")
    lines.append("")
    if ordered:
        lines.append(f'set(CMAKE_PREFIX_PATH "{(";".join(ordered))}" CACHE PATH "" FORCE)')
    if qt6_dir:
        lines.append(f'set(Qt6_DIR "{qt6_dir}" CACHE PATH "" FORCE)')
    if eigen_inc:
        # Helps FindEigen3 that respects EIGEN3_INCLUDE_DIR
        lines.append(f'set(EIGEN3_INCLUDE_DIR "{eigen_inc}" CACHE PATH "" FORCE)')
    lines.append("")

    hints_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"[hints] Wrote {hints_path}")

# ----------------------------
# Verification (file + compile)
# ----------------------------

def compile_check(dep: dict):
    """Compile and run a tiny probe that includes headers and links the dep's lib."""
    v = dep.get("verify", {}) or {}
    cc_name, cc = choose_cxx()
    if not cc:
        raise RuntimeError("No C++ compiler found for compile_check (cl, c++, g++, or clang++).")

    cc_opts   = (v.get("compile_check", {}) or {})

    inc_lines = "\n".join(cc_opts.get("include_lines", []))
    code      = cc_opts.get("code", "int main(){return 0;}")
    cc_defs   = list(cc_opts.get("defines", []))  # e.g., ["SOQT_DLL"]
    extra_libs = list((cc_opts.get("link_libs") or []))  # e.g., ["Qt6Core","Qt6Gui","Qt6Widgets"]

    work = BUILD / dep["name"] / "probe"
    work.mkdir(parents=True, exist_ok=True)
    src = work / "probe.cpp"
    exe = work / ("probe.exe" if platform.system() == "Windows" else "probe")

    src.write_text(inc_lines + "\n" + code, encoding="utf-8")

    include_dir = PREFIX / "include"
    lib_dir     = PREFIX / "lib"
    bin_dir     = PREFIX / "bin"  # runtime (Windows DLLs)

    # Choose a concrete library file to link (robust naming)
    lib_base  = v.get("lib_name")
    lib_files = find_lib_files(lib_base) if lib_base else []
    if lib_base and not lib_files:
        raise RuntimeError(f"compile_check: could not locate any library matching '{lib_base}' under {lib_dir}")
    link_lib = lib_files[0] if lib_files else None

    # Prepare environment and ensure MSVC x64 (on Windows)
    env = os.environ.copy()
    if platform.system() == "Windows":
        env = ensure_msvc_x64_env(env)  # normalize to x64 CRT/toolchain
        # Ensure runtime can find DLLs (e.g., Coin4.dll, SoQt*.dll, Qt6*.dll)
        env["PATH"] = str(bin_dir) + os.pathsep + env.get("PATH", "")

    # Resolve CMAKE_PREFIX_PATHs (Qt + our local prefix) for probe includes/libs
    prefixes = get_cmake_prefix_paths(env)
    extra_lib_paths = resolve_lib_paths(extra_libs, prefixes)

    # Ensure Qt (and other) DLLs are visible at runtime on Windows
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
        # Guard: ensure we really have the x64 toolchain binary
        if "Hostx64\\x64" not in cc.replace("/", "\\"):
            raise RuntimeError("MSVC cl.exe is not x64. Please install VS 2022 Build Tools and try again.")

        # MSVC compile + link
        cmd = [
            cc, "/nologo", "/EHsc",
            "/std:c++17",            # C++17
            "/Zc:__cplusplus",       # proper __cplusplus value
            "/permissive-"           # needed for Qt/MSVC
        ]
        # Preprocessor defines (/DNAME or /DNAME=VALUE)
        for d in cc_defs:
            cmd.append(f"/D{d}")

        # Base includes + source
        cmd += [
            f"/I{include_dir}",
            str(src),
            "/link",
            f"/LIBPATH:{lib_dir}",
            "/MACHINE:X64",
        ]

        # Add includes/libs from CMAKE_PREFIX_PATH (Qt + our local prefix)
        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=True)

        if link_lib:
            cmd.append(str(link_lib))
        for p in extra_lib_paths:
            cmd.append(p)
        cmd += [f"/OUT:{exe}"]
        run(cmd, cwd=str(work), env=env)

    else:
        # GCC/Clang style
        cmd = [
            cc,
            "-std=c++17",            # C++17
        ]

        # Preprocessor defines (-DNAME or -DNAME=VALUE)
        for d in cc_defs:
            cmd.append(f"-D{d}")

        # Base includes + source + output
        cmd += [
            str(src),
            f"-I{include_dir}",
            f"-L{lib_dir}",
        ]

        # Add includes/libs from CMAKE_PREFIX_PATH (Qt + our local prefix)
        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=False)

        cmd += ["-o", str(exe)]

        if link_lib:
            # Absolute path to avoid guessing -l flags
            cmd.append(str(link_lib))
        for p in extra_lib_paths:
            cmd.append(p)
        run(cmd, cwd=str(work), env=env)

    # Run the probe to ensure runtime linkage is OK
    print(f"[compile-check] run:      {exe}")
    run([str(exe)], cwd=str(work), env=env)
    print("[compile-check] OK")

def get_cmake_prefix_paths(env: dict) -> list[str]:
    r"""
    Build an ordered list of CMake prefixes:
      1) Our local install prefix (always first)
      2) Any existing entries from env:CMAKE_PREFIX_PATH
      3) Any Qt6 prefixes discovered on this machine
      4) Common Eigen locations (both header-only root and include/eigen3 layouts)

    Note: This function only returns prefixes. The compile/link helpers are expected
    to derive include/lib subpaths from each prefix. For Eigen, we may return either:
      - a prefix whose include root is <prefix>/include and headers live in <prefix>/include/eigen3, or
      - a prefix that IS the Eigen header root itself (i.e., contains 'Eigen/Core').
    """
    import glob
    # 1) our local prefix
    prefixes: list[str] = [str(PREFIX)]

    # 2) any existing env-specified prefixes
    raw = env.get("CMAKE_PREFIX_PATH", "") or ""
    parts = [p for p in raw.replace(";", os.pathsep).split(os.pathsep) if p]
    prefixes += parts

    # 3) auto-detected Qt6 prefixes (newest first)
    detected_qt = _qt_prefixes_from_env(env)
    if not detected_qt:
        detected_qt = _detect_qt_prefixes()
    for p in detected_qt:
        if p not in prefixes:
            prefixes.append(p)

    # 4) Eigen common locations
    #    We support both:
    #      - unpacked roots like C:\eigen-3.4.1 (contains Eigen/Core)
    #      - prefix/include/eigen3 layouts (system or package-managed)
    eigen_candidates = [
        r"C:\eigen-3.*",                 # typical manual unzip on Windows
        r"C:\eigen3",
        os.path.expanduser("~/eigen-3.*"),
        "/usr/include/eigen3",
        "/usr/local/include/eigen3",
    ]
    for pattern in eigen_candidates:
        for path in glob.glob(pattern):
            if not os.path.isdir(path):
                continue
            # Case A: the path itself is the Eigen header root (has Eigen/Core)
            if os.path.exists(os.path.join(path, "Eigen", "Core")):
                if path not in prefixes:
                    prefixes.append(path)
                continue
            # Case B: '.../include/eigen3/Eigen/Core'
            inc_eigen3 = os.path.join(path, "include", "eigen3", "Eigen", "Core")
            if os.path.exists(inc_eigen3):
                if path not in prefixes:
                    prefixes.append(path)
                continue
            # Case C: '.../eigen3/Eigen/Core' (sometimes the prefix points to a parent)
            eigen3_root = os.path.join(path, "eigen3")
            if os.path.exists(os.path.join(eigen3_root, "Eigen", "Core")):
                if eigen3_root not in prefixes:
                    prefixes.append(eigen3_root)

    # Deduplicate while preserving order
    seen = set()
    ordered = []
    for p in prefixes:
        if p not in seen:
            ordered.append(p)
            seen.add(p)
    return ordered

def add_extra_includes_and_libs_from_prefixes(cmd_list: list[str], prefixes: list[str], msvc: bool):
    r"""
    Inject include/lib paths from each prefix.

    Adds:
      - Generic include/lib roots (<prefix>/include, <prefix>/lib)
      - Qt6 module include dirs (QtCore, QtGui, QtWidgets, QtOpenGL, QtOpenGLWidgets)
      - Eigen include roots for both layouts:
          * <prefix>/include/eigen3
          * <prefix> (when it directly contains Eigen/Core, e.g. C:\eigen-3.4.1)

    For MSVC: ensure /I flags appear BEFORE '/link', and /LIBPATH: AFTER '/link'.
    """
    import os

    includes: list[str] = []
    libpaths: list[str] = []

    qt_modules = ["QtCore", "QtGui", "QtWidgets", "QtOpenGL", "QtOpenGLWidgets"]

    for p in prefixes:
        inc_root = os.path.join(p, "include")
        lib_root = os.path.join(p, "lib")

        # --- Include roots ---
        if os.path.isdir(inc_root):
            includes.append(inc_root)

            # Qt6 module includes if this looks like a Qt prefix
            if os.path.isdir(os.path.join(p, "lib", "cmake", "Qt6")):
                for mod in qt_modules:
                    mod_inc = os.path.join(inc_root, mod)
                    if os.path.isdir(mod_inc):
                        includes.append(mod_inc)

            # Eigen (package layout): <prefix>/include/eigen3
            eigen_inc = os.path.join(inc_root, "eigen3")
            if os.path.isdir(eigen_inc):
                includes.append(eigen_inc)

        # Eigen (unzipped layout): prefix itself contains Eigen/Core
        if os.path.isdir(os.path.join(p, "Eigen")) and os.path.isfile(os.path.join(p, "Eigen", "Core")):
            includes.append(p)

        # Also handle when prefix already *is* the eigen3 root (…/eigen3/Eigen/Core)
        if os.path.isdir(os.path.join(p, "eigen3", "Eigen")) and os.path.isfile(os.path.join(p, "eigen3", "Eigen", "Core")):
            includes.append(os.path.join(p, "eigen3"))

        # --- Library roots ---
        if os.path.isdir(lib_root):
            libpaths.append(lib_root)

    # Deduplicate while preserving order
    def _dedup(seq: list[str]) -> list[str]:
        seen = set()
        out = []
        for x in seq:
            if x not in seen:
                out.append(x)
                seen.add(x)
        return out

    includes = _dedup(includes)
    libpaths = _dedup(libpaths)

    if msvc:
        # Ensure includes come before '/link'
        try:
            link_idx = cmd_list.index("/link")
        except ValueError:
            link_idx = len(cmd_list)
            cmd_list.append("/link")

        for inc in includes:
            cmd_list.insert(link_idx, f"/I{inc}")
            link_idx += 1

        for lp in libpaths:
            cmd_list += [f"/LIBPATH:{lp}"]
    else:
        for inc in includes:
            cmd_list.append(f"-I{inc}")
        for lp in libpaths:
            cmd_list.append(f"-L{lp}")

def _split_paths(val: str):
    return [p for p in (val or "").split(os.pathsep) if p]

def _join_paths(paths):
    return os.pathsep.join(paths)

# ----------------------------
# Install verification
# ----------------------------

def verify_install(dep: dict) -> None:
    r"""
    Basic install checks plus optional compile-check.
    Expects the dep block to optionally define:
      verify:
        header: <relative path under PREFIX>   e.g. include/Inventor/SoDB.h
        lib_name: <basename to search in PREFIX/lib>  e.g. Coin or SoQt
        compile_check:
          include_lines: [ "#include <...>", ... ]
          code: | <tiny main() that touches a symbol>
    """
    v = dep.get("verify", {}) or {}

    # Check header
    header = v.get("header")
    if header:
        p = PREFIX / header
        if not p.exists():
            raise RuntimeError(f"Verification failed: header not found: {p}")

    # Check library presence (accepts multiple file name variants)
    lib_base = v.get("lib_name")
    if lib_base:
        matches = find_lib_files(lib_base)
        if not matches:
            raise RuntimeError(f"Verification failed: library '{lib_base}' not found under {PREFIX/'lib'}")

    # Optional: compile a small probe that includes headers and links the lib(s)
    if v.get("compile_check"):
        compile_check(dep)

# ----------------------------
# Build step (CMake + Git)
# ----------------------------

def build_cmake_git(dep: dict, config: str = "Release", native_flags: bool = False):
    r"""
    Build a CMake project from a Git repo:
      third_party/_build/<name>/src   (repo)
      third_party/_build/<name>/build (build dir)
      third_party/_install            (install prefix)
    """
    name = dep["name"]
    step_dir = BUILD / name
    src_dir  = step_dir / "src"
    bld_dir  = step_dir / "build"
    ok_marker = step_dir / ".ok"

    # Prepare dirs
    TP.mkdir(exist_ok=True)
    BUILD.mkdir(parents=True, exist_ok=True)
    PREFIX.mkdir(parents=True, exist_ok=True)
    step_dir.mkdir(parents=True, exist_ok=True)

    # Fetch (with submodules)
    if not src_dir.exists():
        clone_cmd = ["git", "clone", "--recurse-submodules", dep["repo"], str(src_dir)]
        run(clone_cmd)
        if dep.get("tag"):
            run(["git", "fetch", "--tags"], cwd=str(src_dir))
            run(["git", "checkout", dep["tag"]], cwd=str(src_dir))

    # Base CMake command + generator
    gen = cmake_generator()
    cmake_cmd = [
        "cmake",
        "-S", str(src_dir),
        "-B", str(bld_dir),
        f"-DCMAKE_INSTALL_PREFIX={PREFIX}",
    ] + gen + dep.get("cmake_options", [])

    # Environment (Windows: ensure x64 MSVC CRT/SDK paths)
    env = os.environ.copy()
    if platform.system() == "Windows":
        env = ensure_msvc_x64_env(env)
        # Forward BOOST_ROOT on Windows if set (Coin needs it on Win)
        boost_root = env.get("BOOST_ROOT") or env.get("Boost_ROOT")
        if boost_root:
            cmake_cmd.append(f"-DBOOST_ROOT={boost_root}")

    # Ensure CMake can find our local prefix and any user-provided prefixes (e.g., Qt)
    raw_cpp = env.get("CMAKE_PREFIX_PATH", "") or ""
    extra_prefixes = [p for p in raw_cpp.replace(";", os.pathsep).split(os.pathsep) if p]
    cmake_prefixes = [str(PREFIX)] + extra_prefixes

    # Ensure CMake sees our local prefix + any detected Qt/Eigen prefixes
    cmake_prefixes = get_cmake_prefix_paths(env)
    if cmake_prefixes:
        cmake_cmd.append(f"-DCMAKE_PREFIX_PATH={';'.join(cmake_prefixes)}")

    # If Qt6_DIR is not set, derive it from the first detected Qt prefix
    qt6_dir_env = env.get("Qt6_DIR") or env.get("QT6_DIR")
    if not qt6_dir_env:
        for p in cmake_prefixes:
            q6 = _qt6_dir_from_prefix(p)
            if q6:
                cmake_cmd.append(f"-DQt6_DIR={q6}")
                break

    # Optional native-optimized release flags
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

    # Configure
    run(cmake_cmd, env=env)

    # Build & install
    if platform.system() == "Windows":
        run(["cmake", "--build", str(bld_dir), "--config", config], env=env)
        run(["cmake", "--install", str(bld_dir), "--config", config], env=env)
    else:
        run(["cmake", "--build", str(bld_dir)], env=env)
        run(["cmake", "--install", str(bld_dir)], env=env)

    # After a successful install, (re)write hints so main project can find deps
    write_local_hints(get_cmake_prefix_paths(env))

    # Verify
    verify_install(dep)

    # Mark success
    ok_marker.write_text("ok", encoding="utf-8")

# ----------------------------
# Main
# ----------------------------

def main():
    ap = argparse.ArgumentParser(
        description="Build third-party deps for Tonatiuh++ (incremental)"
    )
    ap.add_argument("--config", default="Release",
                    choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
                    help="CMake configuration")
    ap.add_argument("--only", help="Build only the named dependency")
    ap.add_argument("--from", dest="from_name",
                    help="Start from this dependency (inclusive)")
    ap.add_argument("--force", action="store_true",
                    help="Force rebuild even if .ok exists")
    ap.add_argument("--native", action="store_true",
                    help="Enable CPU-tuned optimizations (-march=native or /arch:AVX2)")
    ap.add_argument("--clean", action="store_true",
                    help="Delete the dep's build directory before configuring")
    args = ap.parse_args()

    manifest = TP / "deps.yaml"
    if not manifest.exists():
        print("Manifest not found at third_party/deps.yaml", file=sys.stderr)
        sys.exit(1)

    # Write initial hints once per run, in case user wants to configure the main project now.
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

        # Optional clean
        bld_dir = BUILD / name / "build"
        if args.clean and bld_dir.exists():
            print(f"[clean] Removing {bld_dir}")
            shutil.rmtree(bld_dir)

        kind = dep.get("kind", "cmake")

        if kind == "cmake":
            build_cmake_git(dep, config=args.config, native_flags=args.native)
            print(f"=== [{name}] OK (installed to {PREFIX}) ===")
            continue

        elif kind == "check":
            # Presence-only dependency (e.g., Eigen): just verify via compile-check
            print(f"[check] Verifying presence of {name} via compile-check…")
            verify_install(dep)  # runs compile_check(dep) if specified
            step_dir = BUILD / name
            step_dir.mkdir(parents=True, exist_ok=True)
            (step_dir / ".ok").write_text("ok", encoding="utf-8")
            # Update hints so the main project can find Eigen too
            write_local_hints(get_cmake_prefix_paths(os.environ.copy()))
            print(f"=== [{name}] OK (presence verified) ===")
            continue

        else:
            raise SystemExit(f"Unsupported dep kind '{kind}' for {name} in this script.")

if __name__ == "__main__":
    main()