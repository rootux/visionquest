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
1.Download the latest openframeworks (Current 0.93).

2.Git clone https://github.com/rootux/ofxFlowTools into your openframeworks dir to the addons path.
into: of0.93/addons/ofxFlowTools

3.Git clone this repository https://github.com/rootux/visionquest into your openframeworks dir to the apps path.
into: of9.93/apps/myApps/visionquest

4.You can use either your webcam, KINECT V2(On windows), or PSEyeCam. to use the PSEyeCam. make sure you replace the driver by following those instructions (Windows):
To install the PS3 Driver use the following instructions:
https://github.com/cboulay/psmove-ue4/wiki/Windows-PSEye-Setup

5.Open visionquest.sln on windows OR visionquest.xcodeproj on mac

Troubleshoots
---
If you're graphic card is having issues. you can try the following fix. replace in main.cpp to GL Version 3.3
```
#ifdef USE_PROGRAMMABLE_GL
	windowSettings.setGLVersion(3, 3);
#endif
```

If the app is frozen when starting or when moving to PSEye source perhpas the USB port is not compatible with the PsEye camera. Plug it off and try a different port. install the libusb-win driver using zadiag (Instructions above) - if still doesn't work try again with a different port - this will save you time and frustrations.

Credits & Acknowledgements
---
This is a derivative work of https://github.com/moostrik/ofxFlowTools by Matthias Oostrik
Based on ofxFX and ofxFluid - The work of ofxFluid (https://github.com/patriciogonzalezvivo/ofxFluid) - Fluid simulation with colitions based on this article of Mark Harris (http://http.developer.nvidia.com/GPUGems/gpugems_ch38.html). We use techniques describes in Stable Fluids by Jos Stem (http://www.dgp.toronto.edu/people/stam/reality/Research/pdf/ns.pdf)
