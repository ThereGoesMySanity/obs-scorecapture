#pragma once
#include <QImage>
#include <QFont>
#include <obs.hpp>

class SCSource {
public:
    SCSource(obs_source_t *_source);
    ~SCSource();
    uint32_t getWidth() { return width; }
    uint32_t getHeight() { return height; }
    void update(obs_data_t *settings);
    void activate();
    void deactivate();
    void enable(bool enabled);
    void videoRender(gs_effect_t *effect);
    obs_properties_t *getProperties();
    static void getDefaults(obs_data_t *settings);

private:
    void scoresUpdated(std::optional<int> p1Score, std::optional<int> p2Score);
    void scoresCleared();

	obs_source_t *source = nullptr;
    OBSSignal enableSignal;
	struct vec2 size;
	gs_texrender_t *texrender = nullptr;
	QImage img;

	uint32_t width = 1280;
	uint32_t height = 100;

	bool isDisabled = false;

	std::optional<int> p1Score, p2Score;
	bool rerender = false;

	int max_score = 400;

	QColor p1Color;
	QColor p2Color;
	QColor text_color;
	QColor background_color;

	QFont font;
	QFont small_font;
	QFont bold_font;
};
