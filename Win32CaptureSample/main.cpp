#pragma once
#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include <opencv2/opencv.hpp>
#include "DofusHuntAnalyzer.h"

#include "DofusDBUtils.h"
#include "ClientSocket.h"
namespace winrt
{
	using namespace Windows::Graphics::Capture;
	using namespace Windows::Storage::Pickers;
	using namespace Windows::UI::Composition;
}

namespace util
{
	using namespace robmikh::common::desktop;
}


//int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
//{
//	WindowList list;
//	cv::Rect r(1280, 720, 1, 1);
//	SendClick(r, list.GetCurrentWindows()[0].WindowHandle, processMonitors());
//	return 0;
//}

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
	 //SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); // works but everything draws small
	 //Initialize COM
	winrt::init_apartment(winrt::apartment_type::single_threaded);

	// Check to see that capture is supported
	auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
	if (!isCaptureSupported)
	{
	    MessageBoxW(nullptr,
	        L"Screen capture is not supported on this device for this release of Windows!",
	        L"Win32CaptureSample",
	        MB_OK | MB_ICONERROR);
	    return 1;
	}

	// Create the DispatcherQueue that the compositor needs to run
	auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

	// Initialize Composition
	auto compositor = winrt::Compositor();
	auto root = compositor.CreateContainerVisual();
	root.RelativeSizeAdjustment({ 1.0f, 1.0f });
	root.Size({ -220.0f, 0.0f });
	root.Offset({ 220.0f, 0.0f, 0.0f });

	// Create the app
	auto app = std::make_shared<App>(root);

	auto window = SampleWindow(220, 150, app);

	// Hookup the visual tree to the window
	auto target = window.CreateWindowTarget(compositor);
	target.Root(root);

	// Message pump
	MSG msg = {};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
	    TranslateMessage(&msg);
	    DispatchMessageW(&msg);
	}
	return util::ShutdownDispatcherQueueControllerAndWait(controller, static_cast<int>(msg.wParam));
}