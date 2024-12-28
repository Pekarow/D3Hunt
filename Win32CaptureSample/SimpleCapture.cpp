#include "pch.h"
#include "SimpleCapture.h"
#include "DofusHuntAnalyzer.h"
#include "DofusDBUtils.h"

#define RETURN_SET_STATE(state)\
	mState = state; \
	cv::imshow("show", c); \
	cv::waitKey(0); \
	mProcessingFrame = false; \
	return;
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
#define Z 0x00
#define CTRL 0x00
#define ARROW_UP 0x00
#define ARROW_DOWN 0x00
#define ARROW_LEFT 0x00
#define ARROW_RIGHT 0x00

SimpleCapture::SimpleCapture(
	winrt::IDirect3DDevice const& device,
	winrt::GraphicsCaptureItem const& item,
	winrt::DirectXPixelFormat pixelFormat, HWND hwnd, ClientSocket * socket)
{
	mSocket = socket;
	mHWND = hwnd;
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
		m_framePool = nullptr;
		m_session = nullptr;
		m_item = nullptr;
	}
}
bool SimpleCapture::getNextFrame(cv::Mat& c, winrt::Direct3D11CaptureFramePool const& sender)
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

	ID3D11Texture2D* stagingTexture = nullptr;
	m_d3dDevice.get()->CreateTexture2D(&desc, nullptr, &stagingTexture);

	auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
	m_d3dContext->CopyResource(stagingTexture, frameSurface.get());

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	m_d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTex);
	m_d3dContext->Unmap(stagingTexture, 0);

	c = cv::Mat(desc.Height, desc.Width, CV_8UC4, mappedTex.pData, mappedTex.RowPitch).clone();
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
	return true;
}
void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{
	cv::Mat c;
	bool ok = getNextFrame(c, sender);
	if (!ok || c.empty())
	{
		return;
	}
	if (mProcessingFrame)
		return;
	mProcessingFrame = true;



	State previousState = mState;
	DofusHuntAnalyzer analyzer(c);
	bool hunt_started = analyzer.interfaceFound();
	if (!hunt_started)
	{
		RETURN_SET_STATE(State::NoHunt);
	}
	else if (previousState == State::NoHunt)
	{
		RETURN_SET_STATE(State::StartHunt);
	}
	// Process travelling to target position ...
	if (previousState == State::WaitingToReachHuntPostion ||
		previousState == State::WaitingToReachHintPostion ||
		previousState == State::WaitingToReachPhorreurPostion)
	{
		// Target not reached, keep waiting ...
		if (analyzer.getCurrentPosition() != mTargetPosition)
		{
			RETURN_SET_STATE(previousState);
		}

		// Target position has been reached ...
		if (previousState == State::WaitingToReachHuntPostion)
		{
			RETURN_SET_STATE(State::StartHunt);
		}
		else if (previousState == State::WaitingToReachHintPostion)
		{
			// Validate hint
			sendClick(analyzer.getLastHintValidationPosition());
			RETURN_SET_STATE(State::StartHunt);
		}
		else if (previousState == State::WaitingToReachPhorreurPostion)
		{
			// Press on Z to display Phorreur
			sendCommand(KeyboardState::PUSH, { Z });
			RETURN_SET_STATE(State::SearchPhorreur);
		}
	}
	if (previousState == State::SearchPhorreur)
	{
		State phorreur_state = State::NoHunt;
		if (analyzer.isPhorreurFound())
		{
			// Validate hint
			sendClick(analyzer.getLastHintValidationPosition());
			phorreur_state = State::StartHunt;
		}
		else
		{
			int direction = analyzer.getLastHintDirection();
			// Travel 1 map in direction
			sendCommand(KeyboardState::PUSH_RELEASE, { CTRL, ARROW_UP });
			// TODO: Make direction a vector
			// mTargetPosition = analyzer.getCurrentPosition() + direction;
			phorreur_state = State::WaitingToReachPhorreurPostion;
		}
		// TODO: Release Z press, could be done via signal?
		sendCommand(KeyboardState::RELEASE, { Z });
		RETURN_SET_STATE(phorreur_state);
	}
	if (previousState == State::StartHunt)
	{
		if (analyzer.getHuntStep() == 1 &&
			analyzer.getLastHintIndex() == 1 &&
			analyzer.getCurrentPosition() != analyzer.getStartPosition())
		{
			RETURN_SET_STATE(State::WaitingToReachHuntPostion);
		}
		// Retrieve infos on the hunt
		auto [x, y] = analyzer.getCurrentPosition();
		std::string hint = analyzer.getLastHint();
		DofusDB::DBInfos infos = { x, y, analyzer.getLastHintDirection(), 0,  hint };

		if (hint.find("Phorreur") != std::string::npos)
		{
			mPhorreurType = hint;
			RETURN_SET_STATE(State::SearchPhorreur);
		}

		mSocket.send(DofusDB::DBInfos2json(infos).c_str());
		char* out;
		int size;
		mSocket.receive(&out, size);
		std::string out_s(out);
		// TODO: Go to pos "out"
		RETURN_SET_STATE(State::WaitingToReachHintPostion);
	}
}

void SimpleCapture::sendCommand(KeyboardState action, std::list<int> target)
{
}

void SimpleCapture::sendClick(cv::Rect area)
{
}
