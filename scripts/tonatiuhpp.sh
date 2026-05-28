#!/usr/bin/env bash
set -euo pipefail

# Directory where the installed launcher lives (<prefix>/bin).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
APP_BIN="${INSTALL_ROOT}/bin/tonatiuhpp-bin"

if [[ ! -x "${APP_BIN}" ]]; then
  # Fallback for developer trees or older install layouts.
  APP_BIN="${INSTALL_ROOT}/bin/tonatiuhpp"
fi

export LD_LIBRARY_PATH="${INSTALL_ROOT}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

find_qt_plugin_dir() {
  local candidate

  for tool in qtpaths6 qtpaths-qt6; do
    if command -v "${tool}" >/dev/null 2>&1; then
      candidate="$("${tool}" --plugin-dir 2>/dev/null || true)"
      if [[ -d "${candidate}/platforms" ]]; then
        printf '%s\n' "${candidate}"
        return 0
      fi
    fi
  done

  for tool in qmake6 qmake-qt6; do
    if command -v "${tool}" >/dev/null 2>&1; then
      candidate="$("${tool}" -query QT_INSTALL_PLUGINS 2>/dev/null || true)"
      if [[ -d "${candidate}/platforms" ]]; then
        printf '%s\n' "${candidate}"
        return 0
      fi
    fi
  done

  for candidate in \
    /usr/lib/*/qt6/plugins \
    /usr/lib/qt6/plugins \
    /usr/local/lib/*/qt6/plugins \
    /usr/local/lib/qt6/plugins; do
    if [[ -d "${candidate}/platforms" ]]; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done

  return 1
}

if QT_SYSTEM_PLUGIN_DIR="$(find_qt_plugin_dir)"; then
  export QT_PLUGIN_PATH="${QT_SYSTEM_PLUGIN_DIR}${QT_PLUGIN_PATH:+:${QT_PLUGIN_PATH}}"
  export QT_QPA_PLATFORM_PLUGIN_PATH="${QT_SYSTEM_PLUGIN_DIR}/platforms"
fi

exec "${APP_BIN}" "$@"
