#include "Preset.hpp"
#include <filesystem>
#include <util/dstr.h>
#include <obs-module.h>
#include <opencv2/imgcodecs.hpp>

inline std::vector<cv::Rect2i> rects_from_data(obs_data_array_t *arr)
{
	auto ret = std::vector<cv::Rect2i>{};
	if (!arr)
		return ret;

	for (size_t i = 0; i < MIN(obs_data_array_count(arr), 10); i++) {
		OBSDataAutoRelease data = obs_data_array_item(arr, i);
		ret.push_back(cv::Rect2i((int)obs_data_get_int(data, "x"), (int)obs_data_get_int(data, "y"),
					 (int)obs_data_get_int(data, "width"), (int)obs_data_get_int(data, "height")));
	}
	return ret;
}

bool Preset::load(std::string _name, OBSDataAutoRelease &p)
{
	name = _name;
	OBSDataAutoRelease nativeRes = obs_data_get_obj(p, "native_resolution");
	OBSDataArrayAutoRelease p1Arr = obs_data_get_array(p, "p1Score"), p2Arr = obs_data_get_array(p, "p2Score");
	if (!nativeRes || !p1Arr)
		return false;

	native_resolution =
		cv::Size2i((int)obs_data_get_int(nativeRes, "width"), (int)obs_data_get_int(nativeRes, "height"));

	p1ScoreArea = rects_from_data(p1Arr);
	p2ScoreArea = rects_from_data(p2Arr);

	two_player = p1Arr && p2Arr;

	binarization_mode = {};
	binarization_threshold = {};

	mse_threshold = (float)obs_data_get_double(p, "mse_threshold");

	if (obs_data_has_user_value(p, "binarization_mode"))
		binarization_mode = (int)obs_data_get_int(p, "binarization_mode");
	if (obs_data_has_user_value(p, "binarization_threshold"))
		binarization_threshold = (int)obs_data_get_int(p, "binarization_threshold");

	for (int i = 0; i < 10; i++) {
		dstr path = {0};
		dstr_printf(&path, "%s/%d.png", name.c_str(), i);
		char *file = obs_module_config_path(path.array);

		if (!std::filesystem::exists(file))
			return false;

		cv::Mat mat = cv::imread(file, cv::IMREAD_COLOR);

		cv::Mat mask = mat.clone();
		cv::cvtColor(mask, mask, cv::COLOR_BGR2GRAY);
		cv::threshold(mask, masks[i], 1, 255, cv::THRESH_BINARY);

		if (binarization_threshold) {
			cv::cvtColor(mat, mat, cv::COLOR_BGR2GRAY);
			cv::threshold(mat, digits[i], binarization_threshold.value(), 255,
				      binarization_mode.value_or(cv::THRESH_BINARY));
		} else {
			digits[i] = mat;
		}
		bfree(file);
		dstr_free(&path);
	}
	return true;
}
void Preset::analyzeImageMsd(cv::InputArray img, std::optional<int> &p1, std::optional<int> &p2)
{
	cv::Mat mat(native_resolution, img.type());
	cv::resize(img, mat, mat.size());

	if (p1ScoreArea.size() > 0) {
		p1 = analyzeImageMsd(mat, p1ScoreArea);
	}
	if (p2ScoreArea.size() > 0) {
		p2 = analyzeImageMsd(mat, p2ScoreArea);
	}
}

std::optional<int> Preset::analyzeImageMsd(const cv::Mat &img, std::vector<cv::Rect2i> &scoreArea)
{
	std::optional<int> result = {};
	for (auto &numArea : scoreArea) {
		cv::Mat subArea = img(numArea);
		if (binarization_threshold) {
			cv::cvtColor(subArea, subArea, cv::COLOR_BGR2GRAY);
			cv::threshold(subArea, subArea, binarization_threshold.value(), 255,
				      binarization_mode.value_or(0));
		} else {
			cv::cvtColor(subArea, subArea, cv::COLOR_BGRA2BGR);
		}

		std::optional<int> min = {};
		double minValue = mse_threshold;
		for (int i = 0; i < 10; i++) {
			double msd = cv::norm(subArea, digits[i], cv::NORM_L2, masks[i]) / 256;
			msd = msd * msd / cv::countNonZero(masks[i]);
			if (msd <= minValue) {
				minValue = msd;
				min = i;
			}
		}
		bool last = numArea == scoreArea[scoreArea.size() - 1];
		if ((!min && result) || (min == 0 && !last && !result)) {
			return {};
		} else if (min) {
			result = result.value_or(0) * 10 + min.value();
		}
	}
	return result;
}
