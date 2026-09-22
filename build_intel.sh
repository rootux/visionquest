#!/bin/bash
# Cross-build an Intel (x86_64) bundle from an Apple Silicon Mac, so the machine
# that runs the installation does not have to build anything itself.
#
# Two things make this necessary rather than a one-line -arch flag:
#   * the openFrameworks core has to be compiled for x86_64 as well, and its
#     object files live inside the openFrameworks tree - so we build against an
#     APFS clone of it and leave your arm64 copy completely untouched;
#   * Homebrew's libusb only ever carries the host architecture, so the x86_64
#     build links the copy bundled in src/libusb instead (see config.make).
#
# The result is bin/<name>-x86_64.app. Verify it here with:
#     arch -x86_64 bin/<name>-x86_64.app/Contents/MacOS/<name>
#
#     OF_ROOT=/path/to/openFrameworks ./build_intel.sh

set -e
cd "$(dirname "$0")"

OF_SRC="${OF_ROOT:-$HOME/openFrameworks}"
OF_X86="${OF_X86:-/tmp/openFrameworks-x86_64}"
APP_NAME="$(basename "$PWD")"
JOBS="$(sysctl -n hw.ncpu)"

if [ "$(uname -m)" != "arm64" ]; then
	echo "Already on Intel - just run ./run_mac.sh" >&2
	exit 1
fi

# An APFS clone is near-instant and shares storage until written to, so this
# costs neither minutes nor a second copy of openFrameworks on disk.
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

# The project's own object directory is per-arch too, and is shared with the
# native build, so start from clean and leave it clean for the next arm64 build.
echo "==> Building $APP_NAME for x86_64"
make -s clean >/dev/null
make -j"$JOBS" Release OF_ROOT="$OF_X86" MAC_ARCH=x86_64 "${CROSS[@]}"

rm -rf "bin/$APP_NAME-x86_64.app"
mv "bin/$APP_NAME.app" "bin/$APP_NAME-x86_64.app"
codesign --force --sign - --identifier cc.openFrameworks.visionquest "bin/$APP_NAME-x86_64.app"

# Leave the tree as we found it: object files back to native, native app rebuilt.
echo "==> Restoring the native arm64 build"
make -s clean >/dev/null
make -j"$JOBS" Release >/dev/null
codesign --force --sign - --identifier cc.openFrameworks.visionquest "bin/$APP_NAME.app"

echo
echo "Intel bundle:  bin/$APP_NAME-x86_64.app"
lipo -archs "bin/$APP_NAME-x86_64.app/Contents/MacOS/$APP_NAME"
echo "Native bundle: bin/$APP_NAME.app"
lipo -archs "bin/$APP_NAME.app/Contents/MacOS/$APP_NAME"
echo
echo "Copy the Intel bundle to the Intel Mac along with bin/data."
