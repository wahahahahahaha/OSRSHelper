#include "WindowOverlay.h"



WindowOverlay::WindowOverlay(HWND target_window, UINT fps) : target_window(target_window)
{
	self_handle = reinterpret_cast<HWND>(native_handle());

	//setup update timer
	UINT interval = 1 / fps * 1000;
	internalTimer.interval(interval);
	internalTimer.elapse(std::bind(&WindowOverlay::update, this, std::placeholders::_1));

	//make form undecorated
	LONG lStyle = GetWindowLongPtr(self_handle, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLongPtr(self_handle, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLongPtr(self_handle, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLongPtr(self_handle, GWL_EXSTYLE, lExStyle);
	SetWindowLong(self_handle, GWL_EXSTYLE, GetWindowLong(self_handle, GWL_EXSTYLE) | WS_EX_LAYERED|WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(self_handle, RGB(255, 255,255), 0, LWA_COLORKEY);
	
}

WindowOverlay::~WindowOverlay()
{
}

void WindowOverlay::update(const nana::arg_elapse& elapsed) {
	//put on top
	if (GetForegroundWindow() == target_window) {
		//TODO: Extra check if the overlay is in front of the window, idk
		SetWindowPos(self_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		SetWindowPos(self_handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
	
	//update size and position
	RECT rect;
	GetWindowRect(target_window, &rect);
	if (!rect_equals(rect, lastRect)){
		RECT clientRect;
		GetClientRect(target_window, &clientRect);

		int borderWidth = ((rect.right - rect.left) - clientRect.right) / 2;
		int titlebarHeight = ((rect.bottom - rect.top) - clientRect.bottom - 2 * borderWidth) + borderWidth;
		SetWindowPos(self_handle, (HWND)0, rect.left + borderWidth, rect.top + titlebarHeight, clientRect.right, clientRect.bottom, SWP_FRAMECHANGED);

		lastRect = rect;
	}
}

void WindowOverlay::show()
{
	nana::form::show();
	internalTimer.start();
}

void WindowOverlay::hide()
{
	internalTimer.stop();
	nana::form::hide();
}
