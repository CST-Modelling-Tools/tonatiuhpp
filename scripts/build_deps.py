#!/usr/bin/env python3
# Tonatiuh++ dependency builder (incremental)
#
# Key fixes included:
# - Eigen detection is now robust on macOS (Homebrew /opt/homebrew include roots, brew --prefix eigen).
# - CMAKE_PREFIX_PATH is no longer polluted with raw include directories (e.g. /usr/include/eigen3).
# - LocalDepsHints.cmake writes EIGEN3_INCLUDE_DIR separately and keeps CMAKE_PREFIX_PATH as real prefixes only.
# - get_cmake_prefix_paths now adds sane system prefixes across OSes (/usr, /usr/local, /opt/homebrew) without
#   forcing a specific Ubuntu version.
#
# Additional CI hardening:
# - Per-OS sanitization of CMake options (e.g. disable SIMAGE_USE_GDIPLUS on non-Windows).
# - Never pass empty -DEigen3_DIR= (can confuse CMake).
# - Probe compile-check: strip Windows-only defines (SOQT_DLL, SIMAGE_DLL) on non-Windows.

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
    env = env or os.environ.copy()
    try:
        # Capture output so we can print it on failure (GitHub Actions otherwise hides it sometimes)
        p = subprocess.run(
            cmd,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=True,
        )
        if p.stdout:
            print(p.stdout, end="" if p.stdout.endswith("\n") else "\n")
    except subprocess.CalledProcessError as e:
        if e.stdout:
            print(e.stdout, end="" if e.stdout.endswith("\n") else "\n")
        raise

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

def has_system_simage() -> bool:
    """
    Return True if a usable system simage is available.
    We prefer pkg-config, but also accept header presence as fallback.
    """
    import shutil
    import subprocess
    from pathlib import Path

    pc = shutil.which("pkg-config")
    if pc:
        for name in ("simage", "simage1", "simage-1"):
            r = subprocess.run([pc, "--exists", name])
            if r.returncode == 0:
                return True

    # Fallback: headers present (covers some distros/layouts)
    return (
        Path("/usr/include/simage/simage.h").exists()
        or Path("/usr/include/simage.h").exists()
    )

def system_simage_version() -> str | None:
    import shutil
    import subprocess

    pc = shutil.which("pkg-config")
    if not pc:
        return None

    for name in ("simage", "simage1", "simage-1"):
        r = subprocess.run([pc, "--modversion", name], capture_output=True, text=True)
        if r.returncode == 0:
            v = (r.stdout or "").strip()
            return v or None

    return None

def _linux_multiarch_triplet() -> str | None:
    if platform.system() != "Linux":
        return None
    try:
        if shutil.which("dpkg-architecture"):
            return subprocess.check_output(
                ["dpkg-architecture", "-qDEB_HOST_MULTIARCH"], text=True
            ).strip() or None
    except Exception:
        pass
    return None


def _linux_qt6_include_roots() -> list[str]:
    """
    Return likely Qt6 include roots on Ubuntu/Debian:
      - /usr/include/<triplet>/qt6
      - /usr/include/qt6
    """
    roots: list[str] = []
    trip = _linux_multiarch_triplet()
    if trip:
        roots.append(f"/usr/include/{trip}/qt6")
    roots.append("/usr/include/qt6")
    return [r for r in roots if os.path.isdir(r)]


def _linux_has_system_qt6() -> bool:
    """
    Detect if Qt6 is installed via system packages (CMake config present).
    Used only to decide whether to add Qt include dirs during probe builds.
    """
    if platform.system() != "Linux":
        return False
    candidates = [
        "/usr/lib/x86_64-linux-gnu/cmake/Qt6",
        "/usr/lib/aarch64-linux-gnu/cmake/Qt6",
        "/usr/lib/cmake/Qt6",
        "/usr/lib64/cmake/Qt6",
        "/usr/lib/qt6/lib/cmake/Qt6",
    ]
    return any(os.path.isdir(p) for p in candidates)


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
            r"        Expected something like: C:\Qt\6.x.x\msvc2022_64"
        )
    return qt_root

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

