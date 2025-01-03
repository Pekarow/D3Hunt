#include "pch.h"
#include <windows.h>
#include "SimpleCapture.h"
#include "DofusHuntAnalyzer.h"
#include "DofusDBUtils.h"
#include < ctime >
#define RETURN_SET_STATE(state)\
	mState = state;\
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
#define Z 0x5A
#define ENTER VK_RETURN
#define TAB VK_TAB
#define CTRL VK_CONTROL
#define ARROW_UP VK_UP
#define ARROW_DOWN VK_DOWN
#define ARROW_LEFT VK_LEFT
#define ARROW_RIGHT VK_RIGHT
#define CTRL_ARROW_UP {CTRL, ARROW_UP}
#define CTRL_ARROW_DOWN {CTRL, ARROW_DOWN}
#define CTRL_ARROW_LEFT {CTRL, ARROW_LEFT}
#define CTRL_ARROW_RIGHT {CTRL, ARROW_RIGHT}
namespace
{
	
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

	float lint2(float a, float b, float f, float f2)
	{
		return std::lerp(a, b, f) - std::lerp(a, b, f2);
	}

	void SendClick(cv::Rect rect, HWND hwnd)
	{
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hwnd, &p);
		srand((UINT)time(0));

		int randomX = rect.x + rand() % rect.width;
		int randomY = rect.y + rand() % rect.height;

		int max_i = 100;
		INPUT* Inputs = new INPUT[max_i];
		Inputs[0] = { 0 };

		for (int i = 1; i < max_i; i++)
		{
			int calc_x = lint2(p.x, rect.x, (float)i / max_i, (float)(i - 1) / max_i);
			int calc_y = lint2(p.y, rect.y, (float)i / max_i, (float)(i - 1) / max_i);
			Inputs[i] = { 0 };
			Inputs[i].type = INPUT_MOUSE;
			Inputs[i].mi.dx = calc_x; // desired X coordinate
			Inputs[i].mi.dy = calc_y; // desired Y coordinate
			Inputs[i].mi.dwFlags = MOUSEEVENTF_MOVE;
		}

		Inputs[max_i - 1] = { 0 };
		Inputs[max_i - 1].type = INPUT_MOUSE;
		Inputs[max_i - 1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

		SendInput(max_i-1, Inputs, sizeof(INPUT));
		
		SetCursorPos(randomX, randomY);
		SendInput(1, &(Inputs[max_i - 1]), sizeof(INPUT));
		SetCursorPos(p.x, p.y);

		delete[] Inputs;
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
		SendInput(vec.size(), vec.data(), sizeof(INPUT));
	}
	void travelTo(cv::Point point)
	{
		using namespace std::literals;
		// Focus on chat
		SendKeys({ TAB });
		Sleep(200);
		writeText("/travel "s + std::to_string(point.x) + " "s + std::to_string(point.y));
		Sleep(100);
		SendKeys({ ENTER });
		Sleep(500);
		SendKeys({ ENTER });

	}
}

SimpleCapture::SimpleCapture(
	winrt::IDirect3DDevice const& device,
	winrt::GraphicsCaptureItem const& item,
	winrt::DirectXPixelFormat pixelFormat, HWND hwnd, ClientSocket* socket)
{
	m_lastSize = { -1, -1 };
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
	EventRAII ev(*this);
	State previousState = mState;
	DofusHuntAnalyzer analyzer(c);
	ev.setImage(analyzer.getDebugImage());
	bool hunt_started = analyzer.interfaceFound();
	if (!hunt_started)
	{
		RETURN_SET_STATE(previousState);
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
			SendClick(analyzer.getLastHintValidationPosition(), mHWND);
			RETURN_SET_STATE(State::StartHunt);
		}
		else if (previousState == State::WaitingToReachPhorreurPostion)
		{
			// Press on Z to display Phorreur
			SendKeyPress(Z);
			RETURN_SET_STATE(State::SearchPhorreur);
		}
	}
	if (previousState == State::SearchPhorreur)
	{
		State phorreur_state = State::NoHunt;
		if (analyzer.isPhorreurFound())
		{
			// Validate hint
			SendClick(analyzer.getLastHintValidationPosition(), mHWND);
			phorreur_state = State::StartHunt;
		}
		else
		{
			int direction = analyzer.getLastHintDirection();
			// Travel 1 map in direction
			SendKeys({ CTRL, ARROW_UP });
			// TODO: Make direction a vector
			// mTargetPosition = analyzer.getCurrentPosition() + direction;
			phorreur_state = State::WaitingToReachPhorreurPostion;
		}
		// TODO: Release Z press, could be done via signal?
		SendKeyRelease(Z);
		RETURN_SET_STATE(phorreur_state);
	}
	if (previousState == State::StartHunt)
	{
		if (analyzer.getHuntStep() == 1 &&
			analyzer.getLastHintIndex() == 0 &&
			analyzer.getCurrentPosition() != analyzer.getStartPosition())
		{
			mTargetPosition = analyzer.getStartPosition();
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

		mSocket->send(DofusDB::DBInfos2json(infos).c_str());
		char* out;
		int size;
		mSocket->receive(&out, size);
		std::string out_s(out);
		// TODO: Go to pos "out"
		int index = (int)out_s.find("/");
		cv::Point p = { std::stoi(out_s.substr(0, index)),std::stoi(out_s.substr(index + 1)) };
		SetForegroundWindow(mHWND);
		travelTo(p);
		mTargetPosition = { p.x, p.y };
		RETURN_SET_STATE(State::WaitingToReachHintPostion);
	}
}

inline SimpleCapture::EventRAII::EventRAII(SimpleCapture& sc) : mCapture(sc) { 
	sc.setProcessingFrame(true); 
}

void SimpleCapture::EventRAII::setImage(cv::Mat* image)
{
	mImage = image;
	//cv::namedWindow("RAII", cv::WINDOW_NORMAL);
}

inline SimpleCapture::EventRAII::~EventRAII() {
	if(mImage)
	{
		//cv::imshow("RAII", *mImage);
		//cv::waitKey(0);
	}
	mCapture.setProcessingFrame(false);
}
