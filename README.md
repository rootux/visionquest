# visionquest
Midburn 2016 Kinect Art Installation

Visuals & Info
---
https://www.youtube.com/channel/UC-Nq--JUlaVVTsBCXkxONbg

http://www.headstart.co.il/project.aspx?id=18813&lan=en-US

Info
---
Based on openframeworks. Using the kinect sensor we create a depth image into openframeworks
then we use the work based on ofxFlowTools to create the visuals.

Based on the following libs:
ofxKinectForWindows2Lib, ofxFlowTools

Installation
---
Openframeworks
ofxFlowTools

Building on macOS (Apple Silicon)
---
Tested on an M5 Max / macOS 26 with openFrameworks 0.12.1. The app runs on the
Apple GPU through OpenGL 4.1 core (Metal-backed); it logs the renderer it got
on startup, e.g. `renderer: programmable, GL 4.1 Metal - 90.5, Apple M5 Max`.

1. Install the Command Line Tools (`xcode-select --install`) and libusb, which
   the PS3 Eye grabber links against - the copy in `src/libusb` is an
   x86_64/i386 binary and cannot be used on Apple Silicon:

       brew install libusb

2. Download the openFrameworks 0.12.1 macOS release and build the core:

       curl -L -o of.tar.gz https://github.com/openframeworks/openFrameworks/releases/download/0.12.1/of_v0.12.1_osx_release.tar.gz
       tar xzf of.tar.gz && mv of_v0.12.1_osx_release ~/openFrameworks
       make -C ~/openFrameworks/libs/openFrameworksCompiled/project -j8 Release

3. Install the addon fork this project needs:

       git clone https://github.com/rootux/ofxFlowTools.git ~/openFrameworks/addons/ofxFlowTools

   `ofxGui`, `ofxOsc` and `ofxXmlSettings` ship with openFrameworks.

   One fix is needed in that clone for the addon to compile with a current
   clang - `ftDrawMouseForces::getTextureReference` falls off the end of the
   function when the index is out of range, which is an error rather than a
   warning now. Clamp the index to 0 and return `drawForces[_index].getTexture()`
   on every path.

4. Build and run:

       ./run_mac.sh

   `config.make` looks for openFrameworks in `~/openFrameworks`; override it
   with `OF_ROOT=/path/to/openFrameworks ./run_mac.sh`. Windows-only sources
   (`src/SpoutSDK`) are excluded from the macOS build.

Use `run_mac.sh` rather than launching the binary directly: it ad-hoc signs the
bundle and starts it through LaunchServices, without which macOS refuses the
camera request without ever showing a prompt. The first launch asks for camera
access - allow it (System Settings > Privacy & Security > Camera) and relaunch,
otherwise the visuals only react to the mouse.

There is no Kinect on macOS, so the fallback source (`z` cycles sources) is a
plain `ofVideoGrabber` on the built-in camera. A PS3 Eye, when one is plugged
in, is picked automatically at startup - see below.

Running it without building
---
Both bundles are committed, so the machine that runs the installation needs no
toolchain, no openFrameworks and no libusb - clone the repo and open the one that
matches it:

    bin/visionquest.app          Apple Silicon
    bin/visionquest-x86_64.app   Intel

They only run from inside a checkout, because they load `bin/data` next to
themselves; if you copy one elsewhere, take `bin/data` with it.

Building for both architectures
---
`./run_mac.sh` builds and runs the bundle for the machine you are on. `./build_both.sh`,
on an Apple Silicon Mac, rebuilds *both* of the bundles above - run it after
changing anything under `src/`, and commit the results.

The Intel one can be smoke-tested on an Apple Silicon Mac by just opening it,
since Rosetta runs it - that checks the build, not how fast the Intel GPU is
going to be.

There is no universal binary, because Homebrew's libusb only ever carries the
host architecture. The Intel build links the x86_64 libusb bundled in
`src/libusb` instead, and builds the openFrameworks core for x86_64 against an
APFS clone of it under `/tmp`, so your own openFrameworks tree is left alone.

The bundle is named by `APPNAME` in `config.make` rather than by the checkout
directory, so a git worktree or a renamed clone still produces `visionquest.app`.

PS3 Eye on macOS
---
The PS3 Eye goes through the bundled `PS3EYEDriver` (`src/ps3eye.cpp`) over
libusb, exactly as on Windows - the driver is portable C++ with no x86-specific
code, and libusb has a native macOS backend. Two consequences worth knowing:

* It does not touch AVFoundation, so the camera privacy prompt above does not
  apply to this source - the PS3 Eye works whether or not camera access was
  granted.
* The capture mode is set by `PS_EYE_WIDTH` / `PS_EYE_HEIGHT` / `PS_EYE_FPS` at
  the top of `src/ofApp.cpp`. The driver's rate tables allow up to 60 fps at
  640x480, and up to 187 fps at 320x240 (205 exists but is marked corrupt).
  Note `opticalFlow` runs at 320x180, so a 320x240 feed loses very little.

On a build without Kinect support the PS3 Eye is the camera the piece is for, so
it is selected automatically at startup whenever one is plugged in, and the
built-in camera is used only when there is none. `z` still cycles sources by
hand. If macOS asks whether to allow a newly connected accessory, allow it.

A camera that stops delivering - unplugged, or a USB hiccup - no longer hangs
the app. `update()` waits at most `PS_EYE_FRAME_TIMEOUT_MS` for a frame; after
`PS_EYE_STALL_SECONDS` of silence, or immediately on a failed USB transfer, it
drops the camera and falls back to the built-in one, then re-checks every
`PS_EYE_PROBE_SECONDS` and switches back when the PS3 Eye returns.

Credits & Acknowledgements
---
This is a derivative work of https://github.com/moostrik/ofxFlowTools by Matthias Oostrik
Based on ofxFX and ofxFluid - The work of ofxFluid (https://github.com/patriciogonzalezvivo/ofxFluid) - Fluid simulation with colitions based on this article of Mark Harris (http://http.developer.nvidia.com/GPUGems/gpugems_ch38.html). We use techniques describes in Stable Fluids by Jos Stem (http://www.dgp.toronto.edu/people/stam/reality/Research/pdf/ns.pdf)
