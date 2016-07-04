#pragma once
#ifdef _WIN32
	#ifndef _WIN32_WINNT
	#   define _WIN32_WINNT 0x500
	#endif
	#include "Winuser.h"
	#include "windef.h"
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
#else
        CGPoint pt;
        pt.x = x;
        pt.y = y;
        
        CGSetLocalEventsSuppressionInterval(0);
        CGWarpMouseCursorPosition(pt);
        
        //In mac we click on set cursor position - lazy alert
        CGPostMouseEvent( pt, 1, 1, 1 );
        CGPostMouseEvent( pt, 1, 1, 0 );
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
};
