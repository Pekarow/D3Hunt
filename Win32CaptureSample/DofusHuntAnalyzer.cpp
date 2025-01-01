#include "DofusHuntAnalyzer.h"
#include <string>
#include <ranges>
#include <cctype>
#include <math.h>
using namespace std;
using namespace cv;

namespace
{
	static tesseract::TessBaseAPI* gTesseractAPI = nullptr;
	const Scalar INTERFACE_LOWER = Scalar(103, 0, 41);
	const Scalar INTERFACE_UPPER = Scalar(146, 255, 255);

	const Scalar TITLE_LOWER = Scalar(98, 60, 83);
	const Scalar TITLE_UPPER = Scalar(180, 255, 190);
	const Scalar HINT_LOWER = Scalar(106, 0, 0);
	const Scalar HINT_UPPER = Scalar(180, 255, 71);
	const string TITLE_S = "CHASSE AUX TRESORS";

	const double X_DIRECTION_RATIO = .1;
	const double X_INPROGRESS_RATIO = .25;

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
	string getTextFromImage(Mat& image, tesseract::TessBaseAPI* tess, bool remove_endl = true)
	{
		cv::Mat grey;
		cv::cvtColor(image, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);


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
		vector<Rect> valid_text_areas;
		for (const auto& c : cnts)
		{
			double area = cv::contourArea(c);
			if (area < 1000)
				continue;
			Rect r = cv::boundingRect(c);
			valid_text_areas.push_back(r);
			//cv::rectangle(image, r, Scalar(255, 0, 255), 1);
		}

		std::string output;
		for (const auto& text_area : valid_text_areas)
		{
			Mat sub_area(grey, text_area);
			cv::Mat thresh;
			double ret = cv::threshold(sub_area, thresh, 0, 255, THRESH_OTSU | THRESH_BINARY_INV);
			// recognize text
			tess->TesseractRect(thresh.data, 1, (int)thresh.step1(), 0, 0, thresh.cols, thresh.rows);
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

DofusHuntAnalyzer::DofusHuntAnalyzer(cv::Mat& image) : mImage(image)
{
	mImageDebug = mImage.clone();
	if (!gTesseractAPI)
	{
		gTesseractAPI = new tesseract::TessBaseAPI();
		printf("Tesseract-ocr version: %s\n",
			gTesseractAPI->Version());
		// Initialize tesseract-ocr with English, without specifying tessdata path
		if (gTesseractAPI->Init(NULL, "eng")) {
			fprintf(stderr, "Could not initialize tesseract.\n");
			assert(false);
			return;
		}
		string whiteList = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefgijklmnopqrstuvwxyz Ééèêçà'ô[]()?/,-";
		gTesseractAPI->SetVariable("tessedit_char_whitelist", whiteList.c_str());
	}
}

bool DofusHuntAnalyzer::interfaceFound()
{
	findInterface();
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

Rect DofusHuntAnalyzer::getLastHintValidationPosition()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return mLastHintValidationPosition;
}

std::pair<int, int> DofusHuntAnalyzer::getStepValidationPosition()
{
	if (!mHuntInfosFound)
	{
		initHuntInfos();
	}
	return std::pair<int, int>();
}

bool DofusHuntAnalyzer::isPhorreurFound()
{
	if (!mHuntAreaFound)
	{
		findHuntArea();
	}
	return false;
}

void DofusHuntAnalyzer::initHuntInfos()
{
	if(!interfaceFound()) return;
	Mat _interface(mImage, mInterfaceRect);
	vector<Rect> rects = FindRectInImage(_interface, HINT_LOWER, HINT_UPPER, "", mInterfaceRect.width - 10);
	for (auto r : rects)
	{
		rectangle(_interface, r, Scalar(0, 255, 0));
	}
	// order by y axis, meaning the order will be from top rect to bottom
	std::sort(rects.begin(), rects.end(), [](Rect p1, Rect p2) { return p1.y < p2.y; });
	// get step infos
	Rect step_rect = rects[0];
	Mat step(_interface, step_rect);

	std::string step_s = getTextFromImage(step, gTesseractAPI);
	int index = (int)step_s.find("/");
	mCurrentStep = std::stoi(step_s.substr(index - 1, index));
	mMaxStep = std::stoi(step_s.substr(index + 1, index + 2));

	// get start position infos
	Rect start_rect = rects[1];
	Mat start(_interface, start_rect);
	std::string start_s = getTextFromImage(start, gTesseractAPI);
	int start_index = (int)start_s.find("[");
	int end_index = (int)start_s.find("]");
	string pos = start_s.substr(start_index + 1, end_index - start_index - 1);

	int hint_index = (int)pos.find(",");
	string pos_x = pos.substr(0, hint_index);
	string pos_y = pos.substr(hint_index + 1, pos.size() - hint_index);
	mStartPosition = make_pair(std::stoi(pos_x), std::stoi(pos_y));

	// get hint infos
	for (int i = (int)rects.size() - 1; i > 1; i--)
	{
		Rect _rect = rects[i];
		int offset_start = (int)(5 + _rect.width * X_DIRECTION_RATIO);
		int offset_end = (int)(5 + _rect.width * (X_DIRECTION_RATIO + X_INPROGRESS_RATIO));

		Rect hint_rect(offset_start, _rect.y, _rect.width - offset_end - offset_start, _rect.height);
		Mat hint(_interface, hint_rect);
		// Process hints in reverse
		std::string hint_s = getTextFromImage(hint, gTesseractAPI);
		if (hint_s.size() > 2)
		{
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
}

void DofusHuntAnalyzer::findInterface()
{
	vector<Rect> rects = FindRectInImage(mImage, INTERFACE_LOWER, INTERFACE_UPPER, "", 100, 100);
	for (const Rect& r : rects)
	{
		float ratio = (float)(r.height) / r.width;

		if (ratio < .7 || ratio > 3)
		{
			continue;
		}
		//rectangle(mImage, r, Scalar(0, 255, 0), 1);
		Mat sub(mImage, r);
		vector<Rect> founds = FindRectInImage(sub, TITLE_LOWER, TITLE_UPPER, TITLE_S);
		Rect title_rect;
		for (const auto& found : founds)
		{
			if (found.y < 10 && found.x < 10)
			{
				mInterfaceRect = r;
				rectangle(mImageDebug, mInterfaceRect, Scalar(0, 255, 0), 1);
				return;
			}
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
	cv::rectangle(mImageDebug, inner_rect, Scalar(100, 255, 255));
	mHuntArea = inner_rect;
	mHuntAreaFound = true;
}

void DofusHuntAnalyzer::findCurrentPos()
{
	cv::Mat grey;
	cv::cvtColor(mImageDebug, grey, cv::ColorConversionCodes::COLOR_BGR2GRAY);


	Mat adaptive_thresh;
	cv::threshold(grey, adaptive_thresh, 250, 255, 0);

	Mat blur;
	cv::blur(grey, blur, Size(9, 9));
	// Dilate to combine adjacent text contours
	Mat kernel = cv::getStructuringElement(MORPH_RECT, Size(5, 5));
	Mat dilate;
	cv::dilate(adaptive_thresh, dilate, kernel, Point(-1, -1), 4);

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	cv::findContours(dilate, contours, hierarchy, 1, 2);

	Rect top_left(99999, 99999, 99999, 99999);
	for (const vector<Point> contour : contours)
	{
		double area = cv::contourArea(contour);
		if (area < 100)
			continue;
		Rect r = cv::boundingRect(contour);
		float ratio = (float)(r.height) / r.width;

		if (ratio < .1 || ratio > 10)
		{
			continue;
		}
		if (top_left.x > r.x && top_left.y > r.y)
		{
			top_left = r;
		}
	}

	Mat sub_mat(mImage, top_left);
	string top_left_s = getTextFromImage(sub_mat, gTesseractAPI, false);
	cv::rectangle(mImageDebug, top_left, Scalar(100, 100, 255), 1);
	int ind = (int)top_left_s.find("\n");
	string sub_pos = top_left_s.substr(ind);
	trim(sub_pos);
	int space = (int)sub_pos.find(" - ");
	string pos = sub_pos.substr(0, space);
	int coma = (int)pos.find(',');
	int x_pos = std::stoi(pos.substr(0, coma));
	int y_pos = std::stoi(pos.substr(coma + 1));
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

		if ((bounds.width < minimum_width || bounds.height < minimum_height) )
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
	std::string t1 = getTextFromImage(image, gTesseractAPI);
	return t1.find(text) != std::string::npos;
}


