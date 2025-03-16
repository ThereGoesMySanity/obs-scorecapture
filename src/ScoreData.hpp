#pragma once

#include <optional>
struct ScoreData {
	int score;
	std::optional<int> maxScore;

	ScoreData(int _score) : score(_score) {}

	ScoreData(obs_data_t *data)
	{
		score = (int)obs_data_get_int(data, "score");
		if (obs_data_has_user_value(data, "maxScore")) {
			maxScore = (int)obs_data_get_int(data, "maxScore");
		}
	}

	void toObsData(obs_data_t *data)
	{
		obs_data_set_int(data, "score", score);
		if (maxScore) {
			obs_data_set_int(data, "maxScore", maxScore.value());
		}
	}
};
