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

There is no Kinect on macOS, so the default source (`z` cycles sources) is a
plain `ofVideoGrabber` on the built-in camera.

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

Press `z` to select the PS3 Eye source; macOS never auto-selects it. If macOS
asks whether to allow a newly connected accessory, allow it.

Credits & Acknowledgements
---
This is a derivative work of https://github.com/moostrik/ofxFlowTools by Matthias Oostrik
Based on ofxFX and ofxFluid - The work of ofxFluid (https://github.com/patriciogonzalezvivo/ofxFluid) - Fluid simulation with colitions based on this article of Mark Harris (http://http.developer.nvidia.com/GPUGems/gpugems_ch38.html). We use techniques describes in Stable Fluids by Jos Stem (http://www.dgp.toronto.edu/people/stam/reality/Research/pdf/ns.pdf)
