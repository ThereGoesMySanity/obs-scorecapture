#include "plugin-support.h"
#include "obs-utils.h"

#include <string>
#include <filesystem>

#include <obs-module.h>
#include <obs.hpp>

extern "C" {
bool render_to_stagesurface(obs_source_t *target, gs_texrender_t *texrender, gs_stagesurf_t **stagesurface)
{
	uint32_t width = obs_source_get_base_width(target);
	uint32_t height = obs_source_get_base_height(target);
	if (width == 0 || height == 0) {
		return false;
	}
	gs_texrender_reset(texrender);
	if (!gs_texrender_begin(texrender, width, height)) {
		return false;
	}
	struct vec4 background;
	vec4_zero(&background);
	gs_clear(GS_CLEAR_COLOR, &background, 0.0f, 0);
	gs_ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height), -100.0f, 100.0f);
	gs_blend_state_push();
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
	obs_source_video_render(target);
	gs_blend_state_pop();
	gs_texrender_end(texrender);

	if (*stagesurface) {
		uint32_t stagesurf_width = gs_stagesurface_get_width(*stagesurface);
		uint32_t stagesurf_height = gs_stagesurface_get_height(*stagesurface);
		if (stagesurf_width != width || stagesurf_height != height) {
			gs_stagesurface_destroy(*stagesurface);
			*stagesurface = nullptr;
		}
	}
	if (!*stagesurface) {
		*stagesurface = gs_stagesurface_create(width, height, GS_BGRA);
	}
	gs_stage_texture(*stagesurface, gs_texrender_get_texture(texrender));
	return true;
}

obs_weak_source_t *acquire_weak_output_source_ref(const char *name)
{
	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source) {
		auto weak = obs_source_get_weak_source(source);
		if (!weak) {
			obs_log(LOG_ERROR, "failed to get weak source for source %s", name);
		}
		return weak;
	} else {
		obs_log(LOG_ERROR, "source '%s' not found", name);
	}
	return nullptr;
}

void check_plugin_config_folder_exists()
{
	std::string config_folder = obs_module_config_path("");
	if (!std::filesystem::exists(config_folder)) {
		obs_log(LOG_INFO, "Creating plugin config folder: %s", config_folder.c_str());
		std::filesystem::create_directories(config_folder);
	}
}

obs_data_t *load_presets()
{
	char *presets_file = obs_module_config_path("presets.json");
	obs_data_t *presets = obs_data_create_from_json_file(presets_file);
	bfree(presets_file);
	return presets;
}
bool is_valid_output_source_name(const char *output_source_name)
{
	std::string name = output_source_name;
	return !name.empty() && name != "none" && name != "(null)";
}
}
