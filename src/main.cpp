#include "ofMain.h"
#include "ofApp.h"
//#define USE_PROGRAMMABLE_GL
//========================================================================

//Disable the following (and choose subsystem Console) to show logging
#ifdef _WIN32
//#define WIN_APP
#endif


#define PS_PORT 10001
#define KINECT_PORT 10002
#define DUMMY_PS_PORT 10101
#define DUMMY_KINECT_PORT 10102
#define PORT_SERVER 20001

#ifdef _WIN32
void setAppPath(ofApp* app) {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    app->setRelativePath(buffer);
}
#else
void setAppPath(ofApp* app, char** argv) {
    // Mac - Navigate to Resources path
    if (argv != NULL && argv[0] != NULL) {
        string path = app->ofApp::dirnameOf(argv[0]);
        app->setMacRelativePath(path);
    }
}
#endif



#ifdef _WIN32
BOOL CheckPortUDP(short int dwPort, char *ipAddressStr)
{
	struct sockaddr_in client;
	int sock;

	client.sin_family = AF_INET;
	client.sin_port = htons(dwPort);
	client.sin_addr.s_addr = inet_addr(ipAddressStr);

	sock = (int)socket(AF_INET, SOCK_DGRAM, 0);

	int result = ::bind(sock, (SOCKADDR FAR *) &client, sizeof(SOCKADDR_IN));
	return result == SOCKET_ERROR || result == INVALID_SOCKET;
}
#endif
#ifdef WIN_APP
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdline, int nCmdShow) {
#else
int main(int argc, char** argv) {
#endif
	bool isShouldStartPsCam = false;
	ofGLFWWindowSettings windowSettings;
#ifdef USE_PROGRAMMABLE_GL
	windowSettings.setGLVersion(4, 1);
#endif
	windowSettings.width = 1280;
	windowSettings.height = 720;
	windowSettings.windowMode = OF_FULLSCREEN;
    windowSettings.monitor = 0;
    
    int oscPort;
#ifdef _WIN32
    WSADATA wsaData;
	isShouldStartPsCam = true;
    if (WSAStartup(0x0101, &wsaData) == 0)
    {
		if (!CheckPortUDP(DUMMY_PS_PORT, "127.0.0.1")) {
			//PS is not running. starting ps - nothing is necceserry
			oscPort = PS_PORT;
			printf("Opened OSC on port %d\r\n", oscPort);
		}
		else if (!CheckPortUDP(DUMMY_KINECT_PORT, "127.0.0.1")) {
			//Kinect is not running. starting kinect
			isShouldStartPsCam = false;
			windowSettings.monitor = 1; // Second instance - Open on second monitor
			oscPort = KINECT_PORT;
			printf("Opened OSC on port %d\r\n", oscPort);
		}
		else {
			// 2 instances are running starting 3+
			int availablePort = KINECT_PORT + 1;
			int dummyPort = DUMMY_KINECT_PORT + 1;

			while (CheckPortUDP(dummyPort, "127.0.0.1")) {
				availablePort++;
				dummyPort++;
				printf("Checking next available port %d...\r\n", availablePort);
				isShouldStartPsCam = false;
				windowSettings.monitor = 1; // Second instance - Open on second monitor
			}
			printf("Opened OSC on port %d\r\n", availablePort);
			oscPort = availablePort;
		}
    }
#else
    oscPort = PS_PORT;
#endif
    
    ofCreateWindow(windowSettings);
    
	ofApp* app = new ofApp();
    app->oscPort = oscPort;
	app->shouldStartPsEyeCam = isShouldStartPsCam;
#ifdef _WIN32
    setAppPath(app);
#else
    setAppPath(app, argv);
#endif
    ofRunApp(app);
	
#ifdef _WIN32
	WSACleanup();
#endif
}
