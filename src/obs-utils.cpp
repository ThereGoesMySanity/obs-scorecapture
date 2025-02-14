#include "plugin-support.h"
#include "obs-utils.h"

#include <obs-module.h>

#include <opencv2/core.hpp>

#include <string>
#include <filesystem>
#include <mutex>
#include <opencv2/imgproc.hpp>
#include <fstream>
#include <regex>

std::optional<cv::Mat> getRGBAFromStageSurface(filter_data *tf, uint32_t &width, uint32_t &height)
{

	if (!obs_source_enabled(tf->source)) {
		return {};
	}

	obs_source_t *target = obs_filter_get_target(tf->source);
	if (!target) {
		return {};
	}
	width = obs_source_get_base_width(target);
	height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		return {};
	}
	gs_texrender_reset(tf->texrender);
	if (!gs_texrender_begin(tf->texrender, width, height)) {
		return {};
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f,
		 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(tf->texrender);

	if (tf->stagesurface) {
		uint32_t stagesurf_width = gs_stagesurface_get_width(tf->stagesurface);
		uint32_t stagesurf_height = gs_stagesurface_get_height(tf->stagesurface);
		if (stagesurf_width != width || stagesurf_height != height) {
			gs_stagesurface_destroy(tf->stagesurface);
			tf->stagesurface = nullptr;
		}
	}
	if (!tf->stagesurface) {
		tf->stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
	}
	gs_stage_texture(tf->stagesurface, gs_texrender_get_texture(tf->texrender));
	uint8_t *video_data;
	uint32_t linesize;
	if (!gs_stagesurface_map(tf->stagesurface, &video_data, &linesize)) {
		return {};
	}
	auto ret = cv::Mat(height, width, CV_8UC4, video_data, linesize);
	gs_stagesurface_unmap(tf->stagesurface);
	return std::make_optional(ret);
}

void acquire_weak_output_source_ref(struct filter_data *usd)
{
	obs_source_t *source = obs_get_source_by_name(usd->output_source_name);
	if (source) {
		usd->output_source = obs_source_get_weak_source(source);
		obs_source_release(source);
		if (!usd->output_source) {
			obs_log(LOG_ERROR, "failed to get weak source for source %s",
				usd->output_source_name);
		}
	} else {
		obs_log(LOG_ERROR, "source '%s' not found", usd->output_source_name);
	}
}

void update_text_source_on_settings(struct filter_data *usd, obs_data_t *settings)
{
	// update the text source
	const char *new_source_name = obs_data_get_string(settings, "text_sources");
	obs_weak_source_t *old_weak_source = NULL;

	bool is_valid = is_valid_output_source_name(new_source_name);

	// if we need to free the old one, do so
	if (!is_valid || (usd->output_source_name != nullptr && strcmp(new_source_name, usd->output_source_name) != 0)) {
		if (usd->output_source) {
			old_weak_source = usd->output_source;
			usd->output_source = nullptr;
		}
		if (usd->output_source_name) {
			bfree(usd->output_source_name);
			usd->output_source_name = nullptr;
		}
	}
	if (is_valid) {
		usd->output_source_name = bstrdup(new_source_name);
		acquire_weak_output_source_ref(usd);
	}

	if (old_weak_source) {
		obs_weak_source_release(old_weak_source);
	}
}

void check_plugin_config_folder_exists()
{
	std::string config_folder = obs_module_config_path("");
	if (!std::filesystem::exists(config_folder)) {
		obs_log(LOG_INFO, "Creating plugin config folder: %s", config_folder.c_str());
		std::filesystem::create_directories(config_folder);
	}
}