def _validate_boost_root(boost_root: str) -> str:
    boost_root = _norm(boost_root)
    if not os.path.isdir(boost_root):
        raise SystemExit(f"[error] --boost-root points to a non-existing directory: {boost_root}")

    def has_any_hpp(dirpath: str) -> bool:
        try:
            for root, _, files in os.walk(dirpath):
                for f in files:
                    if f.endswith(".hpp") or f.endswith(".h"):
                        return True
        except OSError:
            return False
        return False

    candidates = [
        os.path.join(boost_root, "boost"),
        os.path.join(boost_root, "include", "boost"),
    ]

    for d in candidates:
        if os.path.isdir(d) and has_any_hpp(d):
            return boost_root

    # Helpful diagnostics (don’t recurse too much)
    tried = "\n        ".join(candidates)
    raise SystemExit(
        "[error] --boost-root does not appear to contain Boost headers.\n"
        "        Expected a 'boost' include directory with at least one header file.\n"
        f"        Tried:\n        {tried}"
    )


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
        cl = shutil.which("cl")
        if cl:
            return ("cl", cl)

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
    versions = sorted(
        [d for d in os.listdir(vc_tools_root) if os.path.isdir(os.path.join(vc_tools_root, d))],
        reverse=True
    )
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
    """Try to locate Windows 10/11 SDK lib dirs (ucrt/um x64). Return list of LIB paths (may be empty)."""
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
    """Load VS (MSVC) x64 dev environment via VsDevCmd if available."""
    if platform.system() != "Windows":
        return env_in.copy()

    vsdev = _find_vsdevcmd()
    if not vsdev:
        return env_in.copy()

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
    """Ensure MSVC environment points to x64 toolchain/libs."""
    env = env_in.copy()
    if platform.system() != "Windows":
        return env

    env = load_msvc_env_x64(env)

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

def _qt6_to_framework_name(lib: str) -> str | None:
    # Qt6Core -> QtCore, Qt6OpenGLWidgets -> QtOpenGLWidgets, etc.
    if not lib.startswith("Qt6"):
        return None
    return "Qt" + lib[len("Qt6") :]

def _find_qt_framework_dirs(prefixes: list[str]) -> list[str]:
    out = []
    for p in prefixes:
        libdir = Path(p) / "lib"
        if (libdir / "QtCore.framework").exists():
            out.append(str(libdir))
    # de-dupe
    seen = set()
    res = []
    for x in out:
        if x not in seen:
            res.append(x)
            seen.add(x)
    return res

def resolve_lib_paths(lib_basenames: list[str], prefixes: list[str]) -> list[str]:
    """
    Given library base names like ["Qt6Core","Qt6Gui"], return absolute paths
    to library files by searching <prefix>/{lib,lib64,lib/<multiarch>} across prefixes.
    """
    is_win = platform.system() == "Windows"
    is_mac = platform.system() == "Darwin"

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
                    candidates = [os.path.join(libdir, f"{base}.lib"), os.path.join(libdir, f"{base}.dll")]
                elif is_mac:
                    candidates = [
                        os.path.join(libdir, f"lib{base}.dylib"),
                        os.path.join(libdir, f"lib{base}.a"),
                        os.path.join(libdir, f"{base}.framework", "Versions", "A", base),
                        os.path.join(libdir, f"{base}.framework", base),
                    ]
                else:
                    candidates = [
                        os.path.join(libdir, f"lib{base}.so"),
                        os.path.join(libdir, f"lib{base}.a"),
                    ]
                    candidates.extend(glob.glob(os.path.join(libdir, f"lib{base}.so.*")))

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
# Eigen detection (robust)
# ----------------------------

