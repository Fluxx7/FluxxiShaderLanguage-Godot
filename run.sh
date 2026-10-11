#!/usr/bin/env bash
# Run the Godot project and capture the log (symbolicated, if built with
# debug_symbols).
#
#   ./run.sh [seconds]            # windowed: real RenderingDevice, like the editor
#   ./run.sh [seconds] --headless # dummy renderer, no GPU (compute will not work)
#
# Defaults to the Godot 4.8 fork the extension is compiled against; a stock
# 4.7 install will not load the GDExtension. Override with GODOT=/path ./run.sh
#
# Output is teed to /tmp/godot_run.log.
set -euo pipefail
cd "$(dirname "$0")"

GODOT="${GODOT:-/Users/nicholas/Godot/Godot Compilations/Godot 4.8 Fork.app/Contents/MacOS/Godot}"
QUIT_AFTER="${1:-5}"
LOG="/tmp/godot_run.log"

if [ ! -x "$GODOT" ]; then
    echo "error: Godot binary not found at '$GODOT' (set \$GODOT to override)" >&2
    exit 1
fi

ARGS=(--path project --quit-after "$QUIT_AFTER")
if [ "${2:-}" = "--headless" ]; then
    ARGS=(--headless "${ARGS[@]}")
fi

"$GODOT" "${ARGS[@]}" 2>&1 | tee "$LOG"
