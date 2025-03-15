#pragma once
#include <opencv2/imgproc.hpp>
#include <obs.h>
#include <optional>
#include <obs.hpp>
#include "Preset.hpp"
#include <future>

class SCFilter {
public:
	SCFilter(obs_source_t *_source);
	~SCFilter();
	void update(obs_data_t *settings);
	void activate();
	void deactivate();
	void enable(bool enabled);
	void videoRender(gs_effect_t *unused);
	obs_properties_t *getProperties();
	static void getDefaults(obs_data_t *settings);
	void updatePresetSettings(obs_data_t *settings);
	bool presetChanged(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings);

private:
	bool shouldUpdate();
	std::optional<cv::Mat> getRGBAFromStageSurface();
	void doProcessing(cv::Mat mat);

	void addPresets(obs_properties_t *props);
	void addModes(obs_properties_t *props);

	std::thread processing_thread;
	std::promise<std::optional<cv::Mat>> promise;
	std::mutex processing_mutex;

	std::optional<Preset> preset = {};
	OBSSourceAutoRelease source = nullptr;
	std::string unique_id;
	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurface = nullptr;

	OBSDataAutoRelease presets = nullptr;

	std::string mode;

	std::optional<int> p1Score, p2Score;

	int clear_delay = 0;
	int clear_timer = 0;

	bool isDisabled = false;

	OBSSignal enableSignal;
};