def _brew_prefix(pkg: str) -> str | None:
    """
    Best-effort: return `brew --prefix <pkg>` if brew exists, else None.
    """
    if platform.system() != "Darwin":
        return None
    brew = shutil.which("brew")
    if not brew:
        return None
    try:
        out = subprocess.check_output([brew, "--prefix", pkg], text=True).strip()
        return out or None
    except Exception:
        return None


def detect_eigen_include_root(env: dict) -> str | None:
    r"""
    Return an include root that makes `#include <Eigen/Core>` work.
    Priority:
      1) explicit override --eigen-root / TONATIUH_EIGEN_ROOT
      2) EIGEN3_INCLUDE_DIR / EIGEN3_ROOT (env) if valid
      3) PREFIX installs: <_install>/include/eigen3 or <_install>/eigen3 or <_install>
      4) system/common: /usr/include/eigen3, /usr/local/include/eigen3
      5) macOS Homebrew: /opt/homebrew/include/eigen3, /opt/homebrew/opt/eigen/include/eigen3, brew --prefix eigen
      6) Windows typical: C:\eigen-* (root or include/eigen3)
    """
    if USER_OVERRIDES.get("eigen_root"):
        er = USER_OVERRIDES["eigen_root"]
        if er:
            if os.path.isfile(os.path.join(er, "Eigen", "Core")):
                return er
            if os.path.isfile(os.path.join(er, "include", "eigen3", "Eigen", "Core")):
                return os.path.join(er, "include", "eigen3")
            if os.path.isfile(os.path.join(er, "eigen3", "Eigen", "Core")):
                return os.path.join(er, "eigen3")

    for key in ("EIGEN3_INCLUDE_DIR", "EIGEN3_ROOT", "EIGEN_ROOT"):
        val = env.get(key)
        if val and os.path.isdir(val):
            if os.path.isfile(os.path.join(val, "Eigen", "Core")):
                return val
            if os.path.isfile(os.path.join(val, "include", "eigen3", "Eigen", "Core")):
                return os.path.join(val, "include", "eigen3")
            if os.path.isfile(os.path.join(val, "eigen3", "Eigen", "Core")):
                return os.path.join(val, "eigen3")

    cand = PREFIX / "include" / "eigen3"
    if (cand / "Eigen" / "Core").is_file():
        return str(cand)
    cand = PREFIX / "eigen3"
    if (cand / "Eigen" / "Core").is_file():
        return str(cand)
    cand = PREFIX
    if (cand / "Eigen" / "Core").is_file():
        return str(cand)

    for p in ("/usr/include/eigen3", "/usr/local/include/eigen3"):
        if os.path.isfile(os.path.join(p, "Eigen", "Core")):
            return p

    if platform.system() == "Darwin":
        for p in (
            "/opt/homebrew/include/eigen3",
            "/opt/homebrew/opt/eigen/include/eigen3",
            "/usr/local/include/eigen3",
            "/usr/local/opt/eigen/include/eigen3",
        ):
            if os.path.isfile(os.path.join(p, "Eigen", "Core")):
                return p
        bp = _brew_prefix("eigen")
        if bp:
            p = os.path.join(bp, "include", "eigen3")
            if os.path.isfile(os.path.join(p, "Eigen", "Core")):
                return p

    if platform.system() == "Windows":
        for pat in (r"C:\eigen-*", r"C:\eigen3", r"C:\local\eigen-*", r"C:\local\eigen3"):
            for base in glob.glob(pat):
                if os.path.isfile(os.path.join(base, "Eigen", "Core")):
                    return base
                if os.path.isfile(os.path.join(base, "include", "eigen3", "Eigen", "Core")):
                    return os.path.join(base, "include", "eigen3")
                if os.path.isfile(os.path.join(base, "eigen3", "Eigen", "Core")):
                    return os.path.join(base, "eigen3")

    return None


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
    if USER_OVERRIDES.get("qt_root"):
        return [USER_OVERRIDES["qt_root"]]  # type: ignore[return-value]

    found: list[str] = []
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

            candidates = [c for c in candidates if _qt6_dir_from_prefix(c)]
            candidates = [c for c in candidates if "arm64" not in c.lower()]

            def ver_key(p):
                for part in Path(p).parts:
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
                    for part in Path(p).parts:
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

    seen = set()
    out = []
    for p in prefixes:
        if p not in seen:
            out.append(p)
            seen.add(p)
    return out


