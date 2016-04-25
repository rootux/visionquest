#pragma once

#ifdef _WIN32
#define _KINECT
#endif

#include <string>

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxFlowTools.h"
#include "ofxXmlSettings.h"
#include "ofxOsc.h"

#ifdef _KINECT
#include "ofxKinectForWindows2.h"
#endif

#include "ps3eye.h"

#include "ofxRecolor.h"
#include "ftVelocityOffset.h"

#define USE_PROGRAMMABLE_GL					// Maybe there is a reason you would want to

#define PORT 10000
#define PORT_SERVER 10001

using namespace flowTools;

enum drawModeEnum {
	DRAW_COMPOSITE = 0,
	DRAW_FLUID_DENSITY,
	DRAW_PARTICLES,
	DRAW_VELDOTS,
	DRAW_FLUID_FIELDS,
	DRAW_FLUID_VELOCITY,        
	DRAW_FLUID_PRESSURE,
	DRAW_FLUID_TEMPERATURE,
	DRAW_FLUID_DIVERGENCE,
	DRAW_FLUID_VORTICITY,
	DRAW_FLUID_BUOYANCY,
	DRAW_FLUID_OBSTACLE,
	DRAW_FLOW_MASK,
	DRAW_OPTICAL_FLOW,
	DRAW_SOURCE,
	DRAW_MOUSE,
    DRAW_DISPLACEMENT,
    DRAW_COUNT,
};

enum sourceModeEnum {
	SOURCE_KINECT_PS3EYE = 0,
	SOURCE_KINECT_DEPTH_PS3EYE,
	SOURCE_KINECT,
	SOURCE_PS3EYE,

	SOURCE_COUNT
};

enum transitionModeEnum {
	TRANSITION_NONE = 0,
	TRANSITION_LINEAR,

	TRANSITION_COUNT
};

class ofApp : public ofBaseApp {
public:
	void	setup();
	void	setupPsEye();
	void	update();
	void	draw();

	// Camera
	ofVideoGrabber		simpleCam;
#ifdef _KINECT
	ofxKFW2::Device		kinect;
	ftFbo				kinectFbo;
#endif
	ps3eye::PS3EYECam::PS3EYERef eye = NULL;
	unsigned char *		videoFrame;
	ofTexture			videoTexture;

	bool				isKinectSource();
	bool				isPsEyeSource();
	bool				isKinectAndPsEyeSource();


	bool				didCamUpdate;
	ftFbo				cameraFbo;
	ofParameter<bool>	doFlipCamera;


	// Time
	float				lastTime;
	float				deltaTime;

	// FlowTools
	int					flowWidth;
	int					flowHeight;
	int					drawWidth;
	int					drawHeight;

	ftOpticalFlow		opticalFlow;
	ftVelocityMask		velocityMask;
	ftFluidSimulation	fluidSimulation;
	ftParticleFlow		particleFlow;

	ftVelocitySpheres	velocityDots;
    
    ftVelocityOffset    velocityOffset;

	ofImage				flowToolsLogoImage;
	bool				showLogo;

	// MouseDraw
	ftDrawMouseForces	mouseForces;

	// Visualisations
	ofParameterGroup	visualizeParameters;
	ftDisplayScalar		displayScalar;
	ftVelocityField		velocityField;
	ftTemperatureField	temperatureField;
	ftPressureField		pressureField;
	ftVTField			velocityTemperatureField;

	// Source - Kinect, Ps3
	ofParameter<int>	sourceMode;
    
    ofxRecolor          recolor;

	ofParameter<bool>	showScalar;
	ofParameter<bool>	showField;
	ofParameter<float>	displayScalarScale;
	void				setDisplayScalarScale(float& _value) { displayScalar.setScale(_value); }
	ofParameter<float>	velocityFieldScale;
	void				setVelocityFieldScale(float& _value) { velocityField.setVelocityScale(_value); velocityTemperatureField.setVelocityScale(_value); }
	ofParameter<float>	temperatureFieldScale;
	void				setTemperatureFieldScale(float& _value) { temperatureField.setTemperatureScale(_value); velocityTemperatureField.setTemperatureScale(_value); }
	ofParameter<float>	pressureFieldScale;
	void				setPressureFieldScale(float& _value) { pressureField.setPressureScale(_value); }
	ofParameter<bool>	velocityLineSmooth;
	void				setVelocityLineSmooth(bool& _value) { velocityField.setLineSmooth(_value); velocityTemperatureField.setLineSmooth(_value); }

