#!/bin/bash
# Build both macOS bundles from one Apple Silicon Mac, so the machine that runs
# the installation does not have to build anything itself:
#
#     bin/<name>.app          native (arm64)
#     bin/<name>-x86_64.app   Intel, for a pre-Apple-Silicon Mac
#
# Two things make the Intel half more than an -arch flag:
#   * the openFrameworks core has to be compiled for x86_64 as well, and its
#     object files live inside the openFrameworks tree - so we build against an
#     APFS clone of it and leave your own copy completely untouched;
#   * Homebrew's libusb only ever carries the host architecture, so the x86_64
#     build links the copy bundled in src/libusb instead (see config.make).
#
# The Intel bundle can be smoke-tested here, under Rosetta:
#     open bin/<name>-x86_64.app
#
#     OF_ROOT=/path/to/openFrameworks ./build_both.sh

set -e
cd "$(dirname "$0")"

OF_SRC="${OF_ROOT:-$HOME/openFrameworks}"
OF_X86="${OF_X86:-/tmp/openFrameworks-x86_64}"
JOBS="$(sysctl -n hw.ncpu)"

# config.make pins APPNAME so the bundle keeps its name in a worktree or a
# renamed clone; read it from there rather than repeating it.
APP_NAME="$(sed -n 's/^[[:space:]]*APPNAME[[:space:]]*=[[:space:]]*\([^[:space:]#]*\).*/\1/p' config.make | tail -1)"
: "${APP_NAME:=$(basename "$PWD")}"

if [ "$(uname -m)" != "arm64" ]; then
	echo "This cross-builds Intel from Apple Silicon. On an Intel Mac ./run_mac.sh" >&2
	echo "already produces the only bundle that machine needs." >&2
	exit 1
fi

if [ ! -d "$OF_X86" ]; then
	echo "==> Cloning $OF_SRC -> $OF_X86"
	cp -c -R "$OF_SRC" "$OF_X86"
fi

# openFrameworks adds -mtune=native, which on this host resolves to an Apple
# Silicon CPU name that the x86_64 target rejects outright. -O3 alone is what
# the release build is really after.
OPT_FLAGS="-O3"
# Carrying the architecture on the compiler rather than in CFLAGS means an
# implicit rebuild of the core, which the project makefile will happily trigger,
# also comes out x86_64 instead of silently replacing it with a native one.
CROSS=(CC="cc -arch x86_64" CXX="c++ -arch x86_64"
       PLATFORM_OPTIMIZATION_CFLAGS_RELEASE="$OPT_FLAGS")

CORE_LIB="$OF_X86/libs/openFrameworksCompiled/lib/osx/libopenFrameworks.a"
if [ "$(lipo -archs "$CORE_LIB" 2>/dev/null)" = "x86_64" ]; then
	echo "==> openFrameworks core is already built for x86_64"
else
	echo "==> Building the openFrameworks core for x86_64 (this is the slow part)"
	# Anything left from a native build would be quietly skipped at link time.
	rm -rf "$OF_X86/libs/openFrameworksCompiled/lib/osx" "$OF_X86/addons/obj"
	make -C "$OF_X86/libs/openFrameworksCompiled/project" -j"$JOBS" Release "${CROSS[@]}"
fi

# The project's object directory is per-architecture too and is shared between
# the two builds, so each one starts from clean.
echo "==> Building $APP_NAME for x86_64"
make -s clean >/dev/null
make -j"$JOBS" Release OF_ROOT="$OF_X86" MAC_ARCH=x86_64 "${CROSS[@]}"

rm -rf "bin/$APP_NAME-x86_64.app"
mv "bin/$APP_NAME.app" "bin/$APP_NAME-x86_64.app"

echo "==> Building $APP_NAME for arm64"
make -s clean >/dev/null
make -j"$JOBS" Release >/dev/null

# Ad-hoc signatures, for the same reason run_mac.sh signs: the linker-signed
# binary has no stable identity for the macOS privacy controls. Both bundles
# share one identifier so they share one camera permission.
for app in "bin/$APP_NAME.app" "bin/$APP_NAME-x86_64.app"; do
	codesign --force --sign - --identifier cc.openFrameworks.visionquest "$app"
done

echo
for app in "bin/$APP_NAME.app" "bin/$APP_NAME-x86_64.app"; do
	printf '%-40s %s\n' "$app" "$(lipo -archs "$app/Contents/MacOS/$APP_NAME")"
done
echo
echo "Ship a bundle together with bin/data - it will not run without it."