def _probe_boost_root(env: dict) -> str | None:
    """
    Try to find a usable Boost root:
      1) Explicit override (CLI/env)
      2) Respect BOOST_ROOT / Boost_ROOT if set and valid.
      3) Otherwise, look in a few common locations.
    """
    if USER_OVERRIDES.get("boost_root"):
        return USER_OVERRIDES["boost_root"]

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
            candidates.extend(glob.glob(pat))
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


def write_local_hints(prefixes: list[str], env: dict | None = None) -> None:
    """
    Write cmake/LocalDepsHints.cmake.
    IMPORTANT: CMAKE_PREFIX_PATH must contain real *prefixes*, not include directories.
    Eigen include is written via EIGEN3_INCLUDE_DIR.
    """
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

    env2 = env or os.environ.copy()
    eigen_inc = detect_eigen_include_root(env2)
    if eigen_inc:
        eigen_inc = _normalize_to_cmake_path(eigen_inc)

    boost_root = _probe_boost_root(env2)
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
# Prefix discovery (important)
# ----------------------------

def _default_system_prefixes() -> list[str]:
    """
    Return sensible system prefixes per OS (prefixes only, not include dirs).
    This is how we stay Ubuntu-version-agnostic.
    """
    out: list[str] = []
    if platform.system() in ("Linux", "Darwin"):
        for p in ("/usr/local", "/usr"):
            if os.path.isdir(p):
                out.append(p)

        if platform.system() == "Darwin":
            for p in ("/opt/homebrew", "/usr/local"):
                if os.path.isdir(p) and p not in out:
                    out.append(p)

    return out

def get_cmake_prefix_paths(env: dict) -> list[str]:
    """
    Compute *prefixes* for dependency discovery.

    Included (in order):
      - our local third_party/_install
      - Qt prefix (from TONATIUH_QT_ROOT / Qt6_DIR / CMAKE_PREFIX_PATH / auto-detect)
      - Boost root (normalized to an actual prefix when possible)
      - environment CMAKE_PREFIX_PATH entries (prefixes only)
      - system prefixes (/usr/local, /usr, /opt/homebrew)
      - optional user --eigen-root (if it is a real prefix)

    IMPORTANT:
      - Never add raw include directories like /usr/include/eigen3 here.
      - Prefer prefix roots (the thing that contains include/ and lib/).
    """
    prefixes: list[str] = []

    def _add(p: str | None) -> None:
        if not p:
            return
        p = os.path.normpath(p)
        if p and p not in prefixes:
            prefixes.append(p)

    # 0) Always start with our install prefix
    _add(str(PREFIX))

    # 1) Qt prefix (highest-priority "external" prefix)
    # Prefer explicit override / env var set by CI: TONATIUH_QT_ROOT
    qt_root = USER_OVERRIDES.get("qt_root") or env.get("TONATIUH_QT_ROOT") or env.get("QT_ROOT_DIR")
    if qt_root and _qt6_dir_from_prefix(qt_root):
        _add(qt_root)
    else:
        detected_qt = _qt_prefixes_from_env(env)
        if not detected_qt:
            detected_qt = _detect_qt_prefixes()
        for p in detected_qt:
            _add(p)

    # 2) Boost "prefix"
    # _probe_boost_root may return either a prefix (/usr, C:\boost_*) or an include dir (vcpkg .../include).
    # Normalize include-dir inputs back to the vcpkg prefix when possible.
    boost_root = _probe_boost_root(env)
    if boost_root:
        br = os.path.normpath(boost_root)
        # vcpkg layout: <prefix>/include/boost/version.hpp
        if br.lower().endswith(os.path.normpath("include").lower()):
            cand_prefix = os.path.dirname(br)
            if os.path.isdir(os.path.join(cand_prefix, "include")):
                br = cand_prefix
        _add(br)

    # 3) Environment CMAKE_PREFIX_PATH entries
    raw = env.get("CMAKE_PREFIX_PATH", "") or ""
    for p in [p for p in raw.replace(";", os.pathsep).split(os.pathsep) if p]:
        # Skip common "bad" entries that are not prefixes
        low = p.replace("\\", "/").lower()
        if low.endswith("/include") or "/include/" in low:
            continue
        if low.endswith("/include/eigen3") or low.endswith("/eigen3"):
            continue
        _add(p)

    # 4) System prefixes
    for p in _default_system_prefixes():
        _add(p)

    # 5) If user passed --eigen-root, add it ONLY if it is a real prefix (not just .../include/eigen3)
    er = USER_OVERRIDES.get("eigen_root")
    if er:
        ern = os.path.normpath(er)
        low = ern.replace("\\", "/").lower()
        if not (low.endswith("/include/eigen3") or low.endswith("/include")):
            _add(ern)

    return prefixes

