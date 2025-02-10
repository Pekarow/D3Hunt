#include "DofusHuntAnalyzer.h"
#include <string>
#include <ranges>
#include <cctype>
#include <math.h>
#include <cmath>
#include <regex>
#include <tesseract/baseapi.h>
using namespace std;
using namespace cv;

namespace
{
	const Scalar INTERFACE_LOWER = Scalar(103, 0, 41);
	const Scalar INTERFACE_UPPER = Scalar(146, 255, 255);
	
	const Scalar CURRENT_POS_LOWER = Scalar(0, 0, 10);
	const Scalar CURRENT_POS_UPPER = Scalar(0, 0, 255);

	const Scalar TITLE_LOWER = Scalar(98, 60, 83);
	const Scalar TITLE_UPPER = Scalar(180, 255, 190);
	const Scalar HINT_LOWER = Scalar(106, 0, 0);
	const Scalar HINT_UPPER = Scalar(180, 255, 71);
	const Scalar VALIDATE_LOWER = Scalar(34, 145, 109);
	const Scalar VALIDATE_UPPER = Scalar(40, 255, 186);

	const string TITLE_S = "CHASSE AUX TRESORS";

	const double X_DIRECTION_RATIO = .1;
	const double X_INPROGRESS_RATIO = .25;
	const string TESSERACT_FILTER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefgijklmnopqrstuvwxyz Ééèêçà'ô[]()?/,-%";
	
