#pragma once

#include <optional>
#include <vector>
#include <string>
#include <obs.hpp>

#include "ScoreData.hpp"

class SCOutputManager {
public:
	void Init();
	void SetScores(const char *source, std::optional<ScoreData> p1Score, std::optional<ScoreData> p2Score);
	void ClearScores(const char *source);

private:
	std::optional<ScoreData> scores[2];

	std::vector<OBSWeakSourceAutoRelease> output_sources;
	OBSSignal sourceCreated;
};