# ----------------------------
# Probe compilation helper flags
# ----------------------------

def add_extra_includes_and_libs_from_prefixes(cmd_list: list[str],
                                             prefixes: list[str],
                                             msvc: bool,
                                             env: dict | None = None) -> None:
    """
    Add reasonable -I/-L (or /I /LIBPATH:) flags based on prefixes.

    - Special-cases Qt, Eigen and Boost includes.
    - On macOS with Qt frameworks, also adds -F<libdir> and framework Headers.
    """
    includes: list[str] = []
    libpaths: list[str] = []
    framework_roots: list[str] = []

    qt_modules = ["QtCore", "QtGui", "QtWidgets", "QtOpenGL", "QtOpenGLWidgets"]

    linux_qt6_roots: list[str] = []
    if platform.system() == "Linux" and _linux_has_system_qt6():
        linux_qt6_roots = _linux_qt6_include_roots()

    multiarch_lib_subdirs = [
        "lib",
        "lib64",
        "lib/x86_64-linux-gnu",
        "lib/aarch64-linux-gnu",
        "lib/arm-linux-gnueabihf",
    ]

    env2 = env or os.environ.copy()
    eigen_inc = detect_eigen_include_root(env2)
    if eigen_inc and os.path.isdir(eigen_inc):
        includes.append(eigen_inc)

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

            eigen3_inc = os.path.join(inc_root, "eigen3")
            if os.path.isdir(eigen3_inc):
                includes.append(eigen3_inc)

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

    if linux_qt6_roots:
        for root in linux_qt6_roots:
            includes.append(root)
            for mod in qt_modules:
                mod_dir = os.path.join(root, mod)
                if os.path.isdir(mod_dir):
                    includes.append(mod_dir)

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

    # Strip Windows-only defines on non-Windows probe builds.
    # (Avoid SoQt/simage import/export macro mismatches on macOS/Linux.)
    if platform.system() != "Windows" and cc_defs:
        cc_defs = [d for d in cc_defs if d not in ("SOQT_DLL", "SIMAGE_DLL")]

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

    # ---- helpers for Qt runtime in headless CI (Linux runners have no DISPLAY) ----
    def _looks_like_qt_probe() -> bool:
        # If we include Qt headers or link Qt libs, assume this probe may load Qt plugins at runtime.
        if "Qt" in inc_lines or "QApplication" in code or "QGuiApplication" in code:
            return True
        for L in extra_libs:
            if isinstance(L, str) and (L.startswith("Qt6") or L.startswith("Qt")):
                return True
        if link_lib and "Qt" in str(link_lib):
            return True
        return False

    def _add_qt_plugin_env(env_run: dict) -> None:
        """
        Try to locate Qt plugins/platforms under any known prefix and set:
          - QT_PLUGIN_PATH
          - QT_QPA_PLATFORM_PLUGIN_PATH
        This helps when Qt is installed in a non-system prefix (e.g. install-qt-action).
        """
        # Respect explicit user settings
        if env_run.get("QT_QPA_PLATFORM_PLUGIN_PATH") and env_run.get("QT_PLUGIN_PATH"):
            return

        candidate_prefixes: list[str] = []

        # 1) include the computed prefixes first
        candidate_prefixes += [p for p in prefixes if p]

        # 2) common Qt env vars (install-qt-action / local installs)
        for k in ("QT_ROOT_DIR", "QTDIR", "Qt_ROOT_DIR", "Qt6_ROOT", "Qt6_ROOT_DIR", "TONATIUH_QT_ROOT"):
            val = env.get(k)
            if val:
                candidate_prefixes.append(val)

        # De-dup while preserving order
        seen = set()
        ordered = []
        for p in candidate_prefixes:
            if p and p not in seen:
                ordered.append(p)
                seen.add(p)

        for p in ordered:
            pth = Path(p)
            plugins = pth / "plugins"
            platforms = plugins / "platforms"
            if platforms.is_dir():
                env_run.setdefault("QT_PLUGIN_PATH", str(plugins))
                env_run.setdefault("QT_QPA_PLATFORM_PLUGIN_PATH", str(platforms))
                return

            # Some layouts put plugins under <prefix>/lib/qt6/plugins (less common)
            plugins2 = pth / "lib" / "qt6" / "plugins"
            platforms2 = plugins2 / "platforms"
            if platforms2.is_dir():
                env_run.setdefault("QT_PLUGIN_PATH", str(plugins2))
                env_run.setdefault("QT_QPA_PLATFORM_PLUGIN_PATH", str(platforms2))
                return

    # ---------- macOS: handle Qt properly (frameworks on install-qt-action) ----------
    qt_fw_names: list[str] = []
    extra_libs_for_path_resolve = list(extra_libs)
    qt_framework_dirs: set[str] = set()

    def _add_qt_framework_dir_from_prefix(prefix: str) -> None:
        if not prefix:
            return
        libp = Path(prefix) / "lib"
        if (libp / "QtCore.framework").exists():
            qt_framework_dirs.add(str(libp))

    if platform.system() == "Darwin":
        # 1) from known prefixes
        for p in prefixes:
            _add_qt_framework_dir_from_prefix(p)

        # 2) from install-qt-action envs
        for k in ("QT_ROOT_DIR", "QTDIR", "Qt_ROOT_DIR", "Qt6_ROOT", "Qt6_ROOT_DIR", "TONATIUH_QT_ROOT"):
            val = env.get(k)
            if val:
                _add_qt_framework_dir_from_prefix(val)

        # Translate Qt6* logical libs into -framework Qt*
        if extra_libs:
            non_qt6 = []
            for L in extra_libs:
                if isinstance(L, str) and L.startswith("Qt6"):
                    qt_fw_names.append("Qt" + L[len("Qt6"):])  # Qt6Core -> QtCore, etc.
                else:
                    non_qt6.append(L)
            extra_libs_for_path_resolve = non_qt6

    # Resolve non-Qt libraries to absolute paths (Qt on macOS is handled as frameworks)
    extra_lib_paths = resolve_lib_paths(extra_libs_for_path_resolve, prefixes)

    # Windows: also add all prefix bin/ to PATH so probe can run
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

    runtime_lib_dirs: set[str] = set()

    if cc_name == "cl":
        cc_norm = cc.replace("/", "\\").lower()
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

        # Only add our PREFIX include if it exists (Qt/Eigen/Boost may be system-provided)
        if include_dir.is_dir():
            cmd.append(f"/I{include_dir}")

        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=True, env=env)

        cmd += [str(src), "/link"]

        # Only add our PREFIX lib if it exists
        if lib_dir.is_dir():
            cmd.append(f"/LIBPATH:{lib_dir}")

        cmd += ["/MACHINE:X64"]

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

        cmd.append(str(src))

        # Only add our PREFIX include/lib if they exist
        if include_dir.is_dir():
            cmd.append(f"-I{include_dir}")
        if lib_dir.is_dir():
            cmd.extend(["-L", str(lib_dir)])

        add_extra_includes_and_libs_from_prefixes(cmd, prefixes, msvc=False, env=env)

        # Runtime search paths (rpath / DYLD_LIBRARY_PATH)
        if lib_dir.is_dir():
            runtime_lib_dirs.add(str(lib_dir))

        for p in prefixes:
            pr_lib = Path(p) / "lib"
            if pr_lib.is_dir():
                runtime_lib_dirs.add(str(pr_lib))

        if link_lib:
            runtime_lib_dirs.add(str(Path(link_lib).parent))
        for p in extra_lib_paths:
            runtime_lib_dirs.add(str(Path(p).parent))

        # macOS: add Qt frameworks if requested
        if platform.system() == "Darwin" and qt_fw_names:
            # If we still didn't find any framework dir, fail with a useful error
            if not qt_framework_dirs:
                raise RuntimeError(
                    "Qt frameworks not found on macOS. "
                    "Expected QtCore.framework under <QtPrefix>/lib.\n"
                    "Tip: ensure install-qt-action ran, and QT_ROOT_DIR is set, or pass --qt-root."
                )

            for fdir in sorted(qt_framework_dirs):
                cmd.append(f"-F{fdir}")
                runtime_lib_dirs.add(fdir)

            for fw in qt_fw_names:
                cmd.extend(["-framework", fw])

        # rpaths (macOS uses LC_RPATH; linux uses DT_RUNPATH/DT_RPATH)
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
    ld_var = "DYLD_LIBRARY_PATH" if platform.system() == "Darwin" else "LD_LIBRARY_PATH"
    existing = env_run.get(ld_var, "")
    rt_paths = os.pathsep.join(sorted(runtime_lib_dirs)) if runtime_lib_dirs else ""
    env_run[ld_var] = (rt_paths + (os.pathsep + existing if existing else "")) if rt_paths else existing

    # ---- FIX: headless Qt probes on Linux CI (no DISPLAY) ----
    # This prevents crashes like:
    #   qt.qpa.xcb: could not connect to display
    #   Could not load the Qt platform plugin "xcb" ...
    if platform.system() == "Linux" and _looks_like_qt_probe():
        # Don't override if the user already chose a platform
        env_run.setdefault("QT_QPA_PLATFORM", "offscreen")
        # Help Qt find its plugins when using install-qt-action prefixes
        _add_qt_plugin_env(env_run)

    run([str(exe)], cwd=str(work), env=env_run)
    print("[compile-check] OK")

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


