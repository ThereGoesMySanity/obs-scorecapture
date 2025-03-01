#include <obs-module.h>
#include "SCFilter.hpp"

extern "C" {
struct obs_source_info scorecapture_filter_info = {
	.id = "scorecapture_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO,
	.get_name =
		[](void *unused) {
			UNUSED_PARAMETER(unused);
			return obs_module_text("ScoreCapture");
		},
	.create =
		[](obs_data_t *settings, obs_source_t *source) {
			auto s = new SCFilter(source);
			s->update(settings);
			return static_cast<void *>(s);
		},
	.destroy = [](void *data) { delete static_cast<SCFilter *>(data); },
	.get_defaults = SCFilter::getDefaults,
	.get_properties = [](void *data) { return static_cast<SCFilter *>(data)->getProperties(); },
	.update = [](void *data, obs_data_t *settings) { static_cast<SCFilter *>(data)->update(settings); },
	.activate = [](void *data) { static_cast<SCFilter *>(data)->activate(); },
	.deactivate = [](void *data) { static_cast<SCFilter *>(data)->deactivate(); },
	.video_render =
		[](void *data, gs_effect_t *effect) {
			static_cast<SCFilter *>(data)->videoRender(effect);
		}};
}
