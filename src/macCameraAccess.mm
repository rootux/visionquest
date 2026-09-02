// macOS camera authorization.
//
// openFrameworks' AVFoundation grabber starts a capture session without ever
// calling -requestAccessForMediaType:, so on a modern macOS the session simply
// produces no frames and the app draws a black camera source with no error.
// Ask for access (and wait for the answer) before the grabber is set up.

#ifdef __APPLE__

#import <AVFoundation/AVFoundation.h>

extern "C" const char * vqCameraAccessStatus() {
	switch ([AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo]) {
		case AVAuthorizationStatusNotDetermined: return "not determined";
		case AVAuthorizationStatusRestricted:    return "restricted";
		case AVAuthorizationStatusDenied:        return "denied";
		case AVAuthorizationStatusAuthorized:    return "authorized";
	}
	return "unknown";
}

extern "C" bool vqEnsureCameraAccess() {
	AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
	if (status == AVAuthorizationStatusAuthorized) {
		return true;
	}
	if (status != AVAuthorizationStatusNotDetermined) {
		// Denied or restricted - only the user can change this, in
		// System Settings > Privacy & Security > Camera.
		return false;
	}

	// The completion handler runs on an arbitrary queue, so blocking the main
	// thread here is safe: the system presents the prompt out of process.
	__block BOOL granted = NO;
	dispatch_semaphore_t answered = dispatch_semaphore_create(0);
	[AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
							 completionHandler:^(BOOL isGranted) {
								 granted = isGranted;
								 dispatch_semaphore_signal(answered);
							 }];
	dispatch_semaphore_wait(answered, dispatch_time(DISPATCH_TIME_NOW, (int64_t)(120 * NSEC_PER_SEC)));
	return granted;
}

#endif
