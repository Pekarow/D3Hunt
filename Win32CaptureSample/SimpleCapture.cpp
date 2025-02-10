#include "pch.h"
#include <windows.h>
#include "SimpleCapture.h"
#include "DofusHuntAnalyzer.h"
#include "DofusDBUtils.h"

#include <tesseract/baseapi.h>
#include <chrono>
#include <iomanip>
#include <fstream>
#include "CaptureUtils.h"
#include <thread>
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

namespace
{
	RECT toRECT(cv::Rect r)
	{
		RECT R;
		R.left = r.x;
		R.right = r.x + r.width;
		R.top = r.y;
		R.bottom = r.y + r.height;
		return R;
	}
	static tesseract::TessBaseAPI* gTesseractAPI = nullptr;
	std::string getCurrentTimestamp() {
		// Get the current time
		auto now = std::chrono::system_clock::now();
		std::time_t now_time = std::chrono::system_clock::to_time_t(now);

		// Convert to local time and format as a string
		std::tm local_time;
		localtime_s(&local_time, &now_time); // Windows-specific local time conversion

		std::stringstream ss;
		ss << std::put_time(&local_time, "%Y%m%d_%H%M%S");

		return ss.str();
	}
	void initTesseractAPI()
	{
		if (!gTesseractAPI)
		{
			gTesseractAPI = new tesseract::TessBaseAPI();
			printf("Tesseract-ocr version: %s\n",
				gTesseractAPI->Version());
			// Initialize tesseract-ocr with English, without specifying tessdata path
			if (gTesseractAPI->Init(NULL, "fra+eng")) {
				fprintf(stderr, "Could not initialize tesseract.\n");
				assert(false);
				return;
			}
			//resetTesseractAPI(tesseract::PSM_SPARSE_TEXT, TESSERACT_FILTER);
		}
	}

	void WriteTestData(cv::Mat& c, DofusHuntAnalyzer& analyzer)
	{
		std::string timestamp = getCurrentTimestamp();
		std::string outputFilename = "D:/D3Hunt/test_data/output_" + timestamp;
		// Save image for test purposes
		if (!cv::imwrite(outputFilename + ".jpg", c)) {
			std::cerr << "Error: Could not save image." << std::endl;
			return;
		}

		auto s = analyzer.toString();
		std::string filename = outputFilename + ".txt";

		// Create an output file stream object
		std::ofstream outFile(filename);

		// Check if the file stream was created successfully
		if (!outFile) {
			std::cerr << "Error: Could not open the file " << filename << " for writing." << std::endl;
			return;
		}

		// Write the string to the file
		outFile << s;

	}

	bool looksLikePhorreur(std::string& s)
	{
		if (s.find("Porreur") != std::string::npos) return true;
		if (s.find("Phorreur") != std::string::npos) return true;
		if (s.find("Phoreur") != std::string::npos) return true;
		if (s.find("Poreur") != std::string::npos) return true;
		if (s.find("orreur") != std::string::npos) return true;
		if (s.find("Pnorreur") != std::string::npos) return true;
		return false;
	}


	//LONG lint2(LONG a, LONG b, LONG f, LONG f2)
	//{
	//	return std::lerp(a, b, f) - std::lerp(a, b, f2);
	//}

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

	m_framePool = winrt::Direct3D11CaptureFramePool::Create(m_device, m_pixelFormat, 1, m_item.Size());
	mT = std::chrono::seconds(1);
	m_session = m_framePool.CreateCaptureSession(m_item);
	initTesseractAPI();
	m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });
}

void SimpleCapture::StartCapture()
{
	CheckClosed();
	m_session.IsCursorCaptureEnabled(false);
	//m_session.MinUpdateInterval(mT);
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
	if (frame == NULL)
		return false;
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
	if (!stagingTexture) return false;
	auto frameSurface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
	m_d3dContext->CopyResource(stagingTexture, frameSurface.get());

	D3D11_MAPPED_SUBRESOURCE mappedTex;
	m_d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedTex);
	m_d3dContext->Unmap(stagingTexture, 0);
	cv::Mat cc = cv::Mat(desc.Height, desc.Width, CV_8UC4, mappedTex.pData, mappedTex.RowPitch);
	//cv::imshow("test", cc);
	//cv::waitKey(0);
	c = cc.clone();
	stagingTexture->Release();

	DXGI_PRESENT_PARAMETERS presentParameters = { 0 };

	if (newSize)
	{
		m_framePool.Recreate(
			m_device,
			winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
			1,
			m_lastSize);
	}
	return true;
}

