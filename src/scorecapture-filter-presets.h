#pragma once
#include "scorecapture-data.h"

obs_data_t* load_presets();
preset* load_preset(filter_data *tf, const char *name);
void filter_data_set_preset(filter_data *tf, obs_data_t *settings);