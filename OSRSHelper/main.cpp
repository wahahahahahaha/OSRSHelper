#include <Windows.h>
#include <gdiplus.h>
#include <tlhelp32.h>
#include <cstdio>
#include <string>
#include <vector>
#include <nana/gui.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/button.hpp>
#include "WindowOverlay.h"

using std::string;
using namespace Gdiplus;
using namespace nana;

#pragma comment (lib,"Gdiplus.lib")

Bitmap* CaptureScreen();
Bitmap* CaptureWindow(HWND handle);

/// <summary>
/// Creates an Image object containing a screen shot of the entire desktop
/// </summary>
/// <returns></returns>
Bitmap* CaptureScreen()
{
	return CaptureWindow(GetDesktopWindow());
}

/// <summary>
/// Creates an Image object containing a screen shot of a specific window
/// </summary>
/// <param name="handle">The handle to the window. (In windows forms, this is obtained by the Handle property)</param>
/// <returns></returns>
Bitmap* CaptureWindow(HWND handle)
{
	// get te hDC of the target window
	HDC hdcSrc = GetWindowDC(handle);
	// get the size
	RECT windowRect = RECT();
	GetWindowRect(handle, &windowRect);
	int width = windowRect.right - windowRect.left;
	int height = windowRect.bottom - windowRect.top;
	// create a device context we can copy to
	HDC hdcDest = CreateCompatibleDC(hdcSrc);
	// create a bitmap we can copy it to,
	// using GetDeviceCaps to get the width/height
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcSrc, width, height);
	// select the bitmap object
	HGDIOBJ hOld = SelectObject(hdcDest, hBitmap);
	// bitblt over
	BitBlt(hdcDest, 0, 0, width, height, hdcSrc, 0, 0, SRCCOPY);
	// restore selection
	SelectObject(hdcDest, hOld);
	// clean up 
	DeleteDC(hdcDest);
	ReleaseDC(handle, hdcSrc);

	Bitmap* image = new Bitmap(hBitmap, NULL);
	// free up the Bitmap object
	DeleteObject(hBitmap);
	return image;
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = NULL;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

std::vector<HANDLE> GetProcessesByName(string target) {
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	std::vector<HANDLE> processes;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry) == TRUE)
	{
		while (Process32Next(snapshot, &entry) == TRUE)
		{
			if (_stricmp(entry.szExeFile, target.c_str()) == 0)
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				processes.push_back(hProcess);
			}
		}
	}

	CloseHandle(snapshot);

	return processes;
}

struct EnumData {
	DWORD dwProcessId;
	HWND hWnd;
};

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam) {
	// Retrieve storage location for communication data
	EnumData& ed = *(EnumData*)lParam;
	DWORD dwProcessId = 0x0;
	// Query process ID for hWnd
	GetWindowThreadProcessId(hWnd, &dwProcessId);
	// Apply filter - if you want to implement additional restrictions,
	// this is the place to do so.
	if (ed.dwProcessId == dwProcessId) {
		// Found a window matching the process ID
		ed.hWnd = hWnd;
		// Report success
		SetLastError(ERROR_SUCCESS);
		// Stop enumeration
		return FALSE;
	}
	// Continue enumeration
	return TRUE;
}

HWND FindWindowFromProcessId(DWORD dwProcessId) {
	EnumData ed = { dwProcessId };
	if (!EnumWindows(EnumProc, (LPARAM)&ed) &&
		(GetLastError() == ERROR_SUCCESS)) {
		return ed.hWnd;
	}
	return NULL;
}

// Helper method for convenience
HWND FindWindowFromProcess(HANDLE hProcess) {
	return FindWindowFromProcessId(GetProcessId(hProcess));
}

int main() {
	printf("Starting up...");

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	std::vector<HANDLE> processes = GetProcessesByName("Notepad.exe");
	HANDLE gameProc = NULL;
	HWND windowHandle = NULL;
	HWND firstWindowHandle = NULL;
	if (processes.size() > 0) {

		gameProc = processes[0];
		DWORD gameProcId = GetProcessId(gameProc);

		windowHandle = FindWindowFromProcessId(gameProcId);
		firstWindowHandle = GetWindow(windowHandle, GW_CHILD);

		Bitmap* img = CaptureWindow(firstWindowHandle);
		CLSID myClsId;
		int retVal = GetEncoderClsid(L"image/bmp", &myClsId);
		img->Save(L"output.bmp", &myClsId, NULL);

		delete img;
	}

	
	
	//Define a form object, class form will create a window
	//when a form instance is created.
	//The new window default visibility is false.
	WindowOverlay overlay(windowHandle);
	color col3{ static_cast<color_rgba>(0xFFFFFF00) };
	overlay.bgcolor(nana::colors::white);

	//Define a label on the fm(form) with a specified area,
	//and set the caption.
	label lb{ overlay, rectangle{ 10, 10, 100, 100 } };
	lb.caption("Hello, world!");

	button btn{ overlay, "Quit" };
	btn.events().click([&overlay] {
		overlay.close();
	});

	overlay.div("vert <><<><weight=80% text><>><><weight=24<><button><>><>");
	overlay["text"] << lb;
	overlay["button"] << btn;
	overlay.collocate();

	//Expose the form.
	overlay.show();
	//Pass the control of the application to Nana's event
	//service. It blocks the execution for dispatching user
	//input until the form is closed.

	/*HWND overlayHandle = reinterpret_cast<HWND>(overlay.native_handle());
	LONG lStyle = GetWindowLongPtr(overlayHandle, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLongPtr(overlayHandle, GWL_STYLE, lStyle);

	LONG lExStyle = GetWindowLongPtr(overlayHandle, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
	SetWindowLongPtr(overlayHandle, GWL_EXSTYLE, lExStyle);
	
	SetWindowLong(overlayHandle, -20, GetWindowLong(overlayHandle, -20) | 0x80000 | 0x20);

	SetWindowLong(overlayHandle, GWL_EXSTYLE, GetWindowLong(overlayHandle, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(overlayHandle, 0, 128, LWA_ALPHA);*/
	exec();

	for (HANDLE& proc : processes) {
		CloseHandle(proc);
	}

	GdiplusShutdown(gdiplusToken);

	return processes.size() <= 0;
}