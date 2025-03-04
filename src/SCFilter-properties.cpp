#include <obs-module.h>
#include "SCFilter.hpp"
#include "obs-utils.h"

bool SCFilter::presetChanged(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings)
{
	UNUSED_PARAMETER(prop);
	updatePresetSettings(settings);
	obs_property_set_visible(obs_properties_get(props, "modes"), this->preset && this->preset->isTwoPlayer());
	return true;
}

void SCFilter::addPresets(obs_properties_t *props)
{
	presets = load_presets();
	obs_property_t *preset_list = obs_properties_add_list(props, "preset", obs_module_text("Preset"),
							      OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_data_item_t *item = obs_data_first(presets);
	for (; item != NULL; obs_data_item_next(&item)) {
		const char *name = obs_data_item_get_name(item);
		obs_property_list_add_string(preset_list, name, name);
	}

	obs_property_set_modified_callback2(
		preset_list,
		[](void *data, obs_properties_t *props, obs_property_t *prop, obs_data_t *settings) {
			return static_cast<SCFilter *>(data)->presetChanged(props, prop, settings);
		},
		this);
}
void add_text_source_output(obs_properties_t *props)
{
	// Add a property for the output text source
	obs_property_t *text_sources = obs_properties_add_list(props, "text_sources", obs_module_text("TextSource"),
							       OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_enum_sources(
		[](void *data, obs_source_t *source) -> bool {
			obs_property_t *text_sources = (obs_property_t *)data;
			const char *source_type = obs_source_get_id(source);
			if (strstr(source_type, "text") != NULL) {
				const char *name = obs_source_get_name(source);
				obs_property_list_add_string(text_sources, name, name);
			}

			return true;
		},
		text_sources);

	obs_property_list_add_string(text_sources, obs_module_text("NoOutput"), "none");
}

void SCFilter::addModes(obs_properties_t *props)
{
	obs_property_t *modes = obs_properties_add_list(props, "modes", obs_module_text("Mode"), OBS_COMBO_TYPE_LIST,
							OBS_COMBO_FORMAT_STRING);
	obs_property_list_add_string(modes, obs_module_text("AutoDetect"), "AutoDetect");
	obs_property_list_add_string(modes, obs_module_text("P1"), "P1");
	obs_property_list_add_string(modes, obs_module_text("P2"), "P2");
	obs_property_list_add_string(modes, obs_module_text("Versus"), "Versus");
	obs_property_set_visible(modes, preset && preset->isTwoPlayer());
}

obs_properties_t *SCFilter::getProperties()
{
	obs_properties_t *props = obs_properties_create();

	addPresets(props);

	addModes(props);

	add_text_source_output(props);

	obs_properties_add_int(props, "clear_delay", obs_module_text("ClearDelay"), -1, 30, 1);

	return props;
}

void SCFilter::getDefaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "text_sources", "none");
	obs_data_set_default_string(settings, "preset", "none");
	obs_data_set_default_string(settings, "modes", "AutoDetect");
	obs_data_set_default_bool(settings, "update_always", false);
	obs_data_set_default_int(settings, "clear_delay", 5);
}
