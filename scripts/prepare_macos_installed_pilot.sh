#!/usr/bin/env bash
# Exploratory Phase E macOS CMake-install packaging, derived from the
# existing release app-bundle consolidation. Never run on user installs.
set -euo pipefail
prefix="${1:?Disposable install prefix required}"
case "$prefix" in "${RUNNER_TEMP:?}"/*) ;; *) echo "Not a runner temp install: $prefix" >&2; exit 1 ;; esac
[[ -d "$prefix/tonatiuhpp-gui.app" ]] || { echo "Missing installed app" >&2; exit 1; }
work="$(mktemp -d "${RUNNER_TEMP}/tonatiuhpp-macos-stage-XXXXXXXX")"
ln -s "$prefix" "$work/dist"
cd "$work"
APP_BUNDLE="dist/tonatiuhpp-gui.app"
APP_EXEC="${APP_BUNDLE}/Contents/MacOS/tonatiuhpp-gui"
macdeployqt "$APP_BUNDLE" -verbose=1
FRAMEWORKS_DIR="${APP_BUNDLE}/Contents/Frameworks"
MACOS_DIR="${APP_BUNDLE}/Contents/MacOS"
APP_RESOURCES_DIR="${APP_BUNDLE}/Contents/Resources"
APP_PLUGINS_DIR="${MACOS_DIR}/plugins"
if [ ! -x dist/bin/tonatiuhpp-cli ]; then
  echo "Missing installed macOS CLI before app-bundle consolidation" >&2
  exit 1
fi
mv dist/bin/tonatiuhpp-cli "${MACOS_DIR}/tonatiuhpp-cli"
chmod u+x "${MACOS_DIR}/tonatiuhpp-cli"
mkdir -p "${FRAMEWORKS_DIR}"
mkdir -p "${APP_RESOURCES_DIR}"
mkdir -p "${APP_PLUGINS_DIR}"
# Indirect SoQt QtOpenGL dependency can be absent from macdeployqt output.
QT_FRAMEWORK_LIB_ROOT="${TONATIUH_QT_ROOT:-${QT_ROOT_DIR:-}}/lib"
for fw in QtOpenGL QtOpenGLWidgets; do
  if [ -d "${QT_FRAMEWORK_LIB_ROOT}/${fw}.framework" ] && [ ! -e "${FRAMEWORKS_DIR}/${fw}.framework" ]; then
    ditto "${QT_FRAMEWORK_LIB_ROOT}/${fw}.framework" "${FRAMEWORKS_DIR}/${fw}.framework"
  fi
done

PRIVATE_DYLIBS=()
while IFS= read -r dylib; do
  [ -n "${dylib}" ] || continue
  PRIVATE_DYLIBS+=("${dylib}")
done < <(
  { otool -L "${APP_EXEC}"; otool -L "${MACOS_DIR}/tonatiuhpp-cli"; } |
  grep -E '^[[:space:]]' | awk '{print $1}' | while read -r dep; do
    base="$(basename "${dep}")"
    if [[ "${base}" == libTonatiuh*.dylib && -f "dist/bin/${base}" ]]; then
      echo "${base}"
    fi
  done | sort -u
)

# Include Coin/SoQt runtime libraries, which CMake's macOS install did
# not stage and which release packaging previously borrowed from build SDK.
for src in "${GITHUB_WORKSPACE}/third_party/_install/lib"/libCoin*.dylib "${GITHUB_WORKSPACE}/third_party/_install/lib"/libSoQt*.dylib; do
  [ -e "${src}" ] || continue
  base="$(basename "${src}")"
  cp -fL "${src}" "dist/bin/${base}"
  PRIVATE_DYLIBS+=("${base}")
done
PRIVATE_DYLIBS=($(printf "%s\n" "${PRIVATE_DYLIBS[@]}" | sort -u))
if [ "${#PRIVATE_DYLIBS[@]}" -eq 0 ]; then
  echo "No Tonatiuh private dylibs referenced by ${APP_EXEC}" >&2
  exit 1
fi

echo "Bundling private dylibs:"
printf '  %s\n' "${PRIVATE_DYLIBS[@]}"

ensure_rpath() {
  local file="$1"
  local rpath="$2"
  if ! otool -l "${file}" | awk '
    $1 == "cmd" && $2 == "LC_RPATH" { in_rpath=1; next }
    in_rpath && $1 == "path" { print $2; in_rpath=0 }
  ' | grep -Fxq "${rpath}"; then
    install_name_tool -add_rpath "${rpath}" "${file}"
  fi
}

rewrite_private_refs() {
  local file="$1"
  while read -r dep; do
    [ -n "${dep}" ] || continue
    local base
    base="$(basename "${dep}")"
    if [[ " ${PRIVATE_DYLIBS[*]} " == *" ${base} "* ]]; then
      install_name_tool -change "${dep}" "@rpath/${base}" "${file}"
    fi
  done < <(otool -L "${file}" | tail -n +2 | awk '{print $1}')
}

ensure_rpath "${APP_EXEC}" "@executable_path/../Frameworks"
ensure_rpath "${MACOS_DIR}/tonatiuhpp-cli" "@executable_path/../Frameworks"

for dylib in "${PRIVATE_DYLIBS[@]}"; do
  src="dist/bin/${dylib}"
  dst="${FRAMEWORKS_DIR}/${dylib}"
  cp -f "${src}" "${dst}"
  chmod u+w "${dst}"
  install_name_tool -id "@rpath/${dylib}" "${dst}"
  ensure_rpath "${dst}" "@loader_path"
done

rewrite_private_refs "${APP_EXEC}"
rewrite_private_refs "${MACOS_DIR}/tonatiuhpp-cli"
for dylib in "${PRIVATE_DYLIBS[@]}"; do
  rewrite_private_refs "${FRAMEWORKS_DIR}/${dylib}"
  rm -f "dist/bin/${dylib}"
done

for plugin in dist/bin/*.dylib; do
  [ -f "${plugin}" ] || continue
  base="$(basename "${plugin}")"
  dst="${APP_PLUGINS_DIR}/${base}"
  mv "${plugin}" "${dst}"
  chmod u+w "${dst}"
  ensure_rpath "${dst}" "@loader_path/../../Frameworks"
  rewrite_private_refs "${dst}"
done

for data_dir in examples help resources; do
  if [ -d "dist/${data_dir}" ]; then
    rm -rf "${APP_RESOURCES_DIR}/${data_dir}"
    mv "dist/${data_dir}" "${APP_RESOURCES_DIR}/${data_dir}"
  fi
done

if [ -d dist/bin ] && [ -z "$(find dist/bin -mindepth 1 -print -quit)" ]; then
  rmdir dist/bin
fi

# The macOS release artifact is an app-bundle-only payload. CMake
# install may also stage development artifacts at the top level; keep
# them out of dist/ after all runtime files have moved into the app.
for dev_dir in include lib; do
  if [ -e "dist/${dev_dir}" ]; then
    echo "Removing macOS top-level development artifact: dist/${dev_dir}"
    rm -rf "dist/${dev_dir}"
  fi
done


# Native startup alone does not prove that Mach-O install names and RPATHs
# are independent of the developer's Qt/SoQt/Coin SDK. Reject absolute
# non-system runtime references in every bundled executable, dylib, and
# framework Mach-O object, including Qt plugins.
audit_macho() {
  local candidate="$1" dependency rpath
  if ! file "$candidate" | grep -q 'Mach-O'; then
    return 0
  fi
  while IFS= read -r dependency; do
    case "$dependency" in
      @rpath|@rpath/*|@loader_path|@loader_path/*|@executable_path|@executable_path/*|/System/Library/*|/usr/lib/*) ;;
      *)
        echo "Mac installed bundle has unresolved non-bundled dependency: $candidate -> $dependency" >&2
        return 1 ;;
    esac
  done < <(otool -L "$candidate" | tail -n +2 | awk '{print $1}')
  while IFS= read -r rpath; do
    case "$rpath" in
      @loader_path|@loader_path/*|@executable_path|@executable_path/*|@rpath|@rpath/*|/System/Library/*|/usr/lib/*) ;;
      *)
        echo "Mac installed bundle has external or unrecognized LC_RPATH: $candidate -> $rpath" >&2
        return 1 ;;
    esac
  done < <(otool -l "$candidate" | awk '
      $1 == "cmd" && $2 == "LC_RPATH" { in_rpath=1; next }
      in_rpath && $1 == "path" { print $2; in_rpath=0 }
    ')
}
audit_macho "$APP_EXEC"
audit_macho "$MACOS_DIR/tonatiuhpp-cli"
while IFS= read -r -d '' binary; do
  audit_macho "$binary"
done < <(find "$APP_BUNDLE/Contents/Frameworks" "$APP_BUNDLE/Contents/MacOS/plugins" -type f -print0)
echo "MACOS_BUNDLE_RUNTIME_PATH_AUDIT=PASS"
