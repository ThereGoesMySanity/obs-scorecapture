#include "scorecapture-filter-presets.h"
#include <obs-module.h>
#include <fstream>
#include <filesystem>
#include <util/dstr.h>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <obs.hpp>

std::vector<cv::Rect2i> rects_from_data(obs_data_array_t *arr){
    auto ret = std::vector<cv::Rect2i>{};
    if (!arr) return ret;

    for (size_t i = 0; i < MIN(obs_data_array_count(arr), 10); i++) {
	    OBSDataAutoRelease data = obs_data_array_item(arr, i);
	    ret.push_back(cv::Rect2i(
            obs_data_get_int(data, "x"),
            obs_data_get_int(data, "y"),
            obs_data_get_int(data, "width"),
            obs_data_get_int(data, "height")));
    }
    return ret;
}

void filter_data_set_preset(filter_data *tf, obs_data_t *settings) {
	std::string new_preset = obs_data_get_string(settings, "preset");
	if (!tf->preset || new_preset != tf->preset->name) {
		if (!tf->presets) {
			tf->presets = load_presets();
		}
		// Load new preset
		tf->preset = load_preset(tf, new_preset.c_str());
	}
}

obs_data_t* load_presets() {
	char* presets_file = obs_module_config_path("presets.json");
    obs_data_t *presets = obs_data_create_from_json_file(presets_file);
    bfree(presets_file);
    return presets;
}

preset* load_preset(filter_data *tf, const char *name) {
    OBSDataAutoRelease p = obs_data_get_obj(tf->presets, name);
    if (p != nullptr) {
        auto preset = new struct preset();
        OBSDataAutoRelease nativeRes = obs_data_get_obj(p, "native_resolution");
        OBSDataArrayAutoRelease p1Arr = obs_data_get_array(p, "p1Score"), p2Arr = obs_data_get_array(p, "p2Score");
        *preset = (struct preset) {
            .name = name,
            .native_resolution = cv::Size2i(obs_data_get_int(nativeRes, "width"), obs_data_get_int(nativeRes, "height")),

            .p1ScoreArea = rects_from_data(p1Arr),
            .p2ScoreArea = rects_from_data(p2Arr),

            .two_player = p1Arr && p2Arr,

            .binarization_mode = {},
            .binarization_threshold = {},

            .mse_threshold = (float)obs_data_get_double(p, "mse_threshold")
        };

        if (obs_data_has_user_value(p, "binarization_mode"))
            preset->binarization_mode = (int)obs_data_get_int(p, "binarization_mode");
        if (obs_data_has_user_value(p, "binarization_threshold"))
            preset->binarization_threshold = (int)obs_data_get_int(p, "binarization_threshold");

        for (int i = 0; i < 10; i++) {
            dstr path = {0};
            dstr_printf(&path, "%s/%d.png", name, i);
            char* file = obs_module_config_path(path.array);
            if (!std::filesystem::exists(file)) return nullptr;
            cv::Mat mat = cv::imread(file, cv::IMREAD_COLOR);

            cv::Mat mask = mat.clone();
            cv::cvtColor(mask, mask, cv::COLOR_BGR2GRAY);
            cv::threshold(mask, preset->masks[i], 1, 255, cv::THRESH_BINARY);

            if (preset->binarization_threshold) {
                cv::cvtColor(mat, mat, cv::COLOR_BGR2GRAY);
                cv::threshold(mat, preset->digits[i], *preset->binarization_threshold, 255,
                    preset->binarization_mode.value_or(cv::THRESH_BINARY));
            } else {
                preset->digits[i] = mat;
            }
            bfree(file);
            dstr_free(&path);
        }

        return preset;
    }
    return nullptr;
}