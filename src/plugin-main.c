/*
obs-scorecapture
Copyright (C) 2025 Will Kennedy

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-module.h>
#include <obs-websocket-api.h>
#include "plugin-support.h"
#include "obs-utils.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

extern struct obs_source_info scorecapture_filter_info;
extern struct obs_source_info scorecapture_source_info;

obs_websocket_vendor vendor = NULL;

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("ScoreCapture");
}

bool obs_module_load(void)
{
	check_plugin_config_folder_exists();
	obs_register_source(&scorecapture_filter_info);
	obs_register_source(&scorecapture_source_info);
	obs_log(LOG_INFO, "plugin loaded successfully (version %s)", "1.0.0");
	return true;
}

void obs_module_post_load(void)
{
	vendor = obs_websocket_register_vendor("ScoreCapture");
}

void obs_module_unload(void)
{
	obs_log(LOG_INFO, "plugin unloaded");
}
