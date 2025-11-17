#!/usr/bin/env bash
# Simple launcher for a self-contained TonatiuhXX install

# Directory where the installed script lives (<prefix>/bin)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Installation prefix is one level above
INSTALL_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Ensure our private lib dir is searched first
export LD_LIBRARY_PATH="${INSTALL_ROOT}/lib:${LD_LIBRARY_PATH}"

# Run the real binary
exec "${INSTALL_ROOT}/bin/TonatiuhXX" "$@"