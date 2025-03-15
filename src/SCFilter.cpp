#include "SCFilter.hpp"
#include <obs-module.h>
#include "obs-utils.h"
#include <obs-websocket-api.h>
#include "plugin-support.h"
#include "ScoreData.hpp"
#include "SCOutputManager.hpp"

extern "C" {
extern obs_websocket_vendor *vendor;
extern SCOutputManager manager;
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
		if (shouldUpdate()) {
			clear_timer = 0;
			manager.SetScores(obs_source_get_name(source), 
				p1Score ? std::optional(ScoreData(p1Score.value())) : std::optional<ScoreData>(),
				p2Score ? std::optional(ScoreData(p2Score.value())) : std::optional<ScoreData>());
		}
	} else if ((p1Score || p2Score) && !p1New && !p2New && clear_delay != -1 && clear_timer++ > clear_delay) {
		p1Score = {};
		p2Score = {};

	}
}

bool SCFilter::shouldUpdate()
{
	bool autodetect = mode == "AutoDetect";
	return (p1Score && p2Score && (autodetect || mode == "Versus")) 
		|| (p1Score && (autodetect || mode == "P1"))
		|| (p2Score && (autodetect || mode == "P2"));
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
