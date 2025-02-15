#include "SCFilter.hpp"
#include <obs-module.h>
#include "obs-utils.h"
#include <obs-websocket-api.h>
#include "plugin-support.h"

extern obs_websocket_vendor *vendor;

SCFilter::SCFilter(obs_source_t *_source)
	: source(_source),
	  unique_id(obs_source_get_uuid(_source)),
	  texrender(gs_texrender_create(GS_BGRA, GS_ZS_NONE)),
	  enableSignal(
		  obs_source_get_signal_handler(source), "enable",
		  [](void *data, calldata_t *cd) {
			  static_cast<SCFilter *>(data)->enable(calldata_bool(cd, "enabled"));
		  },
		  this)
{
}

SCFilter::~SCFilter() {
    obs_enter_graphics();
    gs_texrender_destroy(texrender);
    if (stagesurface) {
        gs_stagesurface_destroy(stagesurface);
    }
    obs_leave_graphics();
}

void SCFilter::update(obs_data_t *settings) {
    updateTextSourceSettings(settings);
	updateDisplaySourceSettings(settings);
	updatePresetSettings(settings);

	mode = obs_data_get_string(settings, "modes");
	clear_delay = obs_data_get_int(settings, "clear_delay");
}

void SCFilter::activate() {
    obs_log(LOG_INFO, "scorecapture_filter_activate");
	isDisabled = false;
}

void SCFilter::deactivate() {
    obs_log(LOG_INFO, "scorecapture_filter_deactivate");
	isDisabled = true;
}

void SCFilter::enable(bool enabled) {
    isDisabled = !enabled;
}

void SCFilter::videoRender(gs_effect_t *unused)
{
    UNUSED_PARAMETER(unused);
	// Skip if not fully initialized
	if (isDisabled || !preset) {
		if (source) {
			obs_source_skip_video_filter(source);
		}
		return;
	}

	if (is_valid_output_source_name(text_source_name.c_str()) && !text_source) {
		text_source = acquire_weak_output_source_ref(text_source_name.c_str());
	}
	if (is_valid_output_source_name(display_source_name.c_str()) && !display_source) {
		display_source = acquire_weak_output_source_ref(display_source_name.c_str());
	}

	auto mat_opt = getRGBAFromStageSurface();
	if (mat_opt) {
		std::optional<int> p1New, p2New;
		preset->analyzeImageMsd(mat_opt.value(), p1New, p2New);
		
		if (p1New > p1Score || p2New > p2Score) {
			p1Score = p1New;
			p2Score = p2New;
			std::optional<std::string> update_text = getUpdateText();
			if(update_text) {
				clear_timer = 0;

				if (text_source) {
					updateTextSource(update_text.value());
				}

				if (display_source) {
					OBSSourceAutoRelease source = obs_weak_source_get_source(display_source);
					if (source) {
						calldata_t params = {0};
						calldata_set_ptr(&params, "p1Score", &p1Score);
						calldata_set_ptr(&params, "p2Score", &p2Score);
						proc_handler_call(obs_source_get_proc_handler(source), "scores_updated", &params);
					} else {
						display_source = {};
					}
				}

				if (vendor) {
					OBSDataAutoRelease update_data = obs_data_create();
					obs_data_set_string(update_data, "source_name", text_source_name.c_str());
					if (p1Score) {
						obs_data_set_int(update_data, "p1Score", p1Score.value());
					}
					if (p2Score) {
						obs_data_set_int(update_data, "p2Score", p2Score.value());
					}
					obs_websocket_vendor_emit_event(vendor, "scores_updated", update_data);
				}
			}
		} else if (!p1New && !p2New && clear_delay != -1 && clear_timer++ < clear_delay) {
			p1Score = {};
			p2Score = {};
			if (text_source) {
				updateTextSource("");
			}

			if (display_source) {
				OBSSourceAutoRelease source = obs_weak_source_get_source(display_source);
				if (source) {
					calldata_t params = {0};
					proc_handler_call(obs_source_get_proc_handler(source), "scores_cleared", &params);
				} else {
					display_source = {};
				}
			}
		}
	}

	obs_source_skip_video_filter(source);

}

std::optional<std::string> SCFilter::getUpdateText()
{
	std::optional<std::string> update_text = {};
	bool autodetect = mode == "AutoDetect";
	if (p1Score && p2Score && (autodetect || mode == "Versus")) {
		int diff = p1Score.value() - p2Score.value();
		if (diff > 0) {
			update_text = "P1\n" + std::to_string(diff);
		} else if (diff < 0) {
			update_text = "P2\n" + std::to_string(-diff);
		} else {
			update_text = "Tie";
		}
	} else if (p1Score && (autodetect || mode == "P1")) {
		update_text = std::to_string(p1Score.value());
	} else if (p2Score && (autodetect || mode == "P2")) {
		update_text = std::to_string(p2Score.value());
	}
	return update_text;
}

bool SCFilter::updateTextSource(std::string updateText)
{
	if (!text_source) {
		obs_log(LOG_ERROR, "text_source is null");
		return false;
	}
	OBSSourceAutoRelease target = obs_weak_source_get_source(text_source);
	if (!target) {
		obs_log(LOG_ERROR, "text_source target is null");
		return false;
	}
	OBSDataAutoRelease text_settings = obs_source_get_settings(target);
	obs_data_set_string(text_settings, "text", updateText.c_str());
	obs_source_update(target, text_settings);
	return true;
}

void SCFilter::updateTextSourceSettings(obs_data_t *settings) {
	std::string new_source_name = obs_data_get_string(settings, "text_sources");
	bool is_valid = is_valid_output_source_name(new_source_name.c_str());

	if (is_valid && new_source_name != text_source_name) {
		text_source_name = new_source_name;
		text_source = acquire_weak_output_source_ref(text_source_name.c_str());
	}
}

void SCFilter::updateDisplaySourceSettings(obs_data_t *settings) {
	std::string new_source_name = obs_data_get_string(settings, "display_sources");
	bool is_valid = is_valid_output_source_name(new_source_name.c_str());

	if (is_valid && new_source_name != display_source_name) {
		display_source_name = new_source_name;
		display_source = acquire_weak_output_source_ref(display_source_name.c_str());
	}
}

void SCFilter::updatePresetSettings(obs_data_t *settings) {
	std::string new_preset = obs_data_get_string(settings, "preset");
	if (!preset || new_preset != preset->name) {
		if (!presets) {
			presets = load_presets();
		}
		// Load new preset
		OBSDataAutoRelease p = obs_data_get_obj(presets, new_preset.c_str());
        preset = Preset();
		//Unload if failed
		if (!preset->load(new_preset, p)) {
			preset = {};
		}
	}
}

std::optional<cv::Mat> SCFilter::getRGBAFromStageSurface()
{
	if (!obs_source_enabled(source)) {
		return {};
	}

	obs_source_t *target = obs_filter_get_target(source);
	if (!target) {
		return {};
	}
    if (!render_to_stagesurface(target, texrender, &stagesurface)){
        return {};
    }
	uint8_t *video_data;
	uint32_t linesize;
	if (!gs_stagesurface_map(stagesurface, &video_data, &linesize)) {
		return {};
	}
    auto ret = cv::Mat(gs_stagesurface_get_height(stagesurface), gs_stagesurface_get_width(stagesurface), CV_8UC4,
		       video_data, linesize);
    gs_stagesurface_unmap(stagesurface);
    return ret;
}