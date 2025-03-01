#include "SCSource.hpp"
#include <obs-module.h>

extern "C" {
struct obs_source_info scorecapture_source_info = {
	.id = "scorecapture_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,

	.get_name =
		[](void *unused) {
			UNUSED_PARAMETER(unused);
			return obs_module_text("SourceName");
		},
	.create =
		[](obs_data_t *settings, obs_source_t *source) {
			auto s = new SCSource(source);
			s->update(settings);
			return static_cast<void *>(s);
		},
	.destroy = [](void *data) { delete static_cast<SCSource *>(data); },

	.get_width = [](void *data) { return static_cast<SCSource *>(data)->getWidth(); },
	.get_height = [](void *data) { return static_cast<SCSource *>(data)->getHeight(); },

	.get_defaults = SCSource::getDefaults,
	.get_properties = [](void *data) { return static_cast<SCSource *>(data)->getProperties(); },
	.update = [](void *data, obs_data_t *settings) { static_cast<SCSource *>(data)->update(settings); },
	.activate = [](void *data) { static_cast<SCSource *>(data)->activate(); },
	.deactivate = [](void *data) { static_cast<SCSource *>(data)->deactivate(); },
	.video_render = [](void *data, gs_effect_t *effect) { static_cast<SCSource *>(data)->videoRender(effect); },

};
}