	// trim from start (in place)
	inline void ltrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
			}));
	}

	// trim from end (in place)
	inline void rtrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
			}).base(), s.end());
	}

	inline void trim(string& s)
	{
		rtrim(s);
		ltrim(s);
	}
	typedef std::pair<double, Point> distPoint;
	Rect findInnerRect(vector<Point> contour, Point mdidle_point)
	{
		distPoint top_left = { 9999, mdidle_point };
		distPoint bot_left = { 9999, mdidle_point };
		distPoint top_right = { 9999, mdidle_point };
		distPoint bot_right = { 9999, mdidle_point };


		for (const Point& point : contour)
		{
			double dist = std::abs(mdidle_point.x - point.x) ^ 2 + std::abs(mdidle_point.y - point.y) ^ 2;

			if (((point.x > 0) && (point.x < mdidle_point.x)) && ((point.y > 0) && (point.y < mdidle_point.y)))
			{
				if (top_left.first > dist)
				{
					top_left.second = point;
				}
			}
			else if (((point.x > mdidle_point.x) && (point.x < 2 * mdidle_point.x)) && ((point.y > 0) && (point.y < mdidle_point.y)))
			{
				if (top_right.first > dist)
				{
					top_right.second = point;
				}
			}
			else if (((point.x > 0) && (point.x < mdidle_point.x)) && ((point.y > mdidle_point.y) && (point.y < 2 * mdidle_point.y)))
			{
				if (bot_left.first > dist)
				{
					bot_left.second = point;
				}
			}
			else if (((point.x > mdidle_point.x) && (point.x < 2 * mdidle_point.x)) && ((point.y > mdidle_point.y) && (point.y < 2 * mdidle_point.y)))
			{
				if (bot_right.first > dist)
				{
					bot_right.second = point;
				}
			}
		}


		vector<Point> inner_contour = { top_left.second  ,  top_right.second  ,  bot_left.second  ,  bot_right.second };
		return cv::boundingRect(inner_contour);
	}

	
	string getTextFromImage(Mat& image, tesseract::TessBaseAPI* tess, bool remove_endl = true, bool requires_processing = true)
	{
		cv::Mat grey;
		cv::cvtColor(image, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);
		vector<Rect> valid_text_areas;
		if (requires_processing)
		{
			Mat blur;
			cv::GaussianBlur(grey, blur, Size(9, 9), 0);


			Mat adaptive_thresh;
			cv::adaptiveThreshold(blur, adaptive_thresh, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY_INV, 11, 5);
			// Dilate to combine adjacent text contours
			Mat kernel = cv::getStructuringElement(MORPH_RECT, Size(5, 5));
			Mat dilate;
			cv::dilate(adaptive_thresh, dilate, kernel, Point(-1, -1), 4);

			// Find contours, highlight text areas, and extract ROIs
			vector<vector<Point>> cnts;
			vector<Vec4i> hierarchy;

			cv::findContours(dilate, cnts, hierarchy, RETR_LIST, CHAIN_APPROX_SIMPLE);

			for (const auto& c : cnts)
			{
				double area = cv::contourArea(c);
				if (area < 1000)
					continue;
				Rect r = cv::boundingRect(c);
				valid_text_areas.push_back(r);
				//cv::rectangle(image, r, Scalar(255, 0, 255), 1);
			}
		}
		else
		{
			valid_text_areas.push_back(Rect(0, 0, grey.cols, grey.rows));
		}


		std::string output;
		for (const auto& text_area : valid_text_areas)
		{
			Mat sub_area(grey, text_area);

			cv::threshold(sub_area, sub_area, 0, 255, THRESH_OTSU | THRESH_BINARY_INV);

			// recognize text
			tess->SetImage(sub_area.data, sub_area.cols, sub_area.rows, 1, (int)sub_area.step);
			string t1(tess->GetUTF8Text());
			if (remove_endl)
			{
				t1.erase(std::remove(t1.begin(), t1.end(), '\n'), t1.end());
			}
			if (!output.empty())
			{
				output.append(" ");
			}
			output.append(t1);
		}
		return output;
	}

	int getDirection(Mat image)
	{
		cv::Mat grey;
		cv::cvtColor(image, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);

		cv::Mat thresh;

		double ret = cv::threshold(grey, thresh, 0, 255, THRESH_OTSU | THRESH_BINARY_INV);

		vector<vector<Point>> cnts;
		vector<Vec4i> hierarchy;
		cv::findContours(thresh, cnts, hierarchy, RETR_LIST, CHAIN_APPROX_NONE);
		vector<Rect> valid_text_areas;
		for (const auto& c : cnts)
		{
			double area = cv::contourArea(c);
			if (area > 500 || area < 50)
				continue;
			Rect r = cv::boundingRect(c);
			valid_text_areas.push_back(r);
			//cv::rectangle(image, r, Scalar(0, 255, 255), 1);
			break;
		}
		if (valid_text_areas.size() != 1)
		{
			return -1;
		}
		assert(valid_text_areas.size() == 1);
		Rect r = valid_text_areas[0];

		int x_middle = (int)(r.width / 2);
		int y_middle = (int)(r.height / 2);
		Rect top_left_r(r.x, r.y, x_middle, y_middle);
		Rect top_right_r(r.x + x_middle, r.y, x_middle, y_middle);
		Rect bot_left_r(r.x, r.y + y_middle, x_middle, y_middle);
		Rect bot_right_r(r.x + x_middle, r.y + y_middle, x_middle, y_middle);


		Mat top_left_mat = Mat(thresh, top_left_r);
		Mat top_right_mat = Mat(thresh, top_right_r);
		Mat bot_left_mat = Mat(thresh, bot_left_r);
		Mat bot_right_mat = Mat(thresh, bot_right_r);
		int top_left = cv::countNonZero(top_left_mat);
		int top_right = cv::countNonZero(top_right_mat);
		int bot_left = cv::countNonZero(bot_left_mat);
		int bot_right = cv::countNonZero(bot_right_mat);

		std::vector<int> values = { top_left, top_right, bot_left, bot_right };
		std::sort(values.begin(), values.end());
		if ((values[0] == top_left && values[1] == top_right) || (values[1] == top_left && values[0] == top_right))
		{
			return 0;
		}
		if ((values[0] == top_right && values[1] == bot_right) || (values[1] == top_right && values[0] == bot_right))
		{
			return 1;
		}
		if ((values[0] == bot_left && values[1] == bot_right) || (values[1] == bot_left && values[0] == bot_right))
		{
			return 2;
		}
		if ((values[0] == bot_left && values[1] == top_left) || (values[1] == bot_left && values[0] == top_left))
		{
			return 3;
		}
		return -1;
	}

	Rect getValidationPosition(Mat image)
	{
		cv::Mat grey;
		cv::cvtColor(image, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);

		cv::Mat thresh;

		double ret = cv::threshold(grey, thresh, 0, 255, THRESH_OTSU | THRESH_BINARY_INV);

		vector<vector<Point>> cnts;
		vector<Vec4i> hierarchy;
		cv::findContours(thresh, cnts, hierarchy, RETR_LIST, CHAIN_APPROX_NONE);
		vector<Rect> valid_text_areas;
		for (const auto& c : cnts)
		{
			double area = cv::contourArea(c);
			if (area > 500 || area < 50)
				continue;
			Rect r = cv::boundingRect(c);
			valid_text_areas.push_back(r);
			//cv::rectangle(image, r, Scalar(0, 255, 255), 1);
			break;
		}
		assert(valid_text_areas.size() == 1);
		return valid_text_areas[0];
	}

}