def _find_qt6_moc():
    p = shutil.which("moc")
    if p:
        return p
    candidates = [
        "/usr/lib/qt6/libexec/moc",
        "/usr/lib/x86_64-linux-gnu/qt6/libexec/moc",
    ]
    for c in candidates:
        if os.path.isfile(c) and os.access(c, os.X_OK):
            return c
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
            moc = _find_qt6_moc()
            if moc:
                ok(f"Qt moc: {moc}")
            else:
                warn("Qt moc not found (Qt meta-object compiler)")

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

    env = os.environ.copy()
    prefixes = get_cmake_prefix_paths(env)

    eigen_inc = detect_eigen_include_root(env)
    if eigen_inc:
        ok(f"Eigen include root: {eigen_inc}")
    else:
        warn("Eigen not detected (set --eigen-root or install eigen headers)")

    boost_root = _probe_boost_root(env)
    if boost_root:
        ok(f"Boost root: {boost_root}")
    else:
        warn("Boost root not detected (set --boost-root / TONATIUH_BOOST_ROOT)")

    if platform.system() == "Linux" and has_system_simage():
        ver = system_simage_version()
        if ver:
            ok(f"simage available from system via pkg-config (version {ver})")
        else:
            ok("simage available from system via pkg-config")
    else:
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

