#pragma once

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *scorecapture_filter_getname(void *unused);
void *scorecapture_filter_create(obs_data_t *settings, obs_source_t *source);
void scorecapture_filter_destroy(void *data);
void scorecapture_filter_defaults(obs_data_t *settings);
obs_properties_t *scorecapture_filter_properties(void *data);
void scorecapture_filter_update(void *data, obs_data_t *settings);
void scorecapture_filter_activate(void *data);
void scorecapture_filter_deactivate(void *data);
void scorecapture_filter_video_render(void *data, gs_effect_t *_effect);


#ifdef __cplusplus
}
#endif