void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{

	cv::Mat c;
	bool ok = getNextFrame(c, sender);
	if (c.empty())
	{
		return;
	}
	auto t = std::thread([this, c]
		{
			try
			{
				processFrame(c);
			}
			catch (...)
			{
				return;
			}
		});
	t.detach();

}
void SimpleCapture::processFrame(cv::Mat frame)
{
	if (!mMutex.try_lock())
	{
		return;
	}
	EventRAII ev(*this);
	
	State previousState = mState;
	std::shared_ptr<DofusHuntAnalyzer> analyzer_ptr(new DofusHuntAnalyzer(frame, gTesseractAPI));
	DofusHuntAnalyzer& analyzer = *analyzer_ptr;
	if (mAnalyzer && mAnalyzer->interfaceFound())
	{
		analyzer.createAnalyzerFromExistingData(*mAnalyzer);
	}
	static int click_tries = 0;
	//ev.setImage(analyzer.getDebugImage());
	bool hunt_started = analyzer.interfaceFound();
	if (!hunt_started)
	{
		RETURN_SET_STATE(previousState);
	}
	else if (previousState == State::NoHunt)
	{
		mAnalyzer = analyzer_ptr;
		RETURN_SET_STATE(State::StartHunt);
	}

	if (analyzer.isHuntFinished())
	{
		RETURN_SET_STATE(State::ClickFight);
		Close();
	}

	if ((analyzer.isStepFinished() && previousState != State::StepClicked) || previousState == State::ClickStep)
	{
		if (mCurrentStep == -1)
		{
			mCurrentStep = analyzer.getHuntStep();
		}
		SendClick(toRECT(analyzer.getStepValidationPosition()), mHWND);
		click_tries = 0;
		RETURN_SET_STATE(State::StepClicked);
	}
	else if (previousState == State::StepClicked)
	{
		if (mCurrentStep + 1 == analyzer.getHuntStep())
		{
			mCurrentStep = -1;
			RETURN_SET_STATE(State::StartHunt);
		}


		click_tries++;
		if (click_tries > 10)
		{
			RETURN_SET_STATE(State::ClickStep);
		}
		RETURN_SET_STATE(previousState);
	}
	else if (previousState == State::ClickLastHint)
	{
		SendClick(toRECT(mCurrentHintValidation), mHWND);
		click_tries = 0;
		RETURN_SET_STATE(State::LastHintClicked);
	}
	else if (previousState == State::LastHintClicked)
	{
		bool is_checked = analyzer.isHintChecked(mCurrentHintValidation);
		if (is_checked)
		{
			mCurrentHintValidation = cv::Rect();
			RETURN_SET_STATE(State::StartHunt);
		}
		click_tries++;
		if (click_tries > 10)
		{
			RETURN_SET_STATE(State::ClickLastHint);
		}
		RETURN_SET_STATE(previousState);
	}
	// Process travelling to target position ...
	else if (previousState == State::WaitingToReachHuntPostion ||
		previousState == State::WaitingToReachHintPostion)
	{
		// Target not reached, keep waiting ...
		if (analyzer.getCurrentPosition() != mTargetPosition.load())
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
			mCurrentHintValidation = analyzer.getLastHintValidationPosition();
			RETURN_SET_STATE(State::ClickLastHint);
		}
	}
	else if (previousState == State::WaitingToReachPhorreurPostion)
	{
		// Press on Z to display Phorreur
		//SendKeyPress(Z);
		mCurrentHintValidation = analyzer.getLastHintValidationPosition();
		RETURN_SET_STATE(State::SearchPhorreur);
	}
	else if (previousState == State::SearchPhorreur)
	{
		// Validate hint
		bool is_checked = analyzer.isHintChecked(mCurrentHintValidation);
		if (!is_checked)
		{
			//SendClick(current, mHWND);
			//Sleep(3);
			RETURN_SET_STATE(previousState);
		}
		//SendKeyRelease(Z);
		mCurrentHintValidation = cv::Rect();
		RETURN_SET_STATE(State::StartHunt);
		//State phorreur_state = State::NoHunt;
		//if (analyzer.isPhorreurFound())
		//{
		//	// Validate hint
		//	//SendClick(analyzer.getLastHintValidationPosition(), mHWND);
		//	phorreur_state = State::StartHunt;
		//}
		//else
		//{
		//	int direction = analyzer.getLastHintDirection();
		//	// Travel 1 map in direction
		//	SendKeys({ CTRL, ARROW_UP });
		//	// TODO: Make direction a vector
		//	// mTargetPosition = analyzer.getCurrentPosition() + direction;
		//	phorreur_state = State::WaitingToReachPhorreurPostion;
		//}
		//// TODO: Release Z press, could be done via signal?
		//SendKeyRelease(Z);
		//RETURN_SET_STATE(phorreur_state);
	}
	else if (previousState == State::StartHunt)
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
		if (looksLikePhorreur(hint))
		{
			mPhorreurType = hint;
			RETURN_SET_STATE(State::WaitingToReachPhorreurPostion);
		}

		//WriteTestData(frame, analyzer);
		DofusDB::DBInfos infos = { x, y, analyzer.getLastHintDirection(), 0,  hint };
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
	if (mImage)
	{
		//cv::imshow("RAII", *mImage);
		//cv::waitKey(0);
	}
	mCapture.mMutex.unlock();
	mCapture.setProcessingFrame(false);
}
