#pragma once
#include "CaptureUtils.h"
#include < ctime >
#include < Shellscalingapi.h>
#include <vector>
namespace
{
	BOOL CALLBACK GetMonitorByIndex(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{


		POINT* info = (POINT*)dwData;
		MONITORINFOEX monitorInfo;
		::ZeroMemory(&monitorInfo, sizeof(monitorInfo));
		monitorInfo.cbSize = sizeof(monitorInfo);

		BOOL res = ::GetMonitorInfo(hMonitor, &monitorInfo);
		int left_x = monitorInfo.rcMonitor.left;
		int top_y = monitorInfo.rcMonitor.top;

		if (info->x > left_x)
		{
			info->x = left_x;
		}

		if (info->y > top_y)
		{
			info->y = top_y;
		}
		return TRUE;
	}
	POINT processMonitors()
	{
		POINT infos{ 0,0 };

		EnumDisplayMonitors(NULL, NULL, GetMonitorByIndex, (LPARAM)&infos);
		return infos;
	}
}
void SendClick(RECT rect, HWND hwnd)
{
	srand((UINT)time(0));
	POINT p{ rect.left, rect.top };
	POINT diff = processMonitors();

	// Random point un rect, tak einto consideration the offset provided by the virtual screen composition
	int x = p.x + rand() % (rect.left - rect.right) - diff.x;
	int y = p.y + rand() % (rect.top - rect.bottom) - diff.y;

	constexpr double max_coordinate{ 65535.0 };

	/* The default monitor dpi */
	HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(monitor, &mi);
	UINT monitor_x = mi.rcMonitor.left;
	UINT monitor_y = mi.rcMonitor.top;

	UINT dpi_x, dpi_y;
	GetDpiForMonitor(monitor, MDT_DEFAULT, &dpi_x, &dpi_y);
	constexpr UINT default_dpi{ 96 };
	/* Should help adjust for multiple monitors with multiply dpi settings */
	DPI_AWARENESS_CONTEXT const original_dpi_awareness_context{ SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) };
	/* Get the total desktop size */
	int const virtual_screen_width{ GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN, dpi_x) };
	int const virtual_screen_height{ GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, dpi_y) };

	/* Compute the scaling factor for converting x and y to the MOUSEINPUT domain */
	double const width_scale{ static_cast<double>(virtual_screen_width) / max_coordinate };
	double const height_scale{ static_cast<double>(virtual_screen_height) / max_coordinate };

	/* Convert x and y to the MOUSEINPUT domain of 0-65535. */
	LONG domain_x{ static_cast<LONG>(static_cast<double>(x + monitor_x) / width_scale) };
	LONG domain_y{ static_cast<LONG>(static_cast<double>(y + monitor_y) / height_scale) };


	constexpr DWORD time{ 0 };
	constexpr LONG_PTR extra_info{ 0 };
	constexpr DWORD mouse_data{ 0 };
	/* Flags needed to use absolute coordinates for the entire desktop (possible multiple monitors) */
	constexpr DWORD flags{ MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_VIRTUALDESK };

	::INPUT mouse_input{
		.type = INPUT_MOUSE,
		.mi = {
			.dx = (LONG)domain_x,
			.dy = (LONG)domain_y,
			.mouseData = mouse_data,
			.dwFlags = flags,
			.time = time,
			.dwExtraInfo = extra_info,
	},
	};

	///* Revert the dpi settings on the thread */
	SetThreadDpiAwarenessContext(original_dpi_awareness_context);

	SendInput(1, &mouse_input, sizeof(mouse_input));

	mouse_input = {
		.type = INPUT_MOUSE,
		.mi = {
			.dx = (LONG)domain_x,
			.dy = (LONG)domain_y,
			.mouseData = mouse_data,
			.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP,
			.time = time,
			.dwExtraInfo = extra_info,
	},
	};
	SendInput(1, &mouse_input, sizeof(mouse_input));


}

void SendKeys(std::vector<SHORT> keys)
{
	unsigned int size = (UINT)(keys.size() * 2);
	INPUT* inputs = new INPUT[size];
	unsigned int i = 0;
	for (SHORT key : keys)
	{
		UINT mappedkey;
		INPUT input;
		mappedkey = MapVirtualKey(LOBYTE(key), 0);
		SHORT res = GetAsyncKeyState(key);
		input.type = INPUT_KEYBOARD;
		input.ki.dwFlags = KEYEVENTF_SCANCODE;
		input.ki.wScan = mappedkey;
		*(inputs + i) = input;
		i++;
	}

	for (SHORT key : keys)
	{
		UINT mappedkey;
		INPUT input;
		mappedkey = MapVirtualKey(LOBYTE(key), 0);
		SHORT res = GetAsyncKeyState(key);
		input.type = INPUT_KEYBOARD;
		input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		input.ki.wScan = mappedkey;
		*(inputs + i) = input;
		i++;
	}
	SendInput(size, inputs, sizeof(INPUT));
}

void SendKeyPress(SHORT key)
{
	UINT mappedkey;
	INPUT input;
	mappedkey = MapVirtualKey(LOBYTE(key), 0);
	SHORT res = GetAsyncKeyState(key);
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_SCANCODE;
	input.ki.wScan = mappedkey;
	SendInput(1, &input, sizeof(INPUT));
}

void SendKeyRelease(SHORT key)
{
	UINT mappedkey;
	INPUT input;
	mappedkey = MapVirtualKey(LOBYTE(key), 0);
	SHORT res = GetAsyncKeyState(key);
	input.type = INPUT_KEYBOARD;
	input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	input.ki.wScan = mappedkey;
	SendInput(1, &input, sizeof(INPUT));
}

void writeText(std::string msg)
{
	std::vector<INPUT> vec;
	for (auto ch : msg)
	{
		SHORT key = VkKeyScan(ch);;
		INPUT input = { 0 };
		input.type = INPUT_KEYBOARD;
		input.ki.dwFlags = KEYEVENTF_UNICODE;
		input.ki.wScan = ch;
		vec.push_back(input);

		input.ki.dwFlags |= KEYEVENTF_KEYUP;
		vec.push_back(input);
	}
	SendInput((UINT)vec.size(), vec.data(), sizeof(INPUT));
}
