#include "ofMain.h"
#include "ofApp.h"
//#define USE_PROGRAMMABLE_GL
//========================================================================

#define PORT 10001
#define DUMMY_PORT 10101
#define PORT_SERVER 20001

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
	return result == SOCKET_ERROR;
}
#endif
#ifdef _WIN32
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, LPSTR lpCmdline, int nCmdShow) {
#else
int main(int argc, char** argv) {
#endif

	ofGLFWWindowSettings windowSettings;
#ifdef USE_PROGRAMMABLE_GL
	windowSettings.setGLVersion(4, 1);
#endif
	windowSettings.width = 1280;
	windowSettings.height = 720;

	windowSettings.windowMode = OF_WINDOW;
	
	ofCreateWindow(windowSettings);

	ofApp* app = new ofApp();
#ifdef _WIN32
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	app->setRelativePath(buffer);
#else
    // Mac - Navigate to Resources path
	if (argv != NULL && argv[0] != NULL) {
        string path = app->ofApp::dirnameOf(argv[0]);
		app->setMacRelativePath(path);
	}
#endif
#ifdef _WIN32
	WSADATA wsaData;
	app->shouldStartPsEyeCam = true;
	if (WSAStartup(0x0101, &wsaData) == 0)
	{
		int availablePort = PORT;
		int dummyPort = DUMMY_PORT;
		while (CheckPortUDP(dummyPort, "127.0.0.1")) {
			availablePort++;
			dummyPort++;
			printf("Checking next available port %d...\r\n", availablePort);
			app->shouldStartPsEyeCam = false;
		}

		printf("Opened OSC on port %d\r\n", availablePort);
		app->oscPort = availablePort;
	}
#else
	app->oscPort = PORT;
#endif
	ofRunApp(app);
	
#ifdef _WIN32
	WSACleanup();
#endif
}
