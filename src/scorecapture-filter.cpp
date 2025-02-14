#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <numeric>
#include <memory>
#include <exception>
#include <fstream>
#include <new>
#include <mutex>
#include <filesystem>

#include <util/bmem.h>

#include <obs-websocket-api.h>

#include "plugin-support.h"
#include "scorecapture-data.h"
#include "obs-utils.h"
#include "scorecapture-filter.h"
#include "scorecapture-filter-callbacks.h"
#include "scorecapture-filter-presets.h"
#include <find-font.h>

extern obs_websocket_vendor *vendor;

bool get_update_text(filter_data *tf, std::string& out_text) {
	std::optional<std::string> update_text = {};
	bool autodetect = tf->mode == "AutoDetect";
	if (tf->p1Score && tf->p2Score && (autodetect || tf->mode == "Versus")) {
		int diff = tf->p1Score.value() - tf->p2Score.value();
		if (diff > 0) {
			update_text = "P1\n" + std::to_string(diff);
		} else if (diff < 0) {
			update_text = "P2\n" + std::to_string(-diff);
		} else {
			update_text = "Tie";
		}
	} else if (tf->p1Score && (autodetect || tf->mode == "P1")) {
		update_text = std::to_string(tf->p1Score.value());
	} else if (tf->p2Score && (autodetect || tf->mode == "P2")) {
		update_text = std::to_string(tf->p2Score.value());
	}
	if (update_text) {
		out_text = update_text.value();
		return true;
	}
	return false;
}
bool update_text_source(filter_data *tf, std::string update_text) {
	obs_weak_source_t *text_source = tf->output_source;
	if (!text_source) {
		obs_log(LOG_ERROR, "text_source is null");
		return false;
	}
	auto target = obs_weak_source_get_source(text_source);
	if (!target) {
		obs_log(LOG_ERROR, "text_source target is null");
		return false;
	}
	auto text_settings = obs_source_get_settings(target);
	obs_data_set_string(text_settings, "text", update_text.c_str());
	obs_source_update(target, text_settings);
	obs_data_release(text_settings);
	obs_source_release(target);
	return true;
}


std::optional<int> analyze_img_msd(preset *preset, cv::Mat img, std::vector<cv::Rect2i> scoreArea) {
	std::optional<int> result = {};
	for(auto& numArea : scoreArea) {
		cv::Mat subArea = img(numArea);
		if (preset->binarization_threshold) {
			cv::cvtColor(subArea, subArea, cv::COLOR_BGR2GRAY);
			cv::threshold(subArea, subArea, preset->binarization_threshold.value(), 255, preset->binarization_mode.value_or(0));
		} else {
			cv::cvtColor(subArea, subArea, cv::COLOR_BGRA2BGR);
		}

		std::optional<int> min = {};
		double minValue = preset->mse_threshold;
		for(int i = 0; i < 10; i++) {
			double msd = cv::norm(subArea, preset->digits[i], cv::NORM_L2, preset->masks[i]) / 256;
			msd = msd * msd / cv::countNonZero(preset->masks[i]);
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

const char *scorecapture_filter_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("ScoreCapture");
}

void scorecapture_filter_update(void *data, obs_data_t *settings)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	// Update the output text source
	update_text_source_on_settings(tf, settings);

	filter_data_set_preset(tf, settings);

	const char *new_mode = obs_data_get_string(settings, "modes");
	if (strcmp(new_mode, tf->mode.c_str()) != 0) {
		tf->mode = new_mode;
	}

	tf->clear_delay = obs_data_get_int(settings, "clear_delay");
}

void scorecapture_filter_activate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	obs_log(LOG_INFO, "scorecapture_filter_activate");
	tf->isDisabled = false;
}

void scorecapture_filter_deactivate(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);
	obs_log(LOG_INFO, "scorecapture_filter_deactivate");
	tf->isDisabled = true;
}

void *scorecapture_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct filter_data *tf = new filter_data;

	tf->source = source;
	tf->unique_id = obs_source_get_uuid(source);
	tf->texrender = gs_texrender_create(GS_BGRA, GS_ZS_NONE);
	tf->output_source_name = bstrdup(obs_data_get_string(settings, "text_sources"));
	tf->output_source = nullptr;

	scorecapture_filter_update(tf, settings);

	signal_handler_t *sh_filter = obs_source_get_signal_handler(tf->source);
	if (sh_filter == nullptr) {
		obs_log(LOG_ERROR, "Failed to get signal handler");
		tf->isDisabled = true;
		return nullptr;
	}

	signal_handler_connect(sh_filter, "enable", enable_callback, tf);

	return tf;
}

void scorecapture_filter_destroy(void *data)
{
	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	signal_handler_t *sh_filter = obs_source_get_signal_handler(tf->source);
	signal_handler_disconnect(sh_filter, "enable", enable_callback, tf);

	if (tf) {
		obs_enter_graphics();
		gs_texrender_destroy(tf->texrender);
		if (tf->stagesurface) {
			gs_stagesurface_destroy(tf->stagesurface);
		}
		if (tf->effect != nullptr) {
			gs_effect_destroy(tf->effect);
		}
		obs_leave_graphics();

		obs_data_release(tf->presets);
		delete tf->preset;

		tf->~filter_data();
		bfree(tf);
	}
}

void scorecapture_filter_video_render(void *data, gs_effect_t *_effect)
{
	UNUSED_PARAMETER(_effect);

	struct filter_data *tf = reinterpret_cast<filter_data *>(data);

	// Skip if not fully initialized
	if (tf->isDisabled || !tf->preset) {
		if (tf->source) {
			obs_source_skip_video_filter(tf->source);
		}
		return;
	}

	if (tf->output_source_name 
			&& strcmp(tf->output_source_name, "none") != 0 
			&& !tf->output_source) {
		acquire_weak_output_source_ref(tf);
	}

	uint32_t width, height;
	auto mat_opt = getRGBAFromStageSurface(tf, width, height);
	if (mat_opt) {
		cv::Mat mat(tf->preset->native_resolution, mat_opt.value().type());
		cv::resize(mat_opt.value(), mat, mat.size());
		
		bool update = false;
		
		if (tf->preset->p1ScoreArea.size() > 0) {
			auto p1New = analyze_img_msd(tf->preset, mat, tf->preset->p1ScoreArea);
			if (p1New != tf->p1Score) {
				tf->p1Score = p1New;
				update = true;
			}
		}
		if (tf->preset->p2ScoreArea.size() > 0) {
			auto p2New = analyze_img_msd(tf->preset, mat, tf->preset->p2ScoreArea);
			if (p2New != tf->p2Score) {
				tf->p2Score = p2New;
				update = true;
			}
		}
		if (update) {
			std::string update_text;
			if(get_update_text(tf, update_text)) {
				update_text_source(tf, update_text);
				tf->clear_timer = 0;

				if (vendor) {
					obs_data_t *update_data = obs_data_create();
					obs_data_set_string(update_data, "source_name", tf->output_source_name);
					if (tf->p1Score) {
						obs_data_set_int(update_data, "p1Score", tf->p1Score.value());
					}
					if (tf->p2Score) {
						obs_data_set_int(update_data, "p2Score", tf->p2Score.value());
					}
					obs_websocket_vendor_emit_event(vendor, "scores_updated", update_data);
					obs_data_release(update_data);
				}
			} else if (tf->clear_delay != -1 && tf->clear_timer++ < tf->clear_delay) {
				update_text_source(tf, "");
			}
		}
	}

	obs_source_skip_video_filter(tf->source);
}