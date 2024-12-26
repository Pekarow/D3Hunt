#pragma once
#include <robmikh.common/DesktopWindow.h>

class App;
class WindowList;
//class MonitorList;

struct SampleWindow : robmikh::common::desktop::DesktopWindow<SampleWindow>
{
    static const std::wstring ClassName;

    SampleWindow(int width, int height, std::shared_ptr<App> app);
    ~SampleWindow();

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
    //struct PixelFormatData
    //{
    //    std::wstring Name;
    //    winrt::Windows::Graphics::DirectX::DirectXPixelFormat PixelFormat;
    //};

 /*   struct DirtyRegionModeData
    {
        std::wstring Name;
        winrt::Windows::Graphics::Capture::GraphicsCaptureDirtyRegionMode Mode;
    };*/

    //struct MinUpdateIntervalData
    //{
    //    std::wstring Name;
    //    winrt::Windows::Foundation::TimeSpan Interval;
    //};

    enum class CaptureType
    {
        ProgrammaticWindow,
        ProgrammaticMonitor,
        Picker,
    };

    static void RegisterWindowClass();
    void CreateControls(HINSTANCE instance);
    void SetSubTitle(std::wstring const& text);
    winrt::fire_and_forget OnPickerButtonClicked();
    winrt::fire_and_forget OnSnapshotButtonClicked();
    void StopCapture();
    void OnCaptureItemClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&);
    void OnCaptureStarted(
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item, 
        CaptureType captureType);

private:
    HWND m_windowComboBox = nullptr;
    HWND m_stopButton = nullptr;
    std::unique_ptr<WindowList> m_windows;
    std::shared_ptr<App> m_app;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker m_itemClosedRevoker;
};