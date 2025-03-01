#pragma once
#include <optional>
#include <opencv2/imgproc.hpp>
#include <obs.hpp>

class Preset {
public:
	bool load(std::string _name, OBSDataAutoRelease &presetData);
	std::string name;
	void analyzeImageMsd(cv::InputArray img, std::optional<int> &p1, std::optional<int> &p2);
	bool isTwoPlayer() { return two_player; }

private:
	std::optional<int> analyzeImageMsd(const cv::Mat &img, std::vector<cv::Rect2i> &scoreArea);

	cv::Size2i native_resolution;
	std::vector<cv::Rect2i> p1ScoreArea;
	std::vector<cv::Rect2i> p2ScoreArea;
	bool two_player;
	cv::Mat digits[10];
	cv::Mat masks[10];

	std::optional<int> binarization_mode;
	std::optional<int> binarization_threshold;
	float mse_threshold;
};
