#include "ofApp.h"

static const int ITUR_BT_601_CY = 1220542;
static const int ITUR_BT_601_CUB = 2116026;
static const int ITUR_BT_601_CUG = -409993;
static const int ITUR_BT_601_CVG = -852492;
static const int ITUR_BT_601_CVR = 1673527;
static const int ITUR_BT_601_SHIFT = 20;

#define TIMEOUT_KINECT_PEOPLE_FILTER 5
#define TIMEIN_KINECT_PEOPLE_FILTER 3
#define AUTO_PILOT_TIMEOUT 10

// a pretty useful tokenization systes. equal to str.split
// Example: For the given "settings:recolor:Cutoff" will split to 3 items
static vector<string> tokenize(const string & str, const string & delim);
static vector<string> tokenize(const string & str, const string & delim)
{
	vector<string> tokens;

	size_t p0 = 0, p1 = string::npos;
	while (p0 != string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if (p1 != p0)
		{
			string token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;
}

void ofApp::setup() {
	ofSetEscapeQuitsApp(false);
	ofSetVerticalSync(false);
	ofSetLogLevel(OF_LOG_NOTICE);

	internalWidth = 1280;
	internalHeight = 720;
	// process all but the density on 16th resolution
	flowWidth = internalWidth / 4;
	flowHeight = internalHeight / 4;

	// FLOW & MASK
	opticalFlow.setup(flowWidth, flowHeight);
	velocityMask.setup(internalWidth, internalHeight);

	// FLUID & PARTICLES
	fluidSimulation.setup(flowWidth, flowHeight, internalWidth, internalHeight);
	particleFlow.setup(flowWidth, flowHeight, internalWidth, internalHeight);

	//flowToolsLogoImage.load("flowtools.png");
	//fluidSimulation.addObstacle(flowToolsLogoImage.getTexture());
	showLogo = false;

	velocityDots.setup(flowWidth / 4, flowHeight / 4);

	// VISUALIZATION
	displayScalar.setup(flowWidth, flowHeight);
	velocityField.setup(flowWidth / 4, flowHeight / 4);
	temperatureField.setup(flowWidth / 4, flowHeight / 4);
	pressureField.setup(flowWidth / 4, flowHeight / 4);
	velocityTemperatureField.setup(flowWidth / 4, flowHeight / 4);
	velocityOffset.allocate(flowWidth / 2, flowHeight / 2);

	// MOUSE DRAW
	mouseForces.setup(flowWidth, flowHeight, internalWidth, internalHeight);

	// CAMERA
	simpleCam.setup(640, 480, true);
    
    

#ifdef _KINECT
	// KINECT
	kinect.open();
	kinect.initDepthSource();
	kinect.initColorSource();
	kinect.initInfraredSource();
	kinect.initBodySource();
	kinect.initBodyIndexSource();
	kinectFbo.allocate(internalWidth, internalHeight, GL_R16);
	kinectFbo.getTexture().setRGToRGBASwizzles(true);
	ofLogError("kinect inited");
#endif

	didCamUpdate = false;
	cameraFbo.allocate(internalWidth, internalHeight);
	cameraFbo.black();

	globalFbo.allocate(internalWidth, internalHeight);

	recolor.setup();

	// GUI
	setupGui();

	lastTime = ofGetElapsedTimef();

	doFullScreen.set(0);

	oscReceiver.setup(oscPort);
    lastOscMessageTime = ofGetElapsedTimef();
	if (shouldStartPsEyeCam) {
		sourceMode = SOURCE_PS3EYE;
	}
}

void ofApp::setupPsEye() {
	try {
		using namespace ps3eye;
		std::vector<PS3EYECam::PS3EYERef> devices(PS3EYECam::getDevices());
		if (devices.size())
		{
			// Only stop eye if eye is working and more then one camera is connected
			if (eye && devices.size() > 1) {
				eye->stop();
				//eye = NULL;
			}

			psEyeCameraIndex.setMax(devices.size() - 1);
			int psEyeCameraToUse = 0;
			if (devices.size() > psEyeCameraIndex) {
				psEyeCameraToUse = psEyeCameraIndex;
			}
			
			// Init a new eye only if eye is not set or if devices is bigger then 1
			if (!eye || devices.size() > 1) {
				eye = devices.at(psEyeCameraToUse);
				bool res = eye->init(640, 480, 60);
				if (res) {
					eye->start();
					eye->setExposure(125); //TODO: was 255
					eye->setAutogain(useAgc);

					videoFrame = new unsigned char[eye->getWidth()*eye->getHeight() * 4];
					videoTexture.allocate(eye->getWidth(), eye->getHeight(), GL_RGB);
				}
				else {
					eye = NULL;
				}
			}
		}
		else {
			ofLogError() << "Failed to open PS eye!";
		}
	}
	catch (...) {
		ofLogError() << "Failed to open PS eye. Exception.";
		sourceMode.set((sourceMode.get() + 1) % SOURCE_COUNT);
	}
}

//for settings:recolor:cutoff will  return settings
static string getNextTagNode(string tag, string &firstTagName) {
	vector<string> tokens = tokenize(tag, ":");
	//no tags so we return
	if (tokens.size() == 0) return ""; // No more tokens
	firstTagName = string(tokens[0]);
	if (tokens.size() == 1) return ""; //Last one
	int count = tag.length() - firstTagName.length();
	return tag.substr(firstTagName.length() + 1, count-1);
}

static bool isOnlyDouble(const char* str) {
	char* endptr = 0;
	strtod(str, &endptr);

	if (*endptr != '\0' || endptr == str)
		return false;
	return true;
}

static void yuv422_to_rgba(const uint8_t *yuv_src, const int stride, uint8_t *dst, const int width, const int height)
{
	const int bIdx = 2;
	const int uIdx = 0;
	const int yIdx = 0;

	const int uidx = 1 - yIdx + uIdx * 2;
	const int vidx = (2 + uidx) % 4;
	int j, i;

#define _max(a, b) (((a) > (b)) ? (a) : (b))
#define _saturate(v) static_cast<uint8_t>(static_cast<uint32_t>(v) <= 0xff ? v : v > 0 ? 0xff : 0)

	for (j = 0; j < height; j++, yuv_src += stride)
	{
		uint8_t* row = dst + (width * 4) * j; // 4 channels

		for (i = 0; i < 2 * width; i += 4, row += 8)
		{
			int u = static_cast<int>(yuv_src[i + uidx]) - 128;
			int v = static_cast<int>(yuv_src[i + vidx]) - 128;

			int ruv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVR * v;
			int guv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CVG * v + ITUR_BT_601_CUG * u;
			int buv = (1 << (ITUR_BT_601_SHIFT - 1)) + ITUR_BT_601_CUB * u;

			int y00 = _max(0, static_cast<int>(yuv_src[i + yIdx]) - 16) * ITUR_BT_601_CY;
			row[2 - bIdx] = _saturate((y00 + ruv) >> ITUR_BT_601_SHIFT);
			row[1] = _saturate((y00 + guv) >> ITUR_BT_601_SHIFT);
			row[bIdx] = _saturate((y00 + buv) >> ITUR_BT_601_SHIFT);
			row[3] = (0xff);

			int y01 = _max(0, static_cast<int>(yuv_src[i + yIdx + 2]) - 16) * ITUR_BT_601_CY;
			row[6 - bIdx] = _saturate((y01 + ruv) >> ITUR_BT_601_SHIFT);
			row[5] = _saturate((y01 + guv) >> ITUR_BT_601_SHIFT);
			row[4 + bIdx] = _saturate((y01 + buv) >> ITUR_BT_601_SHIFT);
			row[7] = (0xff);
		}
	}
}
//--------------------------------------------------------------
void ofApp::setupGui() {
	gui.setup("settings");
	gui.setDefaultBackgroundColor(ofColor(0, 0, 0, 127));
	gui.setDefaultFillColor(ofColor(160, 160, 160, 160));
	gui.add(multiSaveButton.setup("Multiple Save Settings"));
	multiSaveButton.addListener(this, &ofApp::MultiSavePressed);
	loadSettingsFileNumber = getNumberOfSettingsFile();

	settingsGroup.setName("settings transition");
	settingsGroup.add(loadSettingsFileIndex.set("Settings file", 1, 1, loadSettingsFileNumber));
	loadSettingsFileIndex.addListener(this, &ofApp::setLoadSettingsName);

	settingsGroup.add(transitionMode.set("Transition mode", TRANSITION_NONE, TRANSITION_NONE, TRANSITION_COUNT - 1));
	settingsGroup.add(transitionTime.set("Transition time", 0, 0, 720));
	settingsGroup.add(doJumpBetweenStates.set("Jump between states", false));
	doJumpBetweenStates.addListener(this, &ofApp::startJumpBetweenStates);
	settingsGroup.add(jumpBetweenStatesInterval.set("Jump between interval", 0, 0, 720));

	gui.add(settingsGroup);

	gui.add(guiFPS.set("average FPS", 0, 0, 60));
	gui.add(guiMinFPS.set("minimum FPS", 0, 0, 60));
	gui.add(doFullScreen.set("fullscreen (F)", false));
	doFullScreen.addListener(this, &ofApp::setFullScreen);
	gui.add(toggleGuiDraw.set("show gui (G)", false));
	gui.add(doFlipCamera.set("flip camera", true));
	gui.add(doDrawCamBackground.set("draw camera (C)", true));
#ifdef _WIN32
	gui.add(sendToSpout.set("Send to Spout", false));
#else
	sendToSpout.set(false);
#endif
	gui.add(drawMode.set("draw mode", DRAW_COMPOSITE, DRAW_COMPOSITE, DRAW_COUNT - 1));
	drawMode.addListener(this, &ofApp::drawModeSetName);
	gui.add(drawName.set("MODE", "draw name"));
	gui.add(sourceMode.set("Source mode (z)", SOURCE_KINECT, SOURCE_PS3EYE, SOURCE_COUNT - 1));
	gui.add(psEyeCameraIndex.set("PsEye Camera num (x)", 0, 0, 2));
	gui.add(psEyeRawOpticalFlow.set("psEye raw flow", true));
	gui.add(useAgc.set("psEye AGC", false));
	gui.add(kinectFilterUsers.set("Users-only kinect filter", false));
	kinectFilterUsers.addListener(this, &ofApp::onUserOnlyKinectFilter);
	psEyeCameraIndex.addListener(this, &ofApp::psEyeCameraChanged);
	sourceMode.addListener(this, &ofApp::sourceChanged);

	int guiColorSwitch = 0;
	ofColor guiHeaderColor[2];
	guiHeaderColor[0].set(160, 160, 80, 200);
	guiHeaderColor[1].set(80, 160, 160, 200);
	ofColor guiFillColor[2];
	guiFillColor[0].set(160, 160, 80, 200);
	guiFillColor[1].set(80, 160, 160, 200);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(opticalFlow.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(recolor.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(velocityMask.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(fluidSimulation.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(particleFlow.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(mouseForces.leftButtonParameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(mouseForces.rightButtonParameters);

	visualizeParameters.setName("visualizers");
	visualizeParameters.add(showScalar.set("show scalar", true));
	visualizeParameters.add(showField.set("show field", true));
	visualizeParameters.add(displayScalarScale.set("scalar scale", 0.15, 0.05, 1.0));
	displayScalarScale.addListener(this, &ofApp::setDisplayScalarScale);
	visualizeParameters.add(velocityFieldScale.set("velocity scale", 0.1, 0.0, 0.5));
	velocityFieldScale.addListener(this, &ofApp::setVelocityFieldScale);
	visualizeParameters.add(temperatureFieldScale.set("temperature scale", 0.1, 0.0, 0.5));
	temperatureFieldScale.addListener(this, &ofApp::setTemperatureFieldScale);
	visualizeParameters.add(pressureFieldScale.set("pressure scale", 0.02, 0.0, 0.5));
	pressureFieldScale.addListener(this, &ofApp::setPressureFieldScale);
	visualizeParameters.add(velocityLineSmooth.set("line smooth", false));
	velocityLineSmooth.addListener(this, &ofApp::setVelocityLineSmooth);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(visualizeParameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(velocityDots.parameters);

	gui.setDefaultHeaderBackgroundColor(guiHeaderColor[guiColorSwitch]);
	gui.setDefaultFillColor(guiFillColor[guiColorSwitch]);
	guiColorSwitch = 1 - guiColorSwitch;
	gui.add(visualizeParameters);

	// if the settings file is not present the parameters will not be set during this setup
	if (!ofFile("settings.xml"))
		gui.saveToFile("settings.xml");

	gui.loadFromFile("settings.xml");

	gui.minimizeAll();
	toggleGuiDraw = false;

}

int ofApp::getNumberOfSettingsFile() {
	int counter = 1;
	while (isFileExist(relateiveDataPath + "settings" + std::to_string(counter) + ".xml")) {
		counter++;
	}
	return counter;
}

//TODO: create a generic listener that will connect an osc path to send whanever this parameter changes
//void ofApp::connectParameterToOsc() {
	//
//}

void ofApp::psEyeCameraChanged(int& index) {
	if (isPsEyeSource()) {
		setupPsEye();
	}
}

void ofApp::onUserOnlyKinectFilter(bool& isOn) {
	if (isOn) {
		//reset last time a person was in frame
		timeSinceLastTimeAPersonWasInFrame = ofGetElapsedTimef();

		//Only if there is a person set it on otherwise turn it back off
		if (ofApp::getNumberOfTrackedBodies() <= 0) {
			kinectFilterUsers.set(false);
		}
	}
	else {
		//reset last time a person was not in frame
		timeSinceLastTimeAPersonWasInFrame = ofGetElapsedTimef() -TIMEOUT_KINECT_PEOPLE_FILTER; 
	}
}

/*
Whenever a source is changed we are loading a different settins file.
one from bin/data for the kinect
second from bin/data/pseyesettings for the pseye
*/
void ofApp::sourceChanged(int& mode) {
	switch (mode) {
	case SOURCE_KINECT:
		relateiveDataPath = relateiveKinectDataPath;
		ofLogWarning("Switched to Kinect");
		break;
	case SOURCE_PS3EYE:		
		relateiveDataPath = relateivePsEyeDataPath;
		ofLogWarning("Switched to PsEye");
		break;
	}
	updateNumberOfSettingFiles();
}

bool ofApp::isKinectSource() {
	return (sourceMode.get() == SOURCE_KINECT);
}

bool ofApp::isPsEyeSource() {
	return (sourceMode.get() == SOURCE_PS3EYE);
}

//--------------------------------------------------------------
void ofApp::update() {

	deltaTime = ofGetElapsedTimef() - lastTime;
	lastTime = ofGetElapsedTimef();

	simpleCam.update();
#ifdef _KINECT
	if (isKinectSource()) {
		kinect.update(); 
	}
#endif

	if(isPsEyeSource()) {
		if (!eye) {
			setupPsEye();
		}
	}

	if (isPsEyeSource() && eye)
	{
		try {
			uint8_t* new_pixels = eye->getFrame();
			yuv422_to_rgba(new_pixels, eye->getRowBytes(), videoFrame, eye->getWidth(), eye->getHeight());
			videoTexture.loadData(videoFrame, eye->getWidth(), eye->getHeight(), GL_RGBA);
			free(new_pixels);
		}
		catch (...) {
			ofLogWarning("Can't open ps eye. exception. moving to kinect");
			sourceMode.set((sourceMode.get() + 1) % SOURCE_COUNT);
		}
	}
#ifdef _KINECT
	if ((isKinectSource() && (kinect.getDepthSource()->isFrameNew())) ||
		(isPsEyeSource() && eye)) {
#else
	if ((isPsEyeSource() && eye) || simpleCam.isFrameNew()) {
#endif

		ofTexture *videoSource;
		switch (sourceMode) {
#ifdef _KINECT
		case SOURCE_KINECT:
		{
			checkIfPersonIdentified();

			int tracked = kinect.getBodySource()->getBodyCount();
			if (kinectFilterUsers.get()) {
				drawMaskedShader.update(kinectFbo, kinect.getDepthSource()->getTexture(), kinect.getBodyIndexSource()->getTexture(), tracked);
				videoSource = &kinectFbo.getTexture();
			}
			else {
				videoSource = &kinect.getDepthSource()->getTexture();
			}
		}
			break;
#endif
		case SOURCE_PS3EYE:
			videoSource = &videoTexture;
			break;
		default:
			videoSource = &simpleCam.getTexture();
			break;
		}

		ofPushStyle();
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);

		recolor.update(cameraFbo, *videoSource, doFlipCamera);

		ofPopStyle();
		// TODO: figure out how to use kinectFbo for this on kinect and to have it work
		if ((sourceMode == SOURCE_PS3EYE) && (psEyeRawOpticalFlow.get())) {
			opticalFlow.setSource(videoTexture);
		}
		else {
			opticalFlow.setSource(cameraFbo.getTexture());
		}

		//opticalFlow.update(deltaTime);
		// use internal deltatime instead
		opticalFlow.update();


		velocityMask.setDensity(cameraFbo.getTexture());
		velocityMask.setVelocity(opticalFlow.getOpticalFlow());
		velocityMask.update();
	}


	fluidSimulation.addVelocity(opticalFlow.getOpticalFlowDecay());  //!
																	 //fluidSimulation.addVelocity(cameraFbo.getTexture()); //!
	fluidSimulation.addDensity(velocityMask.getColorMask());
	fluidSimulation.addTemperature(velocityMask.getLuminanceMask());

	mouseForces.update(deltaTime);

	for (int i = 0; i<mouseForces.getNumForces(); i++) {
		if (mouseForces.didChange(i)) {
			switch (mouseForces.getType(i)) {
			case FT_DENSITY:
				fluidSimulation.addDensity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
				break;
			case FT_VELOCITY:
				fluidSimulation.addVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
				particleFlow.addFlowVelocity(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
				break;
			case FT_TEMPERATURE:
				fluidSimulation.addTemperature(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
				break;
			case FT_PRESSURE:
				fluidSimulation.addPressure(mouseForces.getTextureReference(i), mouseForces.getStrength(i));
				break;
			case FT_OBSTACLE:
				fluidSimulation.addTempObstacle(mouseForces.getTextureReference(i));
			default:
				break;
			}
		}
	}

	fluidSimulation.update();

	if (particleFlow.isActive()) {
		particleFlow.setSpeed(fluidSimulation.getSpeed());
		particleFlow.setCellSize(fluidSimulation.getCellSize());
		particleFlow.addFlowVelocity(opticalFlow.getOpticalFlow());
		particleFlow.addFluidVelocity(fluidSimulation.getVelocity());
		//		particleFlow.addDensity(fluidSimulation.getDensity());
		particleFlow.setObstacle(fluidSimulation.getObstacle());
	}
	particleFlow.update();

	updateTransition();

	updateOscMessages();

	updateJumpBetweenStates();
}

void ofApp::checkIfPersonIdentified() {
	//Update time of last person identified
	//Sample every 4 seconds -> TODO now sample from 4.0 4.1 and so on till 5.0 need to sample only once
	if ((int)ofGetElapsedTimef() % 4 == 0) {
		int realTrackedPerson = getNumberOfTrackedBodies();

		if (realTrackedPerson > 0) {
			timeSinceLastTimeAPersonWasInFrame = ofGetElapsedTimef();
		}
	}

	float delta = ofGetElapsedTimef() - timeSinceLastTimeAPersonWasInFrame;

	//Only on auto pilot (doJumpBetweenStates) we set those modes
	if (delta >= TIMEOUT_KINECT_PEOPLE_FILTER && kinectFilterUsers.get() && doJumpBetweenStates) {
		ofLogWarning("No person found. moving to background mode");
		kinectFilterUsers.set(false);
		return;
	}

	//Only on auto pilot (doJumpBetweenStates) we set those modes
	if (delta < TIMEIN_KINECT_PEOPLE_FILTER && !kinectFilterUsers.get() && doJumpBetweenStates) {
		ofLogWarning("Person found. Removing background");
		kinectFilterUsers.set(true);
	}
}

int ofApp::getNumberOfTrackedBodies() {
	int result = 0;
	const vector<ofxKinectForWindows2::Data::Body> bodies = kinect.getBodySource()->getBodies();
	for (int i = 0; i < bodies.size(); i++) {
		result += (bodies[i].tracked ? 1 : 0);
	}
	return result;
}

void ofApp::updateJumpBetweenStates() {
	if (!doJumpBetweenStates || jumpBetweenStatesInterval <= 0) {
		return;
	}
	float timeSinceAnimationStart = ofGetElapsedTimef() - transitionStartTime;
	// Check if animation completed
	if (timeSinceAnimationStart >= jumpBetweenStatesInterval) {
		jumpToNextEffect();
		bool dummy;
		startJumpBetweenStates(dummy);
		return;
	}
}

void ofApp::updateOscMessages() {
	while (oscReceiver.hasWaitingMessages()) {
		ofxOscMessage m;
		oscReceiver.getNextMessage(&m);
		
        lastOscMessageTime = ofGetElapsedTimef();
		// Set remote address by osc message
		if (!m.getRemoteIp().empty() && oscRemoteServerIpAddress.empty()) {
			oscRemoteServerIpAddress = m.getRemoteIp();
			//oscSender.setup(oscRemoteServerIpAddress, PORT_SERVER);
		}

		if (m.getAddress() == "helo") {
			
		}
			
		if (m.getAddress() == "/1/strength") {
			opticalFlow.setStrength(m.getArgAsFloat(0));
		}
		if (m.getAddress() == "/1/speed") {
			fluidSimulation.setSpeed(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/cutoff") {
			recolor.cutoff.set(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/draw_camera" &&
			m.getArgAsBool(0) == true) {
			doDrawCamBackground.set(!doDrawCamBackground.get());
		}

		if (m.getAddress() == "/1/stretch" &&
			m.getArgAsBool(0) == true) {
			particleFlow.bStretch.set(!particleFlow.bStretch);
		}

		if (m.getAddress() == "/1/spawn_hue") {
			particleFlow.spawnHue.set(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/over_color") {
			velocityMask.hueOffset.set(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/particle_size") {
			particleFlow.size.set(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/reset" &&
			m.getArgAsBool(0) == true) {
			reset();
		}

		if (m.getAddress() == "/1/source" &&
			m.getArgAsBool(0) == true) {
			sourceMode.set((sourceMode.get() + 1) % SOURCE_COUNT);
		}

		if ((m.getAddress().find("/1/effects") != std::string::npos) &&
			(m.getArgAsBool(0) == true)) {
			int mode = stoi(m.getAddress().substr(m.getAddress().find("1", 2) + 2)); //find the next /1
			switch (mode) {
				case 0: drawMode.set(DRAW_COMPOSITE); break;
				case 1: drawMode.set(DRAW_COMPOSITE); break;
				case 2: drawMode.set(DRAW_FLUID_DENSITY); break;
				case 3: drawMode.set(DRAW_PARTICLES); break;
				case 4: drawMode.set(DRAW_DISPLACEMENT); break;
			}
		}
        
        if (m.getAddress() == "/1/animate_scale") {
            recolor.animateScale.set(m.getArgAsBool(0));
			recolor.animateOffset.set(m.getArgAsBool(0));
        }

		if (m.getAddress() == "/1/gravity_y") {
			ofVec2f gravity = fluidSimulation.getGravity();
			fluidSimulation.setGravity(ofVec2f(gravity.x, m.getArgAsFloat(0)));
		}

		if (m.getAddress() == "/1/dissipation") {
			fluidSimulation.setDissipation(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/1/next_effect" &&
			m.getArgAsBool(0) == true) {
			jumpToNextEffect();
		}

		if (m.getAddress() == "/1/ir_jump" &&
			m.getArgAsBool(0) == true) {
			if (sourceMode != SOURCE_PS3EYE) {
				sourceMode = SOURCE_PS3EYE;
			}
			else {
				psEyeCameraIndex.set((psEyeCameraIndex.get() + 1) % (psEyeCameraIndex.getMax() + 1));
			}
		}

		if (m.getAddress() == "/1/ir_exposure") {
			if (eye) {
				eye->setExposure(m.getArgAsFloat(0));
			}
		}

		if (m.getAddress() == "/1/ir_gain") {
			if (eye) {
				eye->setGain(m.getArgAsFloat(0));
			}
		}

		if (m.getAddress() == "/1/ir_hue") {
			if (eye) {
				eye->setHue(m.getArgAsFloat(0));
			}
		}

		if (m.getAddress() == "/1/kinect_filter_users") {
			kinectFilterUsers.set(m.getArgAsBool(0));
		}

		if (m.getAddress() == "/1/ps_eye_raw_optical_flow") {
			psEyeRawOpticalFlow.set(m.getArgAsBool(0));
		}

		if (m.getAddress() == "/settings/transition_time" &&
            m.getArgAsBool(0) == true) {
			transitionTime.set(m.getArgAsFloat(0));
		}

		if (m.getAddress() == "/settings/jump_between_states_time") {
			jumpBetweenStatesInterval.set(m.getArgAsFloat(0));
			
			//TODO: check why after 2 minutes its Crashing the system
			//float now = ofGetElapsedTimef();
			//if (timeSinceLastOscMessage < now - 2) { //2 seconds thrashold between message
			//	sendOscMessage("/settings/animation_time", m.getArgAsFloat(0));
			//	timeSinceLastOscMessage = now;
			//}
		}

		if (m.getAddress() == "/settings/animate") {
			doJumpBetweenStates.set(m.getArgAsBool(0));
		}

		if ((m.getAddress().find("/settings/jump_to_setting") != std::string::npos) &&
			m.getArgAsBool(0) == true) {

			int startOfRow = m.getAddress().find("jump_to_setting") + std::string("jump_to_setting/").length();
			int row = stoi(m.getAddress().substr(startOfRow,1));
			//int startOfCol = m.getAddress().find("/", startOfRow);
			int col = stoi(m.getAddress().substr(m.getAddress().find("/", startOfRow)+1));
			int oldSettingsFileIndex = loadSettingsFileIndex;
			//Map the matrix of rows and cols to file number
			loadSettingsFileIndex = ((row-1) * 6) + col; //6 is length of line
			//TODO: DRY
			//Transition from current setting to the next one
			startTransition(relateiveDataPath + "settings" + std::to_string(oldSettingsFileIndex) + ".xml",
				relateiveDataPath + "settings" + std::to_string(loadSettingsFileIndex) + ".xml");
		}

		if (m.getAddress() == "/settings/ir_autogain") {
			useAgc.set(m.getArgAsBool(0));
			if (eye) {
				eye->setAutogain(useAgc);
			}
		}
        
        //If the user send a manual command - auto pilot will turn off
        if(doJumpBetweenStates == 1) {
            ofLogWarning("User took control. got osc message. auto pilot turned off");
            doJumpBetweenStates.set(0);
        }
	}
    
    //Activate auto pilot to on if no message was received for a given period (30 seconds) and no auto pilot is set yet
    float timeSinceLastMessage = ofGetElapsedTimef() - lastOscMessageTime;
    if(timeSinceLastMessage >= AUTO_PILOT_TIMEOUT && doJumpBetweenStates.get() != 1) {
        lastOscMessageTime = ofGetElapsedTimef();
        doJumpBetweenStates.set(1);
        ofLogWarning("No osc message received for the last 15 seconds. moving to auto pilot");
    }
}


void ofApp::startTransition(string settings1Path, string settings2Path) {
	transitionStartTime = ofGetElapsedTimef();
	settingsFrom = new ofxXmlSettings(settings1Path);
	settingsFromPath = settings1Path;
	settingsTo = new ofxXmlSettings(settings2Path);
	settingsToPath = settings2Path;
	isTransitionFinished = false;
}


void ofApp::updateTransition() {
	if (isTransitionFinished)
		return;

	float timeSinceAnimationStart = ofGetElapsedTimef() - transitionStartTime;

	// Check if animation completed
	if (timeSinceAnimationStart >= transitionTime) {
		//When animation finish - set the new file as the loaded file
		isTransitionFinished = true;
		loadNextSettingsFile(settingsToPath);
		return;
	}

	if (settingsFrom == NULL || !settingsFrom->bDocLoaded)
		return;

	updateGuiFromTag(timeSinceAnimationStart, "settings:optical_flow:strength", "/1/strength");
	updateGuiFromTag(timeSinceAnimationStart, "settings:velocity_mask:hue_offset");
	updateGuiFromTag(timeSinceAnimationStart, "settings:optical_flow:threshold");
	updateGuiFromTag(timeSinceAnimationStart, "settings:recolor:Cutoff", "/1/cutoff");
	updateGuiFromTag(timeSinceAnimationStart, "settings:fluid_solver:speed", "/1/speed");
	updateGuiFromTag(timeSinceAnimationStart, "settings:fluid_solver:cell_size");
	updateGuiFromTag(timeSinceAnimationStart, "settings:fluid_solver:viscosity");
	updateGuiFromTag(timeSinceAnimationStart, "settings:fluid_solver:vorticity");
	updateGuiFromTag(timeSinceAnimationStart, "settings:fluid_solver:dissipation");
	updateGuiFromTag(timeSinceAnimationStart, "settings:velocity_mask:strength");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:spawn_hue", "/1/spawn_hue");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:size", "/1/particle_size");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:mass");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:mass_spread");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:lifespan");
	updateGuiFromTag(timeSinceAnimationStart, "settings:particle_flow:cell_size");

	//The following is more generic but cost loading time and other loaded settings which kills the transition
	//This could work if we already set all the to parameters on the other settings - this prevents us
	// from live dj during a transition
	//gui.loadFrom(*settingsFrom);
}

/*
Receives a tag. for example "settings:recolor:Cutoff".
Reads from settingsFrom and settingsTo xml files
Updates the parameters based on a time based transition median
*/
void ofApp::updateGuiFromTag(float timeSinceAnimationStart, string tag, string oscMsgPath) {
	string subTag;

	string value = getValueAsString(tag);
	float amount = ofMap(timeSinceAnimationStart, 0, transitionTime, 0, 1);
	//Check if value is double, int or bool	

	double parameterValue; //TODO: bool
	if (value.compare("1") == 0 || value.compare("0") == '0') {
		parameterValue = getValueTransitionStep(tag, (int)amount);
	}
	if (isOnlyDouble(value.c_str())) {
		parameterValue = getValueTransitionStep(tag, amount);
	}
	else {
		parameterValue = getValueTransitionStep(tag, (int)amount);
	}
	//TODO: check for bool

	string firstTagName = string();
	subTag = getNextTagNode(tag, firstTagName); //Remove first node "settings:"
	std::replace(subTag.begin(), subTag.end(), '_',' '); //TODO: wont work for all settings since also '(' translates to ' '
	ofxGuiGroup group = gui;
	string lastTagName;
	//Start from gui and drill down till we get to the last node
	do {
		lastTagName = firstTagName;
		subTag = getNextTagNode(subTag, firstTagName);
		if (subTag == "")
			continue;

		group = group.getGroup(firstTagName);
	} while (subTag != "");

	//no more nodes - extract control
	ofxBaseGui* control = group.getControl(firstTagName);
	ofAbstractParameter* parameter = &control->getParameter();

	// Find what type of parameter it is
	if (ofParameter<float>* floatParam = dynamic_cast<ofParameter<float>*>(parameter)) {
		floatParam->set(parameterValue);
		sendOscMessage(oscMsgPath, (float)parameterValue);
		return;
	}
	// Find what type of parameter it is
	if (ofParameter<int>* intParam = dynamic_cast<ofParameter<int>*>(parameter)) {
		intParam->set(parameterValue);
		sendOscMessage(oscMsgPath, (int)parameterValue);
		return;
	}
}

void ofApp::sendOscMessage(string oscAddress, int value) {
	if (!oscAddress.empty() && !oscRemoteServerIpAddress.empty()) {
		ofxOscMessage m;
		m.setAddress(oscAddress);
		m.addIntArg(value);
		//oscSender.sendMessage(m);
	}
}

void ofApp::sendOscMessage(string oscAddress, float value) {
	if (!oscAddress.empty() && !oscRemoteServerIpAddress.empty()) {
		ofxOscMessage m;
		m.setAddress(oscAddress);
		m.addFloatArg(value);
		//oscSender.sendMessage(m);
	}
}

int ofApp::getValueTransitionStep(string tagName, int amount) {
	int val1 = settingsFrom->getValue(tagName, 0);
	int val2 = settingsTo->getValue(tagName, 0);
	return ofLerp(val1, val2, amount);
}

double ofApp::getValueTransitionStep(string tagName, double amount) {
	double val1 = settingsFrom->getValue(tagName, 0.0, 0);
	double val2 = settingsTo->getValue(tagName, 0.0, 0);
	return ofLerp(val1, val2, amount);
}

string ofApp::getValueAsString(string tagName) {
	return settingsTo->getValue(tagName, "");
}

bool ofApp::getValueTransitionStep(string tagName, bool amount) {
	bool val1 = settingsFrom->getValue(tagName, 0.0, 0);
	bool val2 = settingsTo->getValue(tagName, 0.0, 0);
	return (amount > 0.5) ? val2 : val1;
}


int ofApp::getNextSettingsCounter() {
	if (!isFileExist(relateiveDataPath + "settings" + std::to_string(loadSettingsFileIndex.get()+1) + ".xml")) {
		return 1;
	}
	else {
		return loadSettingsFileIndex.get()+1;
	}
}

void ofApp::loadNextSettingsFile(string settingsToPathArg) {
	ofLogWarning("Loaded a file");
	gui.loadFromFile(settingsToPathArg);
}

void ofApp::reset() {
	fluidSimulation.reset();
	//fluidSimulation.addObstacle(flowToolsLogoImage.getTexture());
	mouseForces.reset();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case OF_KEY_TAB:
	case 'G':
	case 'g': toggleGuiDraw = !toggleGuiDraw; break;
	case 'f':
	case 'F': doFullScreen.set(!doFullScreen.get()); break;
	case 'c':
	case 'C': doDrawCamBackground.set(!doDrawCamBackground.get()); break;

	case '0': drawMode.set(DRAW_COMPOSITE); break;
	case '1': drawMode.set(DRAW_COMPOSITE); break;
	case '2': drawMode.set(DRAW_FLUID_DENSITY); break;
	case '3': drawMode.set(DRAW_PARTICLES); break;
	case '4': drawMode.set(DRAW_VELDOTS); break;
	case '5': drawMode.set(DRAW_FLUID_VELOCITY); break;
	case '6': drawMode.set(DRAW_DISPLACEMENT); break;

	case 'r':
	case 'R':
		reset();
		break;

	case 'z':
	case 'Z':
		sourceMode.set((sourceMode.get() + 1) % SOURCE_COUNT);
		break;
	case 'x':
	case 'X':
		psEyeCameraIndex.set((psEyeCameraIndex.get() + 1) % (psEyeCameraIndex.getMax()+1));
		break;
	case 'l':
	case 'L':	
		transitionTime = 0;
		jumpToNextEffect();
		break;
	case 'o':
	case 'O':
		increaseParameter(velocityMask.hueOffset, 0.01);
		break;
	case 'i':
	case 'I':
		decreaseParameter(velocityMask.hueOffset, 0.01);
		break;
		//	case 'y':
		//        {
		//        ofTexture resultTex = opticalFlow.getOpticalFlow();
		//
		//		ofPixels pix;
		//		resultTex.readToPixels(pix);
		//
		//			
		//		ofFile output;
		//		output.open("opticalflow.bin", ofFile::WriteOnly, true);
		//		output.write((const char *)pix.getData(), pix.getTotalBytes());
		//		output.close();
		//        }
		break;
	case OF_KEY_UP:
	{
		float speed = fluidSimulation.getSpeed();
		if (speed < 99.8)
			fluidSimulation.setSpeed(fluidSimulation.getSpeed() + 0.2);
	}
	break;
	case OF_KEY_DOWN:
	{
		float speed = fluidSimulation.getSpeed();
		if (speed > 0.2)
			fluidSimulation.setSpeed(fluidSimulation.getSpeed() - 0.2);
		else
			fluidSimulation.setSpeed(0);
	}
	break;
	case OF_KEY_RIGHT:
	{
		float strength = opticalFlow.getStrength();
		if (strength < 99)
			opticalFlow.setStrength(opticalFlow.getStrength() + 1);
	}
	break;
	case OF_KEY_LEFT:
	{
		float strength = opticalFlow.getStrength();
		if (strength > 1)
			opticalFlow.setStrength(opticalFlow.getStrength() - 1);
		else
			opticalFlow.setStrength(0);
	}
	break;
	case 's':
		increaseParameter(recolor.cutoff, 0.0001);
		break;
	case 'S':
		increaseParameter(recolor.cutoff, 0.001);
		break;
	case 'a':
		decreaseParameter(recolor.cutoff, 0.0001);
		break;
	case 'A':
		decreaseParameter(recolor.cutoff, 0.001);
		break;
	case 'K':
	case 'k':
	{
		jumpToNextEffect();
		break;
	}
	case 'v':
		increaseParameter(particleFlow.spawnHue, 0.001);
		break;
	case 'V':
		increaseParameter(particleFlow.spawnHue, 0.01);
		break;
	case 'b':
		decreaseParameter(particleFlow.spawnHue, 0.001);
		break;
	case 'B':
		decreaseParameter(particleFlow.spawnHue, 0.01);
		break;
	case '>':
	case '.':
		increaseParameter(particleFlow.size, 0.1, 10);
		break;
	case '<':
	case ',':
		decreaseParameter(particleFlow.size, 0.1);
		break;
	default: break;
	}
}

void ofApp::increaseParameter(ofParameter<float> parameter, float val, float max) {
	if (parameter < (max - val))
		parameter += val;
	else
		parameter = max;
}

void ofApp::decreaseParameter(ofParameter<float> parameter, float val, float min) {
	if (parameter > val)
		parameter -= val;
	else
		parameter = min;
}

//Transition from current setting to the next one
void ofApp::jumpToNextEffect() {
	int oldSettingsFileIndex = loadSettingsFileIndex;
	loadSettingsFileIndex = ofApp::getNextSettingsCounter();
	startTransition(relateiveDataPath + "settings" + std::to_string(oldSettingsFileIndex) + ".xml",
		relateiveDataPath + "settings" + std::to_string(loadSettingsFileIndex) + ".xml");
}

//--------------------------------------------------------------
void ofApp::drawModeSetName(int &_value) {
	switch (_value) {
	case DRAW_COMPOSITE:		drawName.set("Composite      (1)"); break;
	case DRAW_PARTICLES:		drawName.set("Particles      "); break;
	case DRAW_FLUID_FIELDS:		drawName.set("Fluid Fields   (2)"); break;
	case DRAW_FLUID_DENSITY:	drawName.set("Fluid Density  "); break;
	case DRAW_FLUID_VELOCITY:	drawName.set("Fluid Velocity (3)"); break;
	case DRAW_FLUID_PRESSURE:	drawName.set("Fluid Pressure (4)"); break;
	case DRAW_FLUID_TEMPERATURE:drawName.set("Fld Temperature(5)"); break;
	case DRAW_FLUID_DIVERGENCE: drawName.set("Fld Divergence "); break;
	case DRAW_FLUID_VORTICITY:	drawName.set("Fluid Vorticity"); break;
	case DRAW_FLUID_BUOYANCY:	drawName.set("Fluid Buoyancy "); break;
	case DRAW_FLUID_OBSTACLE:	drawName.set("Fluid Obstacle "); break;
	case DRAW_OPTICAL_FLOW:		drawName.set("Optical Flow   (6)"); break;
	case DRAW_FLOW_MASK:		drawName.set("Flow Mask      (7)"); break;
	case DRAW_SOURCE:			drawName.set("Source         "); break;
	case DRAW_MOUSE:			drawName.set("Left Mouse     (8)"); break;
	case DRAW_VELDOTS:			drawName.set("VelDots        (0)"); break;
	case DRAW_DISPLACEMENT:		drawName.set("Displacement   "); break;
	}
}

void ofApp::setLoadSettingsName(int &fileIndex) {
	//ofLogWarning("Loading settings name");
	//gui.loadFromFile(relateiveDataPath + "settings" + std::to_string(fileIndex) + ".xml");
}

//--------------------------------------------------------------
void ofApp::draw() {
	if (sendToSpout) {
		globalFbo.begin();
	}
	ofClear(0, 0);
	if (doDrawCamBackground.get())
		drawSource();

	//if (eye) {
	//	videoTexture.draw(0, 0, eye->getWidth(), eye->getHeight());
	//}


	switch (drawMode.get()) {
	case DRAW_COMPOSITE: drawComposite(); break;
	case DRAW_PARTICLES: drawParticles(); break;
	case DRAW_FLUID_FIELDS: drawFluidFields(); break;
	case DRAW_FLUID_DENSITY: drawFluidDensity(); break;
	case DRAW_FLUID_VELOCITY: drawFluidVelocity(); break;
	case DRAW_FLUID_PRESSURE: drawFluidPressure(); break;
	case DRAW_FLUID_TEMPERATURE: drawFluidTemperature(); break;
	case DRAW_FLUID_DIVERGENCE: drawFluidDivergence(); break;
	case DRAW_FLUID_VORTICITY: drawFluidVorticity(); break;
	case DRAW_FLUID_BUOYANCY: drawFluidBuoyance(); break;
	case DRAW_FLUID_OBSTACLE: drawFluidObstacle(); break;
	case DRAW_FLOW_MASK: drawMask(); break;
	case DRAW_OPTICAL_FLOW: drawOpticalFlow(); break;
	case DRAW_SOURCE: drawSource(); break;
	case DRAW_MOUSE: drawMouseForces(); break;
	case DRAW_VELDOTS: drawVelocityDots(); break;
	case DRAW_DISPLACEMENT: drawVelocityDisplacement(); break;
	}
	if (sendToSpout) {
		globalFbo.end();
	}
#ifdef _WIN32
	if (!spoutInitialized) {
		int spoutRandom = rand() % 1000 + 1;
		string spoutName = "OF Spout Sender" + to_string(spoutRandom);
		char *spoutNameCstr = new char[spoutName.length() + 1];
		strcpy(spoutNameCstr, spoutName.c_str());
		
		spoutInitialized = senderSpout.CreateSender(spoutNameCstr, internalWidth, internalHeight);
		
		if (!spoutInitialized) {
			ofLogError() << "Failed to initialize sender spout!";
		}
	}
	// TODO: check if enabled too
	if (spoutInitialized && sendToSpout) {
		const auto& texData = globalFbo.getTexture().getTextureData();
		senderSpout.SendTexture(texData.textureID, texData.textureTarget, internalWidth, internalHeight, false);
	}
#endif
	if (sendToSpout) {
		ofPushStyle();
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		globalFbo.draw(0, 0, ofGetWidth(), ofGetHeight());
		ofPopStyle();
	}
	if (toggleGuiDraw)
	{
		ofShowCursor();
		drawGui();
	}
	else {
		ofHideCursor();
	}

}

//--------------------------------------------------------------
void ofApp::drawComposite(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	ofEnableBlendMode(OF_BLENDMODE_ADD);
	fluidSimulation.draw(_x, _y, _width, _height);

	ofEnableBlendMode(OF_BLENDMODE_ADD);
	if (particleFlow.isActive())
		particleFlow.draw(_x, _y, _width, _height, fluidSimulation.getVelocity());

	if (showLogo) {
		flowToolsLogoImage.draw(_x, _y, _width, _height);
	}

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawParticles(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	if (particleFlow.isActive())
		particleFlow.draw(_x, _y, _width, _height, fluidSimulation.getVelocity());
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidFields(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	pressureField.setPressure(fluidSimulation.getPressure());
	pressureField.draw(_x, _y, _width, _height);
	velocityTemperatureField.setVelocity(fluidSimulation.getVelocity());
	velocityTemperatureField.setTemperature(fluidSimulation.getTemperature());
	velocityTemperatureField.draw(_x, _y, _width, _height);
	temperatureField.setTemperature(fluidSimulation.getTemperature());

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidDensity(int _x, int _y, int _width, int _height) {
	ofPushStyle();

	fluidSimulation.draw(_x, _y, _width, _height);

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidVelocity(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofClear(0, 0);
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		//	ofEnableBlendMode(OF_BLENDMODE_DISABLED); // altenate mode
		displayScalar.setSource(fluidSimulation.getVelocity());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(fluidSimulation.getVelocity());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidPressure(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofClear(128);
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getPressure());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		pressureField.setPressure(fluidSimulation.getPressure());
		pressureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidTemperature(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getTemperature());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		temperatureField.setTemperature(fluidSimulation.getTemperature());
		temperatureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidDivergence(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getDivergence());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		temperatureField.setTemperature(fluidSimulation.getDivergence());
		temperatureField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidVorticity(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getConfinement());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofSetColor(255, 255, 255, 255);
		velocityField.setVelocity(fluidSimulation.getConfinement());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidBuoyance(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
		displayScalar.setSource(fluidSimulation.getSmokeBuoyancy());
		displayScalar.draw(_x, _y, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(fluidSimulation.getSmokeBuoyancy());
		velocityField.draw(_x, _y, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawFluidObstacle(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	fluidSimulation.getObstacle().draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawMask(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	velocityMask.draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawOpticalFlow(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	if (showScalar.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		displayScalar.setSource(opticalFlow.getOpticalFlowDecay());
		displayScalar.draw(0, 0, _width, _height);
	}
	if (showField.get()) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		velocityField.setVelocity(opticalFlow.getOpticalFlowDecay());
		velocityField.draw(0, 0, _width, _height);
	}
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawSource(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_DISABLED);
	cameraFbo.draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawMouseForces(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofClear(0, 0);

	for (int i = 0; i<mouseForces.getNumForces(); i++) {
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		if (mouseForces.didChange(i)) {
			ftDrawForceType dfType = mouseForces.getType(i);
			if (dfType == FT_DENSITY)
				mouseForces.getTextureReference(i).draw(_x, _y, _width, _height);
		}
	}

	for (int i = 0; i<mouseForces.getNumForces(); i++) {
		ofEnableBlendMode(OF_BLENDMODE_ALPHA);
		if (mouseForces.didChange(i)) {
			switch (mouseForces.getType(i)) {
			case FT_VELOCITY:
				velocityField.setVelocity(mouseForces.getTextureReference(i));
				velocityField.draw(_x, _y, _width, _height);
				break;
			case FT_TEMPERATURE:
				temperatureField.setTemperature(mouseForces.getTextureReference(i));
				temperatureField.draw(_x, _y, _width, _height);
				break;
			case FT_PRESSURE:
				pressureField.setPressure(mouseForces.getTextureReference(i));
				pressureField.draw(_x, _y, _width, _height);
				break;
			default:
				break;
			}
		}
	}

	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawVelocityDots(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	velocityDots.setVelocity(fluidSimulation.getVelocity());
	velocityDots.draw(_x, _y, _width, _height);
	ofPopStyle();
}

void ofApp::drawVelocityDisplacement(int _x, int _y, int _width, int _height) {
	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	velocityOffset.setColorMap(cameraFbo.getTextureReference());
	velocityOffset.setSource(fluidSimulation.getVelocity());
	velocityOffset.draw(_x, _y, _width, _height);
	ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawGui() {
	guiFPS = (int)(ofGetFrameRate() + 0.5);

	// calculate minimum fps
	deltaTimeDeque.push_back(deltaTime);

	while (deltaTimeDeque.size() > guiFPS.get())
		deltaTimeDeque.pop_front();

	float longestTime = 0;
	for (int i = 0; i<deltaTimeDeque.size(); i++) {
		if (deltaTimeDeque[i] > longestTime)
			longestTime = deltaTimeDeque[i];
	}

	guiMinFPS.set(1.0 / longestTime);


	ofPushStyle();
	ofEnableBlendMode(OF_BLENDMODE_ALPHA);
	gui.draw();

	// HACK TO COMPENSATE FOR DISSAPEARING MOUSE
	ofEnableBlendMode(OF_BLENDMODE_SUBTRACT);
	ofDrawCircle(ofGetMouseX(), ofGetMouseY(), ofGetWindowWidth() / 300.0);
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	ofDrawCircle(ofGetMouseX(), ofGetMouseY(), ofGetWindowWidth() / 600.0);
	ofPopStyle();
}

void ofApp::startJumpBetweenStates(bool&) {
	jumpBetweenStatesStartTime = ofGetElapsedTimef();
    lastOscMessageTime = ofGetElapsedTimef(); //This to emulate the fact that user manually set to on
}

void ofApp::updateNumberOfSettingFiles() {
	lastSaveFileCounter = 1;
	while (isFileExist(relateiveDataPath + "settings" + std::to_string(lastSaveFileCounter) + ".xml")) {
		lastSaveFileCounter++;
	}

	loadSettingsFileIndex.setMax(lastSaveFileCounter - 1);
	loadSettingsFileIndex = 1;
}

void ofApp::MultiSavePressed(const void * sender) {
	ofLogWarning("Saving to a new settings.xml file");
	ofxButton * button = (ofxButton*)sender;

	while (isFileExist(relateiveDataPath + "settings" + std::to_string(lastSaveFileCounter) + ".xml")) {
		lastSaveFileCounter++;
	}

	loadSettingsFileIndex = lastSaveFileCounter;

	gui.saveToFile(relateiveDataPath + "settings" + std::to_string(lastSaveFileCounter) + ".xml");
	loadSettingsFileNumber = lastSaveFileCounter;
}

void ofApp::setRelativePath(const char *filename) {
	relateiveDataPath = dirnameOf(filename) + "\\data\\";
	relateiveKinectDataPath = relateiveDataPath;
	relateivePsEyeDataPath = relateiveDataPath + "\\pseyesettings\\";
}

bool ofApp::isFileExist(string fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

string ofApp::dirnameOf(const string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (string::npos == pos)
		? ""
		: fname.substr(0, pos);
}

void ofApp::exit() {
#ifdef _WIN32
	senderSpout.ReleaseSender(); // Release the sender
#endif
}