DofusHuntAnalyzer::DofusHuntAnalyzer(cv::Mat& image, tesseract::TessBaseAPI* tesseract_api) : mImage(image)
{
	//mImageDebug = mImage.clone();
	mTesseractAPI = tesseract_api;
}

bool DofusHuntAnalyzer::interfaceFound()
{
	if (mInterfaceRect.empty())
	{
		findInterface();
	}

	return !mInterfaceRect.empty();
	
}

std::pair<int, int> DofusHuntAnalyzer::getStartPosition()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mStartPosition;
}

std::pair<int, int> DofusHuntAnalyzer::getCurrentPosition()
{
	if (!mCurrentPositionFound)
	{
		findCurrentPos();
	}
	return mCurrentPosition;
}

std::string DofusHuntAnalyzer::getLastHint()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mLastHint;
}

int DofusHuntAnalyzer::getLastHintDirection()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mLastHintDirection;
}

bool DofusHuntAnalyzer::isHuntFinished()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mCurrentStep == mMaxStep;
}

int DofusHuntAnalyzer::getLastHintIndex()
{
	return mLastHintIndex;
}

bool DofusHuntAnalyzer::isHintChecked(cv::Rect rect)
{
	cv::Mat validation(mImage, rect);
	cv::threshold(validation, validation, 250, 255, cv::THRESH_BINARY);
	cv::cvtColor(validation, validation, cv::ColorConversionCodes::COLOR_BGR2GRAY);
	return (cv::countNonZero(validation) == 0);
}

int DofusHuntAnalyzer::getHuntStep()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mCurrentStep;
}

int DofusHuntAnalyzer::getMaxHuntStep()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mMaxStep;
}

bool DofusHuntAnalyzer::isStepFinished()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return (mLastHintIndex == mNbHint) && isHintChecked(mLastHintValidationPosition);
}

Rect DofusHuntAnalyzer::getLastHintValidationPosition()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mLastHintValidationPosition;
}

Rect DofusHuntAnalyzer::getStepValidationPosition()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mValidateRect;
}

bool DofusHuntAnalyzer::canValidate()
{
	return !mValidateRect.empty();
}

bool DofusHuntAnalyzer::isPhorreurFound()
{
	if (!mHuntAreaFound)
	{
		findHuntArea();
	}
	return false;
}

inline void DofusHuntAnalyzer::resetTesseractAPI(tesseract::PageSegMode mode, const std::string& whitelist)
{
	mTesseractAPI->SetPageSegMode(mode);
	mTesseractAPI->SetVariable("tessedit_char_whitelist", whitelist.c_str());
}

inline string DofusHuntAnalyzer::getPreciseTextFromImage(Mat& image, bool block)
{
	Mat rgb;
	cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

	resetTesseractAPI(tesseract::PSM_SINGLE_CHAR, "?");
	mTesseractAPI->SetImage(rgb.data, rgb.cols, rgb.rows, 3, (int)rgb.step);

	string res = mTesseractAPI->GetUTF8Text();
	if (block)
	{
		resetTesseractAPI(tesseract::PSM_SINGLE_BLOCK, TESSERACT_FILTER);
	}
	else
	{
		resetTesseractAPI(tesseract::PSM_SPARSE_TEXT, TESSERACT_FILTER);
	}
	if (res == "?")
	{
		return res;
	}

	mTesseractAPI->SetImage(rgb.data, rgb.cols, rgb.rows, 3, (int)rgb.step);

	res = mTesseractAPI->GetUTF8Text();
	resetTesseractAPI(tesseract::PSM_SPARSE_TEXT, TESSERACT_FILTER);

	return res;
}

