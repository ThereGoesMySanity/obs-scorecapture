#include "scorecapture-filter.h"

struct obs_source_info scorecapture_filter_info = {
	.id = "scorecapture_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name = scorecapture_filter_getname,
	.create = scorecapture_filter_create,
	.destroy = scorecapture_filter_destroy,
	.get_defaults = scorecapture_filter_defaults,
	.get_properties = scorecapture_filter_properties,
	.update = scorecapture_filter_update,
	.activate = scorecapture_filter_activate,
	.deactivate = scorecapture_filter_deactivate,
	.video_render = scorecapture_filter_video_render,
};
