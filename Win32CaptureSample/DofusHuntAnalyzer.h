#pragma once
#include <vector>
#include <utility>
#include <string>
#include <opencv2/opencv.hpp>
#include <sstream>
namespace tesseract
{
	class TessBaseAPI;
	enum PageSegMode;
}
class DofusHuntAnalyzer
{
public:
	DofusHuntAnalyzer(cv::Mat& image, tesseract::TessBaseAPI* tesseract_api);
	bool interfaceFound();

	std::pair<int, int> getStartPosition();
	std::pair<int, int> getCurrentPosition();
	std::string getLastHint();
	int getLastHintDirection();
	bool isHuntFinished();
	int getLastHintIndex();
	bool isLastHintLast() { return mLastHintIndex == (mNbHint - 1); };
	bool isHintChecked(cv::Rect rect);
	int getHuntStep();
	int getMaxHuntStep();
	bool isStepFinished();
	cv::Rect getLastHintValidationPosition();
	std::pair<int, int> getStepValidationPosition();
	bool isPhorreurFound();
	cv::Mat* getDebugImage() { return &mImageDebug; };
	void resetTesseractAPI(tesseract::PageSegMode mode, const std::string& whitelist);
	std::string getPreciseTextFromImage(cv::Mat& image);
	std::string toString() const;
	static DofusHuntAnalyzer fromString(const std::string& str);

	bool operator==(const DofusHuntAnalyzer& other) const
	{
		return mCurrentPositionFound == other.mCurrentPositionFound &&
			mHuntInfosFound == other.mHuntInfosFound &&
			mHuntAreaFound == other.mHuntAreaFound &&
			mInterfaceRect == other.mInterfaceRect &&
			mLastHintDirection == other.mLastHintDirection &&
			mLastHintIndex == other.mLastHintIndex &&
			mNbHint == other.mNbHint &&
			mLastHintValidationPosition == other.mLastHintValidationPosition &&
			mHuntArea == other.mHuntArea &&
			mCurrentStep == other.mCurrentStep &&
			mMaxStep == other.mMaxStep &&
			mStartPosition == other.mStartPosition &&
			mCurrentPosition == other.mCurrentPosition;
	}

private:
	void initHuntInfos();
	void findInterface();
	void findHuntArea();
	void findCurrentPos();
	std::vector<cv::Rect> FindRectInImage(
		cv::Mat& image, 
		cv::Scalar lower_bound, 
		cv::Scalar upper_bound, 
		std::string text_content="", 
		int minimum_width=10, 
		int minimum_height = 10);
	bool containsText(cv::Mat& image, std::string text);
private:
	bool mCurrentPositionFound = false;
	bool mHuntInfosFound = false;
	bool mHuntAreaFound=false;
	cv::Rect mInterfaceRect;
	int mLastHintDirection = -1;
	int mLastHintIndex = -1;
	int mNbHint = -1;
	cv::Rect mLastHintValidationPosition;
	cv::Rect mHuntArea;
	cv::Mat& mImage;
	cv::Mat mImageDebug;
	std::string mLastHint;
	int mCurrentStep = -1;
	int mMaxStep = -1;
	std::pair<int, int> mStartPosition;
	std::pair<int, int> mCurrentPosition;
	tesseract::TessBaseAPI* mTesseractAPI;
};

