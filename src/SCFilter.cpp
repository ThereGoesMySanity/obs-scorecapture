#include "SCFilter.hpp"
#include <obs-module.h>
#include "obs-utils.h"
#include <obs-websocket-api.h>
#include "plugin-support.h"

extern "C" {
extern obs_websocket_vendor *vendor;
}

SCFilter::SCFilter(obs_source_t *_source)
	: source(_source),
	  unique_id(obs_source_get_uuid(_source)),
	  texrender(gs_texrender_create(GS_BGRA, GS_ZS_NONE)),
	  enableSignal(
		  obs_source_get_signal_handler(_source), "enable",
		  [](void *data, calldata_t *cd) {
			  static_cast<SCFilter *>(data)->enable(calldata_bool(cd, "enabled"));
		  },
		  this),
	  sourceCreated(
		  obs_get_signal_handler(), "source_create",
		  [](void *data, calldata_t *cd) {
			  auto source = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
			  if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0 ||
			      strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
				  static_cast<SCFilter *>(data)->output_sources.push_back(
					  obs_source_get_weak_source(source));
			  }
		  },
		  this)
{
	processing_thread = std::thread([this]() {
		while (this->source) {
			std::unique_lock lock(processing_mutex, std::defer_lock);
			auto mat = this->promise.get_future().get();
			lock.lock();
			if (mat)
				this->doProcessing(mat.value());
		}
	});
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0 ||
			    strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
				static_cast<SCFilter *>(data)->output_sources.push_back(
					obs_source_get_weak_source(source));
			}
			return true;
		},
		this);
}

SCFilter::~SCFilter()
{
	obs_enter_graphics();
	gs_texrender_destroy(texrender);
	if (stagesurface) {
		gs_stagesurface_destroy(stagesurface);
	}
	obs_leave_graphics();
	source = nullptr;
	promise.set_value({});
	processing_thread.join();
}

void SCFilter::update(obs_data_t *settings)
{
	updateTextSourceSettings(settings);
	updatePresetSettings(settings);

	mode = obs_data_get_string(settings, "modes");
	clear_delay = (int)obs_data_get_int(settings, "clear_delay");
}

void SCFilter::activate()
{
	obs_log(LOG_INFO, "scorecapture_filter_activate");
	isDisabled = false;
}

void SCFilter::deactivate()
{
	obs_log(LOG_INFO, "scorecapture_filter_deactivate");
	isDisabled = true;
}

void SCFilter::enable(bool enabled)
{
	isDisabled = !enabled;
}

void SCFilter::videoRender(gs_effect_t *unused)
{
	UNUSED_PARAMETER(unused);
	if (source) {
		obs_source_skip_video_filter(source);
	}

	std::unique_lock lock(processing_mutex, std::defer_lock);
	if (lock.try_lock()) {
		auto mat_opt = getRGBAFromStageSurface();
		if (mat_opt) {
			promise.set_value(mat_opt);
			promise = {};
		}
	}
}

void SCFilter::doProcessing(cv::Mat mat)
{
	std::optional<int> p1New, p2New;
	preset->analyzeImageMsd(mat, p1New, p2New);

	if (p1New > p1Score || p2New > p2Score) {
		if (p1New > p1Score) p1Score = p1New;
		if (p2New > p2Score) p2Score = p2New;
		std::optional<std::string> update_text = getUpdateText();
		if (update_text) {
			clear_timer = 0;
			std::erase_if(output_sources,
				      [](obs_weak_source_t *ws) { return !obs_weak_source_get_source(ws); });

			OBSDataAutoRelease update_data = obs_data_create();
			obs_data_set_string(update_data, "name", obs_source_get_name(source));
			obs_data_set_string(update_data, "preset", preset->name.c_str());
			if (p1Score) {
				obs_data_set_int(update_data, "p1Score", p1Score.value());
			}
			if (p2Score) {
				obs_data_set_int(update_data, "p2Score", p2Score.value());
			}

			for (auto &weak_source : output_sources) {
				const OBSSourceAutoRelease source = obs_weak_source_get_source(weak_source);
				if (!source)
					continue;

				if (strncmp("text_", obs_source_get_unversioned_id(source), 5) == 0) {
					updateTextSource(source, update_text.value());
				}

				if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0) {
					calldata_t params = {0};
					calldata_set_ptr(&params, "p1Score", &p1Score);
					calldata_set_ptr(&params, "p2Score", &p2Score);
					proc_handler_call(obs_source_get_proc_handler(source), "scores_updated",
							  &params);
				}

				if (strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
					calldata_t params = {0};
					calldata_set_string(&params, "eventName", "scores_updated");
					calldata_set_string(&params, "jsonString", obs_data_get_json(update_data));
					proc_handler_call(obs_source_get_proc_handler(source), "javascript_event",
							  &params);
				}
			}

			if (vendor) {
				obs_websocket_vendor_emit_event(vendor, "scores_updated", update_data);
			}
		}
	} else if ((p1Score || p2Score) && !p1New && !p2New && clear_delay != -1 && clear_timer++ > clear_delay) {
		p1Score = {};
		p2Score = {};

		std::erase_if(output_sources, [](obs_weak_source_t *ws) { return !obs_weak_source_get_source(ws); });

		OBSDataAutoRelease update_data = obs_data_create();
		obs_data_set_string(update_data, "name", obs_source_get_name(source));

		for (auto &weak_source : output_sources) {
			const OBSSourceAutoRelease source = obs_weak_source_get_source(weak_source);
			if (!source)
				continue;

			if (strncmp("text_", obs_source_get_unversioned_id(source), 5) == 0) {
				updateTextSource(source, "");
			}

			if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0) {
				calldata_t params = {0};
				proc_handler_call(obs_source_get_proc_handler(source), "scores_cleared", &params);
			}
			if (strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
				calldata_t params = {0};
				calldata_set_string(&params, "eventName", "scores_cleared");
				calldata_set_string(&params, "jsonString", obs_data_get_json(update_data));
				proc_handler_call(obs_source_get_proc_handler(source), "javascript_event", &params);
			}
		}

		if (vendor) {
			obs_websocket_vendor_emit_event(vendor, "scores_cleared", update_data);
		}
	}
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

bool SCFilter::updateTextSource(const OBSSourceAutoRelease &text_source, std::string updateText)
{
	if (!text_source) {
		obs_log(LOG_ERROR, "text_source is null");
		return false;
	}
	OBSDataAutoRelease text_settings = obs_source_get_settings(text_source);
	obs_data_set_string(text_settings, "text", updateText.c_str());
	obs_source_update(text_source, text_settings);
	return true;
}

void SCFilter::updateTextSourceSettings(obs_data_t *settings)
{
	const char *new_source_name = obs_data_get_string(settings, "text_sources");
	bool is_valid = is_valid_output_source_name(new_source_name);

	if (is_valid) {
		output_sources.push_back(acquire_weak_output_source_ref(new_source_name));
	}
}

void SCFilter::updatePresetSettings(obs_data_t *settings)
{
	std::string new_preset = obs_data_get_string(settings, "preset");
	if (!preset || new_preset != preset->name) {
		if (!presets) {
			presets = load_presets();
		}
		// Load new preset
		OBSDataAutoRelease p = obs_data_get_obj(presets, new_preset.c_str());
		preset = {};
		auto newp = Preset();
		//Unload if failed
		if (newp.load(new_preset, p)) {
			preset = newp;
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
	if (!render_to_stagesurface(target, texrender, &stagesurface)) {
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
