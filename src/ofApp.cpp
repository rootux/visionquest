#include "ofApp.h"

static const int ITUR_BT_601_CY = 1220542;
static const int ITUR_BT_601_CUB = 2116026;
static const int ITUR_BT_601_CUG = -409993;
static const int ITUR_BT_601_CVG = -852492;
static const int ITUR_BT_601_CVR = 1673527;
static const int ITUR_BT_601_SHIFT = 20;

//--------------------------------------------------------------
void ofApp::setup() {

	ofSetVerticalSync(false);
	ofSetLogLevel(OF_LOG_NOTICE);

	drawWidth = 1280;
	drawHeight = 720;
	// process all but the density on 16th resolution
	flowWidth = drawWidth / 4;
	flowHeight = drawHeight / 4;

	// FLOW & MASK
	opticalFlow.setup(flowWidth, flowHeight);
	velocityMask.setup(drawWidth, drawHeight);

	// FLUID & PARTICLES
	fluidSimulation.setup(flowWidth, flowHeight, drawWidth, drawHeight);
	particleFlow.setup(flowWidth, flowHeight, drawWidth, drawHeight);

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
	mouseForces.setup(flowWidth, flowHeight, drawWidth, drawHeight);

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
	kinectFbo.allocate(1280, 720, GL_R16);
	kinectFbo.getTexture().setRGToRGBASwizzles(true);
	ofLogError("kinect inited");
#endif
    
	didCamUpdate = false;
	cameraFbo.allocate(1280, 720);
	cameraFbo.black();
    
    recolor.setup();

	// GUI
	setupGui();

	lastTime = ofGetElapsedTimef();


	//Setup ps3 eye
	using namespace ps3eye;
	std::vector<PS3EYECam::PS3EYERef> devices(PS3EYECam::getDevices());
	if (devices.size())
	{
		eye = devices.at(0);
		bool res = eye->init(640, 480, 60);
		if (res) {
			eye->start();
			eye->setExposure(255);
			videoFrame = new unsigned char[eye->getWidth()*eye->getHeight() * 4];
			videoTexture.allocate(eye->getWidth(), eye->getHeight(), GL_RGBA);
		}
		else { 
			eye = NULL;
		}
	}
	else {
		ofLogError() << "Failed to open PS eye!";
	}

}

