#include "ofMain.h"
#include "ofApp.h"
#include "testApp.h"
//#define USE_PROGRAMMABLE_GL
//========================================================================
int main(int argc, char** argv) {

	ofGLFWWindowSettings windowSettings;
#ifdef USE_PROGRAMMABLE_GL
	windowSettings.setGLVersion(4, 1);
#endif
	windowSettings.width = 1280;
	windowSettings.height = 720;
	windowSettings.windowMode = OF_WINDOW;

	ofCreateWindow(windowSettings);

	ofApp* app = new ofApp();
	if (argv != NULL && argv[0] != NULL) {
		app->setRelativePath(argv[0]);
	}
	
	ofRunApp(app);
	
}
