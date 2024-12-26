#include "pch.h"
#include "SimpleCapture.h"
#include <opencv2/opencv.hpp>
#include "DofusHuntAnalyzer.h"
namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::uwp;
}

SimpleCapture::SimpleCapture(
    winrt::IDirect3DDevice const& device,
    winrt::GraphicsCaptureItem const& item, 
    winrt::DirectXPixelFormat pixelFormat)
{
    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;

    m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    m_d3dDevice->GetImmediateContext(m_d3dContext.put());

    m_framePool = winrt::Direct3D11CaptureFramePool::Create(m_device, m_pixelFormat, 2, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}


void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        //m_swapChain = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{
    auto newSize = false;
    auto frame = sender.TryGetNextFrame();
    auto frameContentSize = frame.ContentSize();

    D3D11_TEXTURE2D_DESC desc;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = { 1,0 };
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    if (frameContentSize.Width != m_lastSize.Width ||
        frameContentSize.Height != m_lastSize.Height) {
        newSize = true;
        m_lastSize = frameContentSize;
    }
    desc.Width = m_lastSize.Width;
    desc.Height = m_lastSize.Height;

    ID3D11Texture2D* stagingTexture;
    m_d3dDevice.get()->CreateTexture2D(&desc, nullptr, &stagingTexture);

    auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
    m_d3dContext->CopyResource(stagingTexture, frameSurface.get());

    D3D11_MAPPED_SUBRESOURCE mappedTex;
    m_d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTex);
    m_d3dContext->Unmap(stagingTexture, 0);

    cv::Mat c = cv::Mat(desc.Height, desc.Width, CV_8UC4, mappedTex.pData, mappedTex.RowPitch);


    auto analyzer = DofusHuntAnalyzer(c);
    bool hunt_found = analyzer.interfaceFound();
    std::cout << "not found" << std::endl;
    //Detect current hunt step
    cv::imshow("show", c);

    stagingTexture->Release();


    DXGI_PRESENT_PARAMETERS presentParameters = { 0 };

    if (newSize)
    {
        m_framePool.Recreate(
            m_device,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            m_lastSize);
    }
}