std::string DofusHuntAnalyzer::toString() const
{
	std::stringstream ss;
	ss << "DofusHuntAnalyzer: \n";
	ss << "mCurrentPositionFound: " << mCurrentPositionFound << "\n";
	ss << "mHuntInfosFound: " << mHuntInfosFound << "\n";
	ss << "mHuntAreaFound: " << mHuntAreaFound << "\n";
	ss << "mInterfaceRect: " << mInterfaceRect << "\n";
	ss << "mLastHintDirection: " << mLastHintDirection << "\n";
	ss << "mLastHintIndex: " << mLastHintIndex << "\n";
	ss << "mNbHint: " << mNbHint << "\n";
	ss << "mLastHintValidationPosition: " << mLastHintValidationPosition << "\n";
	ss << "mHuntArea: " << mHuntArea << "\n";
	ss << "mCurrentStep: " << mCurrentStep << "\n";
	ss << "mMaxStep: " << mMaxStep << "\n";
	ss << "mStartPosition: (" << mStartPosition.first << ", " << mStartPosition.second << ")\n";
	ss << "mCurrentPosition: (" << mCurrentPosition.first << ", " << mCurrentPosition.second << ")\n";
	return ss.str();
}

inline DofusHuntAnalyzer DofusHuntAnalyzer::fromString(const std::string& str)
{
	std::istringstream iss(str);
	// Create empty analyzer
	DofusHuntAnalyzer analyzer(cv::Mat(), nullptr);

	std::string dummy;
	iss >> dummy >> dummy; // Skip "DofusHuntAnalyzer:"

	iss >> dummy >> analyzer.mCurrentPositionFound;
	iss >> dummy >> analyzer.mHuntInfosFound;
	iss >> dummy >> analyzer.mHuntAreaFound;

	// Parsing cv::Rect (x, y, width, height)
	int x, y, width, height;
	iss >> dummy >> x >> y >> width >> height;
	analyzer.mInterfaceRect = cv::Rect(x, y, width, height);

	iss >> dummy >> analyzer.mLastHintDirection;
	iss >> dummy >> analyzer.mLastHintIndex;
	iss >> dummy >> analyzer.mNbHint;

	// Parsing cv::Rect (x, y, width, height)
	iss >> dummy >> x >> y >> width >> height;
	analyzer.mLastHintValidationPosition = cv::Rect(x, y, width, height);

	// Parsing cv::Rect (x, y, width, height)
	iss >> dummy >> x >> y >> width >> height;
	analyzer.mHuntArea = cv::Rect(x, y, width, height);

	iss >> dummy >> analyzer.mCurrentStep;
	iss >> dummy >> analyzer.mMaxStep;

	// Parsing std::pair<int, int>
	int first, second;
	iss >> dummy >> dummy >> first >> dummy >> second;
	analyzer.mStartPosition = std::make_pair(first, second);

	iss >> dummy >> dummy >> first >> dummy >> second;
	analyzer.mCurrentPosition = std::make_pair(first, second);

	return analyzer;
}

