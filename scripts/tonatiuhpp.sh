#!/usr/bin/env bash
set -euo pipefail

# Directory where the installed launcher lives (<prefix>/bin).
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

export LD_LIBRARY_PATH="${INSTALL_ROOT}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"

exec "${INSTALL_ROOT}/bin/tonatiuhpp" "$@"
