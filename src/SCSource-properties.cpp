#include <obs-module.h>
#include <obs.hpp>
#include "SCSource.hpp"

void add_score_types(obs_properties_t *props) {
	obs_property_t *type_list = obs_properties_add_list(props, "score_types", obs_module_text("ScoreType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
	char* score_types_file = obs_module_config_path("score_types.json");
    OBSDataAutoRelease types = obs_data_create_from_json_file(score_types_file);

    obs_data_item_t *item = obs_data_first(types);
    for (; item != NULL; obs_data_item_next(&item)) {
        const char *name = obs_data_item_get_name(item);
        obs_property_list_add_int(type_list, name, obs_data_item_get_int(item));
    }
}

obs_properties_t *SCSource::getProperties(){
	obs_properties_t *props = obs_properties_create();

	add_score_types(props);

	obs_properties_add_color(props, "p1_color", obs_module_text("P1Color"));
	obs_properties_add_color(props, "p2_color", obs_module_text("P2Color"));
	obs_properties_add_color(props, "text_color", obs_module_text("TextColor"));
	obs_properties_add_color_alpha(props, "background_color", obs_module_text("BackgroundColor"));

	obs_properties_add_font(props, "font", obs_module_text("Font"));

	return props;
}

void SCSource::getDefaults(obs_data_t *settings) {
	obs_data_set_default_int(settings, "p1_color", 0xDF0909);
	obs_data_set_default_int(settings, "p2_color", 0x0909DF);
	obs_data_set_default_int(settings, "text_color", 0xFFFFFF);
	obs_data_set_default_int(settings, "background_color", 0x0);
	obs_data_set_default_int(settings, "max_score", 400);
}