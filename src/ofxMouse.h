#pragma once
#ifdef _WIN32
	#ifndef _WIN32_WINNT
	#   define _WIN32_WINNT 0x500
	#endif
	#include "Winuser.h"
	#include "windef.h"
#elif defined(__APPLE__)
	#include <ApplicationServices/ApplicationServices.h>
#endif

class ofxMouse
{
public:
    enum MouseEventFlags
	{
		LeftDown = 0x00000002,
		LeftUp = 0x00000004,
		MiddleDown = 0x00000020,
		MiddleUp = 0x00000040,
		Move = 0x00000001,
		Absolute = 0x00008000,
		RightDown = 0x00000008,
		RightUp = 0x00000010
	};

	static void SetCursorPosition(int x, int y) {
#ifdef _WIN32
		SetCursorPos(x, y);
#elif defined(__APPLE__)
        CGPoint pt = CGPointMake(x, y);

        CGWarpMouseCursorPosition(pt);
        CGAssociateMouseAndMouseCursorPosition(true);

        //In mac we click on set cursor position - lazy alert
        postMouseEvent(kCGEventLeftMouseDown, pt);
        postMouseEvent(kCGEventLeftMouseUp, pt);
#endif
	}
    
    
	static void MouseEvent(MouseEventFlags value) {
#ifdef _WIN32
		POINT curPos[2];
		GetCursorPos(&curPos[0]);
		mouse_event(
			(int)value,
			curPos->x,
			curPos->y,
			0,
			0);
#else
        //In mac we use the SetCursorPosition to also click
#endif
	}

#if defined(__APPLE__)
private:
	// CGPostMouseEvent has been deprecated since 10.6; the CGEvent API is the
	// supported way to synthesize mouse input. macOS only lets this through
	// once the app has Accessibility permission
	// (System Settings > Privacy & Security > Accessibility).
	static void postMouseEvent(CGEventType type, CGPoint pt) {
		CGEventRef event = CGEventCreateMouseEvent(NULL, type, pt, kCGMouseButtonLeft);
		if (event) {
			CGEventPost(kCGHIDEventTap, event);
			CFRelease(event);
		}
	}
#endif
};
