#pragma once
#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

bool render_to_stagesurface(obs_source_t *target, gs_texrender_t *texrender, gs_stagesurf_t **stagesurface);

bool is_valid_output_source_name(const char *output_source_name);

obs_weak_source_t* acquire_weak_output_source_ref(const char *name);

void check_plugin_config_folder_exists();
obs_data_t* load_presets();

#ifdef __cplusplus
}
#endif