//--------------------------------------------------------------
void ofApp::setupGui() {
	gui.setup("settings");
	gui.add(multiSaveButton.setup("Multiple Save Settings"));
	gui.setDefaultBackgroundColor(ofColor(0, 0, 0, 127));
	gui.setDefaultFillColor(ofColor(160, 160, 160, 160));
	gui.add(guiFPS.set("average FPS", 0, 0, 60));
	gui.add(guiMinFPS.set("minimum FPS", 0, 0, 60));
	gui.add(doFullScreen.set("fullscreen (F)", false));
	doFullScreen.addListener(this, &ofApp::setFullScreen);
	gui.add(toggleGuiDraw.set("show gui (G)", false));
	gui.add(doFlipCamera.set("flip camera", true));
	gui.add(doDrawCamBackground.set("draw camera (C)", true));
	gui.add(drawMode.set("draw mode", DRAW_COMPOSITE, DRAW_COMPOSITE, DRAW_COUNT - 1));
	drawMode.addListener(this, &ofApp::drawModeSetName);
	gui.add(drawName.set("MODE", "draw name"));
	gui.add(sourceMode.set("Source mode (z)", SOURCE_KINECT_PS3EYE, SOURCE_KINECT_PS3EYE, SOURCE_COUNT - 1));


	multiSaveButton.addListener(this, &ofApp::MultiSavePressed);

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

	// if the settings file is not present the parameters will not be set during this setup
	if (!ofFile("settings.xml"))
		gui.saveToFile("settings.xml");

	gui.loadFromFile("settings.xml");

	gui.minimizeAll();
	toggleGuiDraw = true;

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

bool ofApp::isKinectSource() {
	return (sourceMode.get() == SOURCE_KINECT_PS3EYE || 
		sourceMode.get() == SOURCE_KINECT_DEPTH_PS3EYE || 
		sourceMode.get() == SOURCE_KINECT);
}

bool ofApp::isPsEyeSource() {
	return (sourceMode.get() == SOURCE_KINECT_PS3EYE ||
		sourceMode.get() == SOURCE_KINECT_DEPTH_PS3EYE ||
		sourceMode.get() == SOURCE_PS3EYE);
}

bool ofApp::isKinectAndPsEyeSource() {
	return sourceMode.get() == SOURCE_KINECT_PS3EYE;
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

	if (isPsEyeSource() && eye)
	{
		uint8_t* new_pixels = eye->getFrame();
		yuv422_to_rgba(new_pixels, eye->getRowBytes(), videoFrame, eye->getWidth(), eye->getHeight());
		videoTexture.loadData(videoFrame, eye->getWidth(), eye->getHeight(), GL_RGBA);
		free(new_pixels);
	}
#ifdef _KINECT
	if ((isKinectSource() && (kinect.getDepthSource()->isFrameNew())) ||
		(isPsEyeSource() && eye)) {
#else
    if ((isPsEyeSource() && eye) || simpleCam.isFrameNew()) {
#endif
		//simpleCam.isFrameNew();
		ofPushStyle();
		ofEnableBlendMode(OF_BLENDMODE_DISABLED);
#ifdef _KINECT
		if (sourceMode == SOURCE_KINECT) {
			kinectFbo.begin();
		}
		else
#endif
		{
			cameraFbo.begin();
		}
		if (doFlipCamera) {
			float psEyeXPosition;
			switch (sourceMode) {
#ifdef _KINECT
			case SOURCE_KINECT:
				//kinect.getColorSource()->draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
				kinect.getDepthSource()->draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
                    break;
			//case SOURCE_KINECT_PS3EYE:
			//	kinect.getColorSource()->draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
			//	psEyeXPosition = (cameraFbo.getWidth() / 3 ) + 20;
			//	videoTexture.draw(psEyeXPosition, 20, -cameraFbo.getWidth() / 3, cameraFbo.getHeight() / 3);
			//	break;
			//case SOURCE_KINECT_DEPTH_PS3EYE:
			//	kinect.getDepthSource()->draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
			//	psEyeXPosition = (cameraFbo.getWidth() / 3) + 20;
			//	videoTexture.draw(psEyeXPosition, 20, -cameraFbo.getWidth() / 3, cameraFbo.getHeight() / 3);
			//	break;
#endif
            case SOURCE_PS3EYE:
                videoTexture.draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
                break;
            default:
                simpleCam.draw(cameraFbo.getWidth(), 0, -cameraFbo.getWidth(), cameraFbo.getHeight());
                break;
			};
		}
		else {
			switch (sourceMode) {
				float psEyeXPosition;
#ifdef _KINECT
			case SOURCE_KINECT:
				kinect.getDepthSource()->draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
				break;
   //         case SOURCE_KINECT_PS3EYE:
   //             kinect.getColorSource()->draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
   //             psEyeXPosition = (cameraFbo.getWidth() / 3 * 2) - 20;
   //             videoTexture.draw(psEyeXPosition, 20, cameraFbo.getWidth() / 3, cameraFbo.getHeight() / 3);
   //             break;
			//case SOURCE_KINECT_DEPTH_PS3EYE:
			//	kinect.getDepthSource()->draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
			//	psEyeXPosition = (cameraFbo.getWidth() / 3 * 2) - 20;
			//	videoTexture.draw(psEyeXPosition, 20, cameraFbo.getWidth() / 3, cameraFbo.getHeight() / 3);
			//	break;
#endif
			case SOURCE_PS3EYE:
				videoTexture.draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
				break;
            default:
                simpleCam.draw(0, 0, cameraFbo.getWidth(), cameraFbo.getHeight());
                break;
			};
		}
#ifdef _KINECT
		if (sourceMode == SOURCE_KINECT) {
			kinectFbo.end();

			ofPopStyle();

			ofPushStyle();
			ofEnableBlendMode(OF_BLENDMODE_DISABLED);

			recolor.update(cameraFbo, kinectFbo.getTexture(), doFlipCamera);

			ofPopStyle();

			//TODO: debug kinect raw depth source
			//opticalFlow.setSource(kinectFbo.getTexture());
			opticalFlow.setSource(cameraFbo.getTexture());
		}
		else
#endif
		{
			cameraFbo.end();
			opticalFlow.setSource(cameraFbo.getTexture());
		}



		


		//opticalFlow.update(deltaTime);
		// use internal deltatime instead
		opticalFlow.update();
        

		velocityMask.setDensity(cameraFbo.getTexture());
		velocityMask.setVelocity(opticalFlow.getOpticalFlow());
		velocityMask.update();



		//THIS ALLOWS MOUSE WITH IMAGE


		//opticalFlow.setSource(cameraFbo.getTexture());

		// opticalFlow.update(deltaTime);
		// use internal deltatime instead
		//opticalFlow.update();

		//velocityMask.setDensity(cameraFbo.getTexture());
		//velocityMask.setVelocity(opticalFlow.getOpticalFlow());
		//velocityMask.update();
	}


	fluidSimulation.addVelocity(opticalFlow.getOpticalFlowDecay());  //!
	//fluidSimulation.addVelocity(velocityMask.getColorMask());
	//fluidSimulation.addVelocity(cameraFbo.getTexture()); //!
	fluidSimulation.addDensity(velocityMask.getColorMask());
	//fluidSimulation.addDensity(cameraFbo.getTexture());
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

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	switch (key) {
	case 'G':
	case 'g': toggleGuiDraw = !toggleGuiDraw; break;
	case 'f':
	case 'F': doFullScreen.set(!doFullScreen.get()); break;
	case 'c':
	case 'C': doDrawCamBackground.set(!doDrawCamBackground.get()); break;

	case '1': drawMode.set(DRAW_COMPOSITE); break;
	case '2': drawMode.set(DRAW_FLUID_FIELDS); break;
	case '3': drawMode.set(DRAW_FLUID_VELOCITY); break;
	case '4': drawMode.set(DRAW_FLUID_PRESSURE); break;
	case '5': drawMode.set(DRAW_FLUID_TEMPERATURE); break;
	case '6': drawMode.set(DRAW_OPTICAL_FLOW); break;
	case '7': drawMode.set(DRAW_FLOW_MASK); break;
	case '8': drawMode.set(DRAW_MOUSE); break;

	case 'r':
	case 'R':
		fluidSimulation.reset();
		fluidSimulation.addObstacle(flowToolsLogoImage.getTexture());
		mouseForces.reset();
		break;
	
	case 'z':
	case 'Z':
		sourceMode.set((sourceMode.get() + 1) % SOURCE_COUNT);
		break;

	case 's':
		{
			ofTexture resultTex = opticalFlow.getOpticalFlow();

			ofPixels pix;
			resultTex.readToPixels(pix);

			
			ofFile output;
			output.open("opticalflow.bin", ofFile::WriteOnly, true);
			output.write((const char *)pix.getData(), pix.getTotalBytes());
			output.close(); 
		}
		break;

	default: break;
	}
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

//--------------------------------------------------------------
void ofApp::draw() {
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
		particleFlow.draw(_x, _y, _width, _height);

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
		particleFlow.draw(_x, _y, _width, _height);
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

void	ofApp::MultiSavePressed(const void * sender) {
	ofxButton * button = (ofxButton*)sender;
	ofLogWarning("Saving to a new settings.xml file");
	while (isFileExist(relateiveDataPath + "settings" + std::to_string(lastSaveFileCounter) + ".xml")) {
		lastSaveFileCounter++;
	}
	gui.saveToFile(relateiveDataPath + "settings" + std::to_string(lastSaveFileCounter) + ".xml");
}

void ofApp::setRelativePath(const char *filename) {
	relateiveDataPath = dirnameOf(filename) + "\\data\\";
}

bool ofApp::isFileExist(std::string fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

std::string ofApp::dirnameOf(const std::string& fname)
{
	size_t pos = fname.find_last_of("\\/");
	return (std::string::npos == pos)
		? ""
		: fname.substr(0, pos);
}