def _sanitize_cmake_options(dep_name: str, cmake_options: list[str]) -> list[str]:
    """
    Remove/adjust options that are invalid on the current OS.
    This keeps deps.yaml mostly platform-agnostic.
    """
    opts = list(cmake_options or [])
    sysname = platform.system()

    if dep_name.lower() == "simage" and sysname != "Windows":
        # GDI+ is Windows-only; passing it breaks configure on Linux/macOS.
        opts = [o for o in opts if not o.startswith("-DSIMAGE_USE_GDIPLUS=")]

    return opts


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
    cmake_opts = _sanitize_cmake_options(name, dep.get("cmake_options", []))

    cmake_cmd = [
        "cmake",
        "-S", str(src_dir),
        "-B", str(bld_dir),
        f"-DCMAKE_INSTALL_PREFIX={PREFIX}",
    ] + gen + cmake_opts

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

    # Eigen (header-only): tell CMake where headers are if we can detect them.
    # IMPORTANT: do not pass -DEigen3_DIR= (empty) — that can confuse CMake find logic.
    eigen_inc = detect_eigen_include_root(env)
    if eigen_inc:
        cmake_cmd.append(f"-DEIGEN3_INCLUDE_DIR={eigen_inc}")

    cmake_cmd.append(f"-DCMAKE_BUILD_TYPE={config}")

    cmake_prefixes = get_cmake_prefix_paths(env)
    if cmake_prefixes:
        cmake_cmd.append(f"-DCMAKE_PREFIX_PATH={';'.join(cmake_prefixes)}")

    # Qt6_DIR (optional but helpful)
    qt6_dir_env = env.get("Qt6_DIR") or env.get("QT6_DIR")
    if qt6_dir_env and os.path.isdir(qt6_dir_env):
        cmake_cmd.append(f"-DQt6_DIR={qt6_dir_env}")
    else:
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
        run(["cmake", "--build", str(bld_dir), "--verbose"], env=env)
        run(["cmake", "--install", str(bld_dir)], env=env)

    write_local_hints(get_cmake_prefix_paths(env), env=env)
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

    ap.add_argument("--qt-root", help=r"Qt prefix root (e.g. C:\Qt\6.10.1\msvc2022_64). Overrides auto-detect.")
    ap.add_argument("--boost-root", help=r"Boost root containing boost/version.hpp (e.g. C:\boost_1_89_0).")
    ap.add_argument("--eigen-root", help=r"Eigen root (e.g. C:\eigen-5.0.0 or a folder containing Eigen/Core).")

    args = ap.parse_args()

    _apply_overrides_from_env_and_args(args)

    if args.doctor:
        cmd_doctor()
        return

    manifest = TP / "deps.yaml"
    if not manifest.exists():
        print("Manifest not found at third_party/deps.yaml", file=sys.stderr)
        sys.exit(1)

    env = os.environ.copy()
    write_local_hints(get_cmake_prefix_paths(env), env=env)

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

        # Linux: prefer system simage (libsimage-dev) to avoid giflib API/prototype mismatches
        if platform.system() == "Linux" and name.lower() == "simage" and has_system_simage():
            ver = system_simage_version()
            msg = f"[deps] Using system simage (pkg-config simage {ver})" if ver else "[deps] Using system simage (pkg-config simage)"
            print(msg + "; skipping local simage build.")

            step_dir = BUILD / name
            step_dir.mkdir(parents=True, exist_ok=True)
            (step_dir / ".ok").write_text("ok", encoding="utf-8")

            write_local_hints(get_cmake_prefix_paths(os.environ.copy()), env=os.environ.copy())
            continue

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
            write_local_hints(get_cmake_prefix_paths(os.environ.copy()), env=os.environ.copy())
            print(f"=== [{name}] OK (presence verified) ===")
            continue

        raise SystemExit(f"Unsupported dep kind '{kind}' for {name} in this script.")


if __name__ == "__main__":
    main()