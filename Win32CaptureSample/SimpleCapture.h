#pragma once
#include "ClientSocket.h"
#include <opencv2/opencv.hpp>
enum class State
{
    NoHunt,
    WaitingToReachHuntPostion,
    WaitingToReachPhorreurPostion,
    WaitingToReachHintPostion,
    StartHunt,
    SearchPhorreur,
    ClickLastHint,
    ClickStep,
    ClickFight,
    Fight,
    Ended
};
enum class KeyboardState
{
    PUSH,
    RELEASE,
    PUSH_RELEASE
}; 

class SimpleCapture
{
public:
    SimpleCapture(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat,
        HWND hwnd,
        ClientSocket * socket);
    ~SimpleCapture() { Close(); }

    void StartCapture();
    void Close();

private:
    bool getNextFrame(cv::Mat& out, winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender);
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);
    inline void CheckClosed()
    {
        if (m_closed.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }
protected:
    inline void setProcessingFrame(bool proc) {
        mProcessingFrame = proc;
    }
private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;
    POINT processMonitors();
    POINT mTopLeftDiff;
    std::atomic<bool> m_closed = false;
    std::atomic < State > mState = State::NoHunt;
    std::atomic < std::pair<int, int>> mTargetPosition;
    std::string mPhorreurType;
    ClientSocket * mSocket = nullptr;
    HWND mHWND;
    std::atomic<bool> mProcessingFrame = false;
    std::atomic < cv::Rect>  mCurrentHintValidation;
    std::atomic<int> mCurrentStep = -1;
    winrt::Windows::Foundation::TimeSpan mT;
    friend class EventRAII;
    class EventRAII
    {
    public:
        EventRAII(SimpleCapture& sc);
        void setImage(cv::Mat* image);
        ~EventRAII();
    private:
        SimpleCapture& mCapture;
        cv::Mat* mImage = nullptr;
    };
};