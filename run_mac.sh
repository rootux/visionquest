#!/bin/bash
# Build and launch visionquest on macOS.
#
# Two things here are not optional:
#   * the .app gets an ad-hoc code signature, because the linker-signed binary
#     the makefile produces has no stable identity for macOS privacy controls;
#   * the app is started through LaunchServices ("open"), because a process
#     started straight from a shell inherits the terminal's privacy context and
#     the camera request is refused without ever prompting.
#
# Point OF_ROOT at your openFrameworks install if it is not ~/openFrameworks:
#     OF_ROOT=/path/to/openFrameworks ./run_mac.sh

set -e
cd "$(dirname "$0")"

TARGET="${TARGET:-Release}"
APP="bin/visionquest.app"

make -j"$(sysctl -n hw.ncpu)" "$TARGET"

codesign --force --sign - --identifier cc.openFrameworks.visionquest "$APP"

LOG="${TMPDIR:-/tmp}/visionquest.log"
: > "$LOG"
open --stdout "$LOG" --stderr "$LOG" "$APP"

echo "visionquest launched. Log: $LOG"
echo "The first run asks for camera access - click Allow, then relaunch."
