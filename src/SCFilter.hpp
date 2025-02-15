#pragma once
#include <opencv2/imgproc.hpp>
#include <obs.h>
#include <optional>
#include <obs.hpp>
#include "Preset.hpp"

class SCFilter {
public:
    SCFilter(obs_source_t *_source);
    ~SCFilter();
    void update(obs_data_t *settings);
    void activate();
    void deactivate();
    void enable(bool enabled);
    void videoRender(gs_effect_t *unused);
    obs_properties_t *getProperties();
    static void getDefaults(obs_data_t *settings);
    void updatePresetSettings(obs_data_t *settings);
    bool presetChanged(obs_properties_t *props, obs_property_t *prop, obs_data_t *settings);

    private:
    std::optional<std::string> getUpdateText();
    bool updateTextSource(std::string updateText);
    void updateTextSourceSettings(obs_data_t *settings);
    void updateDisplaySourceSettings(obs_data_t *settings);
    std::optional<cv::Mat> getRGBAFromStageSurface();

    void addPresets(obs_properties_t *props);
    void addModes(obs_properties_t *props);
    
    std::optional<Preset> preset;
    OBSSourceAutoRelease source = nullptr;
    std::string unique_id;
    gs_texrender_t *texrender = nullptr;
    gs_stagesurf_t *stagesurface = nullptr;

    OBSDataAutoRelease presets = nullptr;

    std::string mode;

    std::optional<int> p1Score, p2Score;

    int clear_delay;
    int clear_timer = 0;

    bool isDisabled = false;

    OBSSignal enableSignal;

    // Text source to output the text to
    OBSWeakSourceAutoRelease text_source = nullptr;
    std::string text_source_name;

    OBSWeakSourceAutoRelease display_source = nullptr;
    std::string display_source_name;
};
