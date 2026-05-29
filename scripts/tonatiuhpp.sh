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

QT_BUNDLED_PLUGIN_DIR="${INSTALL_ROOT}/bin"
if [[ -d "${QT_BUNDLED_PLUGIN_DIR}/platforms" ]]; then
  export QT_PLUGIN_PATH="${QT_BUNDLED_PLUGIN_DIR}"
  export QT_QPA_PLATFORM_PLUGIN_PATH="${QT_BUNDLED_PLUGIN_DIR}/platforms"
fi

exec "${APP_BIN}" "$@"
