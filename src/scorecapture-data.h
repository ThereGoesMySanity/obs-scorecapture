#pragma once
#include <opencv2/imgproc.hpp>
#include <obs.h>
#include <optional>

struct preset {
    std::string name;

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

struct filter_data {
	obs_source_t *source;
	std::string unique_id;
	gs_texrender_t *texrender;
	gs_stagesurf_t *stagesurface;
	gs_effect_t *effect;

	obs_data_t *presets = nullptr;
	struct preset *preset = nullptr;

	std::string mode;

	std::optional<int> p1Score, p2Score;

	int clear_delay;
	int clear_timer;

	bool isDisabled;

	// Text source to output the text to
	obs_weak_source_t *output_source = nullptr;
	char *output_source_name = nullptr;
};

struct source_data {
	obs_source_t *source;
	struct vec2 size;
	gs_texrender_t *texrender;

};