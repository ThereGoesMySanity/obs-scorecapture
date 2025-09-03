#pragma once

#include <optional>
struct ScoreData {
	double score;
	std::optional<double> maxScore;

	ScoreData(double _score) : score(_score) {}

	ScoreData(obs_data_t *data)
	{
		score = obs_data_get_double(data, "score");
		if (obs_data_has_user_value(data, "maxScore")) {
			maxScore = obs_data_get_double(data, "maxScore");
		}
	}

	void toObsData(obs_data_t *data)
	{
		obs_data_set_double(data, "score", score);
		if (maxScore) {
			obs_data_set_double(data, "maxScore", maxScore.value());
		}
	}
};
