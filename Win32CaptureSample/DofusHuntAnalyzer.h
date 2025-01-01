#pragma once
#include <vector>
#include <utility>
#include <string>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>

class DofusHuntAnalyzer
{
public:
	DofusHuntAnalyzer(cv::Mat& image);
	bool interfaceFound();

	std::pair<int, int> getStartPosition();
	std::pair<int, int> getCurrentPosition();
	std::string getLastHint();
	int getLastHintDirection();
	bool isHuntFinished();
	int getLastHintIndex();

	int getHuntStep();
	int getMaxHuntStep();
	cv::Rect getLastHintValidationPosition();
	std::pair<int, int> getStepValidationPosition();
	bool isPhorreurFound();
	cv::Mat* getDebugImage() { return &mImageDebug; };


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
	cv::Rect mLastHintValidationPosition;
	cv::Rect mHuntArea;
	cv::Mat& mImage;
	cv::Mat mImageDebug;
	std::string mLastHint;
	int mCurrentStep = -1;
	int mMaxStep = -1;
	std::pair<int, int> mStartPosition;
	std::pair<int, int> mCurrentPosition;
};