void DofusHuntAnalyzer::initHuntInfos()
{
	if (!interfaceFound()) return;
	Mat _interface(mImage, mInterfaceRect);
	vector<Rect> rects = FindRectInImage(_interface, HINT_LOWER, HINT_UPPER, "", mInterfaceRect.width - 10);
	//for (auto r : rects)
	//{
	//	rectangle(_interface, r, Scalar(0, 255, 0));
	//}
	// order by y axis, meaning the order will be from top rect to bottom
	std::sort(rects.begin(), rects.end(), [](Rect p1, Rect p2) { return p1.y < p2.y; });
	// get step infos
	Rect step_rect = rects[0];
	Mat step(_interface, step_rect);

	std::string step_s = getPreciseTextFromImage(step);
	int index = (int)step_s.find("/");
	mCurrentStep = std::stoi(step_s.substr(index - 1, index));
	mMaxStep = std::stoi(step_s.substr(index + 1, index + 2));

	// get start position infos
	Rect start_rect = rects[1];
	Mat start(_interface, start_rect);
	Rect sub_start_r = Rect(1, 1, start.cols - 1, (int)(start.rows / 2) - 1);
	Mat sub_start(start, sub_start_r);
	std::string start_s = getPreciseTextFromImage(start);
	int start_index = (int)start_s.find("[");
	int end_index = (int)start_s.find("]");
	string pos = start_s.substr(start_index + 1, end_index - start_index - 1);

	int hint_index = (int)pos.find(",");
	string pos_x = pos.substr(0, hint_index);
	string pos_y = pos.substr(hint_index + 1, pos.size() - hint_index);
	mStartPosition = make_pair(std::stoi(pos_x), std::stoi(pos_y));
	mNbHint = (int)rects.size() - 3;
	// get hint infos
	for (int i = (int)rects.size() - 1; i > 1; i--)
	{
		Rect _rect = rects[i];
		int offset_start = (int)(5 + _rect.width * X_DIRECTION_RATIO);
		int offset_end = (int)(5 + _rect.width * (X_DIRECTION_RATIO + X_INPROGRESS_RATIO));

		Rect hint_rect(offset_start, _rect.y, _rect.width - offset_end - offset_start, _rect.height);
		Mat hint(_interface, hint_rect);
		// Process hints in reverse
		std::string hint_s = getPreciseTextFromImage(hint);
		if (hint_s != "?" && hint_s.size() > 2)
		{
			std::replace(hint_s.begin(), hint_s.end(), '\n', ' ');
			mLastHint = hint_s;

			Rect validation_rect(_rect.width - offset_start, _rect.y, offset_start, _rect.height);
			Mat validation_pos(_interface, validation_rect);
			Rect last_hint_validation_position = getValidationPosition(validation_pos);
			last_hint_validation_position.x += validation_rect.x + mInterfaceRect.x;
			last_hint_validation_position.y += validation_rect.y + mInterfaceRect.y;
			mLastHintValidationPosition = last_hint_validation_position;

			Rect direction_rect(_rect.x, _rect.y, offset_start, _rect.height);
			Mat direction(_interface, direction_rect);
			mLastHintDirection = getDirection(direction);
			mLastHintIndex = i - 2;
			break;
		}

	}
	mHuntInfosFound = true;
	if (!isStepFinished())
		return;
	vector<Rect> validate_rects = FindRectInImage(_interface, VALIDATE_LOWER, VALIDATE_UPPER, "VALIDER");
	if(validate_rects.size() > 0)
	{
		mValidateRect = validate_rects[0];
		mValidateRect.x += mInterfaceRect.x;
		mValidateRect.y += mInterfaceRect.y;
	}
	
}

void DofusHuntAnalyzer::findInterface()
{
	vector<Rect> rects = FindRectInImage(mImage, INTERFACE_LOWER, INTERFACE_UPPER, "", 100, 100);
	for (const Rect& r : rects)
	{
		float ratio = (float)(r.height) / r.width;

		if (ratio < .5 || ratio > 3)
		{
			continue;
		}
		Rect sub_rect(r.x, r.y, r.width, (int)(r.height * .1));
		Mat sub(mImage, sub_rect);
		Mat sub_test;
		cv::cvtColor(sub, sub_test, cv::COLOR_BGR2GRAY);
		cv::threshold(sub_test, sub_test, 230, 255, cv::THRESH_BINARY);
		if (cv::countNonZero(sub_test) < 100)
		{
			continue;
		}
		std::string t1 = getPreciseTextFromImage(sub);
		if(t1.find(TITLE_S) != std::string::npos)
		{
			mInterfaceRect = r;
			break;
		}
		else if (t1.find("CHASSE AUX TRÉSORS") != std::string::npos)
		{
			mInterfaceRect = r;
			break;
		}
	}
}