	// GUI
	ofxPanel			gui;
	void				setupGui();
	void				keyPressed(int key);
	void				increaseParameter(ofParameter<float> parameter, float val, float max=1);
	void				decreaseParameter(ofParameter<float> parameter, float val, float min=0);
	void				jumpToNextEffect();
	void				drawGui();
	ofParameter<bool>	toggleGuiDraw;
	ofParameter<float>	guiFPS;
	ofParameter<float>	guiMinFPS;
	deque<float>		deltaTimeDeque;
	ofParameter<bool>	doFullScreen;
	void				setFullScreen(bool& _value) { ofSetFullscreen(_value); }

	ofParameterGroup	settingsGroup;
	ofxButton			multiSaveButton;

	void				MultiSavePressed(const void * sender);

	// DRAW
	ofParameter<bool>	doDrawCamBackground;

	ofParameter<int>	drawMode;
	void				drawModeSetName(int& _value);
	ofParameter<string> drawName;

	void				drawComposite() { drawComposite(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawComposite(int _x, int _y, int _width, int _height);
	void				drawParticles() { drawParticles(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawParticles(int _x, int _y, int _width, int _height);
	void				drawFluidFields() { drawFluidFields(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidFields(int _x, int _y, int _width, int _height);
	void				drawFluidDensity() { drawFluidDensity(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidDensity(int _x, int _y, int _width, int _height);
	void				drawFluidVelocity() { drawFluidVelocity(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidVelocity(int _x, int _y, int _width, int _height);
	void				drawFluidPressure() { drawFluidPressure(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidPressure(int _x, int _y, int _width, int _height);
	void				drawFluidTemperature() { drawFluidTemperature(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidTemperature(int _x, int _y, int _width, int _height);
	void				drawFluidDivergence() { drawFluidDivergence(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidDivergence(int _x, int _y, int _width, int _height);
	void				drawFluidVorticity() { drawFluidVorticity(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidVorticity(int _x, int _y, int _width, int _height);
	void				drawFluidBuoyance() { drawFluidBuoyance(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidBuoyance(int _x, int _y, int _width, int _height);
	void				drawFluidObstacle() { drawFluidObstacle(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawFluidObstacle(int _x, int _y, int _width, int _height);
	void				drawMask() { drawMask(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawMask(int _x, int _y, int _width, int _height);
	void				drawOpticalFlow() { drawOpticalFlow(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawOpticalFlow(int _x, int _y, int _width, int _height);
	void				drawSource() { drawSource(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawSource(int _x, int _y, int _width, int _height);
	void				drawMouseForces() { drawMouseForces(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawMouseForces(int _x, int _y, int _width, int _height);

	void				drawVelocityDots() { drawVelocityDots(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
	void				drawVelocityDots(int _x, int _y, int _width, int _height);
    
    void				drawVelocityDisplacement() { drawVelocityDisplacement(0, 0, ofGetWindowWidth(), ofGetWindowHeight()); }
    void                drawVelocityDisplacement(int _x, int _y, int _width, int _height);

    // Settings group
	bool				isFileExist(string fileName);
	string				dirnameOf(const string& fname);
	void				setRelativePath(const char *filename);
	string				relateiveDataPath;
	int					lastSaveFileCounter = 1;
	ofParameter<int>	loadSettingsFileIndex;
	int					loadSettingsFileNumber;
	int                 getNumberOfSettingsFile();
	void				setLoadSettingsName(int& _value);
	void 				loadNextSettingsFile(string settingsTo);

	void	reset();

	// Settings Transitions
	ofParameter<int>	transitionMode;
	ofParameter<float>	transitionTime;
	ofParameter<bool>	doJumpBetweenStates;
	ofParameter<float>	transitionStatesInterval;
	void				updateOscMessages();
	void				startTransition(string settings1Path, string settings2Path);
	void				updateTransition();
	void				updateGuiFromTag(float timeSinceAnimationStart, string tag, string oscMsgPath = "");
	float				transitionStartTime;
	ofxXmlSettings		*settingsFrom;
	string				settingsFromPath;
	ofxXmlSettings		*settingsTo;
	string				settingsToPath;
	double				getValueTransitionStep(string tagName, double amount);
	string				getValueAsString(string tagName);
	bool				getValueTransitionStep(string tagName, bool amount);
	int					getValueTransitionStep(string tagName, int amount);
	int					getNextSettingsCounter();
	bool				isTransitionFinished = true;

	ofxOscReceiver		oscReceiver;
	ofxOscSender		oscSender;
};
