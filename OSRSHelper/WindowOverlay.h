#pragma once
#include <Windows.h>
#include <nana/gui.hpp>
#include <nana/gui/timer.hpp>
#include "Rect_Utils.h"

class WindowOverlay : public nana::form
{
public:
	WindowOverlay(HWND target_window, UINT fps = 30);
	~WindowOverlay();

	void show();
	void hide();
	void update(const nana::arg_elapse& elapsed);
protected:
	RECT lastRect;
	HWND target_window;
	HWND self_handle;
	nana::timer internalTimer;
};

