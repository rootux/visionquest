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
	static enum MouseEventFlags
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
		SetCursorPos(x, y);
	}

	static void MouseEvent(MouseEventFlags value) {
		POINT curPos[2];
		GetCursorPos(&curPos[0]);
		mouse_event(
			(int)value,
			curPos->x,
			curPos->y,
			0,
			0);
	}
};