void DofusHuntAnalyzer::findHuntArea()
{
	Mat rang;
	cv::inRange(mImage, Scalar(0, 0, 0), Scalar(5, 5, 5), rang);

	cv::blur(rang, rang, Point(1, 1));
	auto kernel = cv::getStructuringElement(cv::MORPH_RECT, Point(5, 5));
	cv::dilate(rang, rang, kernel, Point(-1, -1), 5);

	Mat hsv;
	cv::cvtColor(mImage, hsv, COLOR_BGR2HSV);

	cv::inRange(hsv, INTERFACE_LOWER, INTERFACE_UPPER, hsv);
	cv::blur(hsv, hsv, Point(2, 2));
	auto kernel2 = cv::getStructuringElement(cv::MORPH_RECT, Point(3, 3));
	cv::dilate(hsv, hsv, kernel2, Point(-1, -1), 4);

	Mat result;
	cv::bitwise_or(rang, hsv, result);
	cv::bitwise_not(result, result);


	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cv::findContours(result, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
	vector<vector<Point>> final_contours;

	for (const vector<Point>& contour : contours)
	{
		double area = cv::contourArea(contour);
		if (area < 10000)
			continue;
		Rect r = cv::boundingRect(contour);
		float ratio = (float)(r.width / r.height);
		if (ratio < .7 || ratio > 1.3)
		{
			continue;
		}
		final_contours.push_back(contour);
	}
	Rect r = cv::boundingRect(final_contours[0]);
	Rect inner_rect = findInnerRect(final_contours[0], Point(mImage.cols / 2, mImage.rows / 2));
	//cv::rectangle(mImageDebug, inner_rect, Scalar(100, 255, 255));
	mHuntArea = inner_rect;
	mHuntAreaFound = true;
}

void DofusHuntAnalyzer::findCurrentPos()
{
	cv::Mat hsv;
	cv::cvtColor(mImage, hsv, cv::ColorConversionCodes::COLOR_BGR2HSV);

	cv::Mat mask;
	cv::inRange(hsv, CURRENT_POS_LOWER, CURRENT_POS_UPPER, mask);

	cv::Mat res;
	cv::bitwise_and(mImage, mImage, res, mask = mask);

	cv::Mat grey;
	cv::cvtColor(res, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);

	cv::Mat thresh;
	double ret = cv::threshold(grey, thresh, 230, 255, 0);

	Mat blur;
	cv::blur(thresh, blur, Size(3, 3));
	// Dilate to combine adjacent text contours
	Mat kernel = cv::getStructuringElement(MORPH_RECT, Size(7, 5));
	Mat dilate;
	cv::dilate(blur, dilate, kernel, Point(-1, -1), 4);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cv::findContours(dilate, contours, hierarchy, 1, 2);

	std::pair<double, cv::Rect> top_left_d = { 99999, Rect() };
	for (const vector<Point> contour : contours)
	{
		double area = cv::contourArea(contour);
		if (area < 1000) continue;

		Rect r = cv::boundingRect(contour);

		if (r.width < 10 || r.height < 10) continue;
		if (r.width > 1000 || r.height > 1000) continue;

		//rectangle(mDebugImage, r, Scalar(0, 0, 244));
		float ratio = (float)(r.height) / r.width;

		if (ratio < .1 || ratio > 10) continue;

		double dist = std::pow((double)std::abs(0 - r.x), (double)2) + std::pow((double)std::abs(0 - r.y), (double)2);
		if (dist < top_left_d.first)
		{
			top_left_d.first = dist;
			top_left_d.second = r;
		}
	}
	
	auto top_left = top_left_d.second;
	//Rect sub_rect(top_left.x, top_left.y + (int)(top_left.height / 2), (int)(top_left.width / 2) - 1, (int)(top_left.height / 2) - 2);
	Mat sub_mat(mImage, top_left);

	cv::Mat sub_hsv;
	cv::cvtColor(sub_mat, sub_hsv, cv::ColorConversionCodes::COLOR_BGR2HSV);

	cv::Mat sub_mask;
	cv::inRange(sub_hsv, Scalar(0,0,0), Scalar(179, 20, 255), sub_mask);

	cv::Mat sub_res;
	cv::bitwise_and(sub_mat, sub_mat, sub_res, mask = sub_mask);
	string whiteList = "0123456789 -,";
	mTesseractAPI->SetVariable("tessedit_char_whitelist", whiteList.c_str());
	string top_left_s = getPreciseTextFromImage(sub_res);

	std::string target = "O";
	std::string replacement = "0";
	size_t pos = top_left_s.find(target);
	if (pos != std::string::npos) {
		top_left_s.replace(pos, target.length(), replacement);
	}

	// Check if the target substring is found
	const std::regex re("^([-]?[0-9Oo]{1,2}).[, ]{1,5}([-]?[0-9Oo]{1,2})");
	std::smatch match_result;

	string::const_iterator searchStart(top_left_s.cbegin());

	std::regex_search(searchStart, top_left_s.cend(), match_result, re);
	if (match_result.empty())
	{
		top_left_s = getPreciseTextFromImage(sub_res, true);
		std::string target = "O";
		std::string replacement = "0";
		size_t pos = top_left_s.find(target);
		if (pos != std::string::npos) {
			top_left_s.replace(pos, target.length(), replacement);
		}
		searchStart = string::const_iterator(top_left_s.cbegin());
		std::regex_search(searchStart, top_left_s.cend(), match_result, re);
	}

	
	int x_pos = std::stoi(match_result[1]);
	int y_pos = std::stoi(match_result[2]);

	//mTesseractAPI->SetVariable("tessedit_char_whitelist", "");
	//cv::rectangle(mImageDebug, sub_rect, Scalar(100, 100, 255), 1);
	//int ind = (int)top_left_s.find("\n");
	//string sub_pos = top_left_s.substr(ind);

	
	//int y_pos = std::stoi(words[1]);
	mCurrentPosition = { x_pos, y_pos };
	mCurrentPositionFound = true;
}

vector<Rect> DofusHuntAnalyzer::FindRectInImage(Mat& image, Scalar lower_bound, Scalar upper_bound, string text_content, int minimum_width, int minimum_height)
{
	vector<Rect> valid_contours;
	if (image.empty())
	{
		return valid_contours;
	}


	vector<vector<Point>> to_draw;
	cv::Mat hsv;
	cv::cvtColor(image, hsv, cv::ColorConversionCodes::COLOR_BGR2HSV);

	cv::Mat mask;
	cv::inRange(hsv, lower_bound, upper_bound, mask);

	cv::Mat res;
	cv::bitwise_and(image, image, res, mask = mask);

	cv::Mat grey;
	cv::cvtColor(res, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);

	cv::Mat thresh;
	double ret = cv::threshold(grey, thresh, 10, 255, 0);
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	cv::findContours(thresh, contours, hierarchy, 1, 2);
	//drawContours(mImage, contours, -1, Scalar(1, 1, 1), 2);
	for (const vector<Point> contour : contours)
	{

		auto [x1, y1] = contour[0];
		vector<Point> approx;
		//cv::approxPolyDP(contour, approx, .01 * cv::arcLength(contour, true), true);

		//if (approx.size() != 4)
		//{
		//	continue;
		//}
		Rect bounds = cv::boundingRect(contour);

		if ((bounds.width < minimum_width || bounds.height < minimum_height))
			continue;
		if (!text_content.empty() && !containsText(Mat(image, bounds), text_content))
		{
			continue;
		}

		valid_contours.push_back(bounds);
		to_draw.push_back(contour);
	}
	//cv::drawContours(image, to_draw, -1, Scalar(0, 255, 0), 1);
	return valid_contours;
}

bool DofusHuntAnalyzer::containsText(Mat& image, string text)
{
	std::string t1 = getPreciseTextFromImage(image);
	return t1.find(text) != std::string::npos;
}


