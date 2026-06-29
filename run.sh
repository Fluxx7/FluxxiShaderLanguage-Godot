#!/usr/bin/env bash
# Run the Godot project and capture the log (symbolicated, if built with
# debug_symbols).
#
#   ./run.sh [seconds]            # windowed: real RenderingDevice, like the editor
#   ./run.sh [seconds] --headless # dummy renderer, no GPU (compute will not work)
#
# Output is teed to /tmp/godot_run.log. Override Godot with GODOT=/path ./run.sh
set -euo pipefail
cd "$(dirname "$0")"

GODOT="${GODOT:-/Applications/Godot.app/Contents/MacOS/Godot}"
QUIT_AFTER="${1:-5}"
LOG="/tmp/godot_run.log"

ARGS=(--path project --quit-after "$QUIT_AFTER")
if [ "${2:-}" = "--headless" ]; then
    ARGS=(--headless "${ARGS[@]}")
fi

"$GODOT" "${ARGS[@]}" 2>&1 | tee "$LOG"
