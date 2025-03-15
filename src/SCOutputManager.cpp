#include "SCOutputManager.hpp"
#include <format>
#include <obs-websocket-api.h>

extern obs_websocket_vendor vendor;

void SCOutputManager::Init()
{
	sourceCreated = OBSSignal(
		obs_get_signal_handler(), "source_create",
		[](void *data, calldata_t *cd) {
			auto source = static_cast<obs_source_t *>(calldata_ptr(cd, "source"));
			if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0 ||
			    strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
				static_cast<SCOutputManager *>(data)->output_sources.push_back(
					obs_source_get_weak_source(source));
			}
		},
		this);
	obs_enum_sources(
		[](void *data, obs_source_t *source) {
			if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0 ||
			    strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
				static_cast<SCOutputManager *>(data)->output_sources.push_back(
					obs_source_get_weak_source(source));
			}
			return true;
		},
		this);
}
void SCOutputManager::SetScores(const char *source, std::optional<ScoreData> p1Score, std::optional<ScoreData> p2Score)
{
	std::erase_if(output_sources, [](obs_weak_source_t *ws) { return !obs_weak_source_get_source(ws); });

	scores[0] = p1Score;
	scores[1] = p2Score;

	OBSDataAutoRelease update_data = obs_data_create();
	obs_data_set_string(update_data, "name", source);
	for (int i = 0; i < 2; i++) {
		if (!scores[i])
			continue;

		std::string name = std::format("p{}Score", i + 1);
		OBSDataAutoRelease pData = obs_data_create();
		scores[i]->toObsData(pData);
		obs_data_set_obj(update_data, name.c_str(), pData);
	}

	for (auto &weak_source : output_sources) {
		const OBSSourceAutoRelease source = obs_weak_source_get_source(weak_source);
		if (!source)
			continue;

		// if (strncmp("text_", obs_source_get_unversioned_id(source), 5) == 0) {
		// 	updateTextSource(source, update_text.value());
		// }

		if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0) {
			calldata_t params = {0};
			calldata_set_ptr(&params, "p1Score", &p1Score);
			calldata_set_ptr(&params, "p2Score", &p2Score);
			proc_handler_call(obs_source_get_proc_handler(source), "scores_updated", &params);
		}

		if (strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
			calldata_t params = {0};
			calldata_set_string(&params, "eventName", "scores_updated");
			calldata_set_string(&params, "jsonString", obs_data_get_json(update_data));
			proc_handler_call(obs_source_get_proc_handler(source), "javascript_event", &params);
		}
	}

	if (vendor) {
		obs_websocket_vendor_emit_event(vendor, "scores_updated", update_data);
	}
}

void SCOutputManager::ClearScores(const char *source)
{
	std::erase_if(output_sources, [](obs_weak_source_t *ws) { return !obs_weak_source_get_source(ws); });

	OBSDataAutoRelease update_data = obs_data_create();
	obs_data_set_string(update_data, "name", source);

	for (auto &weak_source : output_sources) {
		const OBSSourceAutoRelease source = obs_weak_source_get_source(weak_source);
		if (!source)
			continue;

		// if (strncmp("text_", obs_source_get_unversioned_id(source), 5) == 0) {
		// 	updateTextSource(source, "");
		// }

		if (strcmp("scorecapture_source", obs_source_get_unversioned_id(source)) == 0) {
			calldata_t params = {0};
			proc_handler_call(obs_source_get_proc_handler(source), "scores_cleared", &params);
		}
		if (strcmp("browser_source", obs_source_get_unversioned_id(source)) == 0) {
			calldata_t params = {0};
			calldata_set_string(&params, "eventName", "scores_cleared");
			calldata_set_string(&params, "jsonString", obs_data_get_json(update_data));
			proc_handler_call(obs_source_get_proc_handler(source), "javascript_event", &params);
		}
	}

	if (vendor) {
		obs_websocket_vendor_emit_event(vendor, "scores_cleared", update_data);
	}
}