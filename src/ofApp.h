#pragma once

#ifdef _WIN32
#define _KINECT
#endif

#include <string>

#include "ofMain.h"
#ifdef _WIN32
#include "SpoutSDK\Spout.h" // Spout SDK
#endif

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
#include "ftDrawMasked.h"

#include "ofxMouse.h"

#define USE_PROGRAMMABLE_GL					// Maybe there is a reason you would want to

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
	SOURCE_KINECT = 0,
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
	void	exit();

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
	ofFbo				globalFbo;
	bool				spoutInitialized;
#ifdef _WIN32
	SpoutSender			senderSpout;
#endif

	// Time
	float				lastTime;
	float				deltaTime;
    // We will use this to know when was the last time user touched the system
    // by doing so we will go to auto pilot
    float				lastOscMessageTime;

	// FlowTools
	int				    internalWidth; // base resolution for intermediate buffers
	int					internalHeight;
	int					flowWidth; // base resolution for fluid simulation buffers
	int					flowHeight; // usually 1/4 of internalWidth/Height

	int getDrawWidth() const {
		if (sendToSpout) {
			return internalWidth;
		}
		else {
			return ofGetWindowWidth();
		}
	}

	int getDrawHeight() const {
		if (sendToSpout) {
			return internalHeight;
		}
		else {
			return ofGetWindowHeight();
		}
	}
	ftOpticalFlow		opticalFlow;
	ftVelocityMask		velocityMask;
	ftFluidSimulation	fluidSimulation;
	ftParticleFlow		particleFlow;

	ftVelocitySpheres	velocityDots;
    
    ftVelocityOffset    velocityOffset;
	ftDrawMaskedShader  drawMaskedShader;

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

	ofParameter<bool>   sendToSpout;

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

	void				sourceChanged(int& value);
	// DRAW
	ofParameter<bool>	doDrawCamBackground;

	ofParameter<int>	drawMode;
	void				drawModeSetName(int& _value);
	ofParameter<string> drawName;

	void				drawComposite() { drawComposite(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawComposite(int _x, int _y, int _width, int _height);
	void				drawParticles() { drawParticles(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawParticles(int _x, int _y, int _width, int _height);
	void				drawFluidFields() { drawFluidFields(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidFields(int _x, int _y, int _width, int _height);
	void				drawFluidDensity() { drawFluidDensity(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidDensity(int _x, int _y, int _width, int _height);
	void				drawFluidVelocity() { drawFluidVelocity(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidVelocity(int _x, int _y, int _width, int _height);
	void				drawFluidPressure() { drawFluidPressure(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidPressure(int _x, int _y, int _width, int _height);
	void				drawFluidTemperature() { drawFluidTemperature(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidTemperature(int _x, int _y, int _width, int _height);
	void				drawFluidDivergence() { drawFluidDivergence(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidDivergence(int _x, int _y, int _width, int _height);
	void				drawFluidVorticity() { drawFluidVorticity(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidVorticity(int _x, int _y, int _width, int _height);
	void				drawFluidBuoyance() { drawFluidBuoyance(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidBuoyance(int _x, int _y, int _width, int _height);
	void				drawFluidObstacle() { drawFluidObstacle(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawFluidObstacle(int _x, int _y, int _width, int _height);
	void				drawMask() { drawMask(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawMask(int _x, int _y, int _width, int _height);
	void				drawOpticalFlow() { drawOpticalFlow(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawOpticalFlow(int _x, int _y, int _width, int _height);
	void				drawSource() { drawSource(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawSource(int _x, int _y, int _width, int _height);
	void				drawMouseForces() { drawMouseForces(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawMouseForces(int _x, int _y, int _width, int _height);

	void				drawVelocityDots() { drawVelocityDots(0, 0, getDrawWidth(), getDrawHeight()); }
	void				drawVelocityDots(int _x, int _y, int _width, int _height);
    
    void				drawVelocityDisplacement() { drawVelocityDisplacement(0, 0, getDrawWidth(), getDrawHeight()); }
    void                drawVelocityDisplacement(int _x, int _y, int _width, int _height);

    // Settings group
	bool				isFileExist(string fileName);
	static string				dirnameOf(const string& fname);
	void				setRelativePath(const char *filename);
    void                setMacRelativePath(const string& filename);
	void				setOscPort(const char *filename);
	string				relateiveDataPath;
	string				relateiveKinectDataPath;
	string				relateivePsEyeDataPath;
	int					lastSaveFileCounter = 1;
	ofParameter<int>	loadSettingsFileIndex;
	int					loadSettingsFileNumber;
	int                 getNumberOfSettingsFile();
	void psEyeCameraChanged(int &index);
	void onUserOnlyKinectFilter(bool &);
	void				setLoadSettingsName(int& _value);
	void 				loadNextSettingsFile(string settingsTo);
	string				oscRemoteServerIpAddress;

	void	reset();

	// Settings Transitions
	ofParameter<int>	transitionMode;
	ofParameter<float>	transitionTime;
	ofParameter<bool>	doJumpBetweenStates;
	ofParameter<float>	jumpBetweenStatesInterval;
	float				jumpBetweenStatesStartTime;
	void				startJumpBetweenStates(bool&);
	void				updateNumberOfSettingFiles();
	void				checkIfPersonIdentified();
	int					getNumberOfTrackedBodies();
	void				updateJumpBetweenStates();
	void				updateOscMessages();
	void				startTransition(string settings1Path, string settings2Path);
	void				updateTransition();
	void				updateGuiFromTag(float timeSinceAnimationStart, string tag, string oscMsgPath = "");
	void				sendOscMessage(string oscAddress, int value);
	void				sendOscMessage(string oscAddress, float value);
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

	int					oscPort;
	ofxOscReceiver		oscReceiver;
	ofxOscSender		oscSender;
	float				timeSinceLastOscMessage; //To prevent sending too much osc messages

	ofParameter<int>	psEyeCameraIndex; //Which camera to use from multiple connected pseye camera
	ofParameter<bool>	kinectFilterUsers; // filter out users only for kinect
	ofParameter<bool>   psEyeRawOpticalFlow; // optical flow on raw data and not recolored
	ofParameter<bool>   useAgc; // automatic gain control for ps eye

	float				timeSinceLastTimeAPersonWasInFrame; // When no people is detected we can show the background

	bool				shouldStartPsEyeCam;

	void				setMousePosition(float x, float y);
    void                jumpToNextPattern();



};
