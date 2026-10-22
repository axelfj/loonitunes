/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "StepGrid.h"
#include "Strings.h"

namespace loonitunes
{

class MainComponent : public juce::Component,
                      private juce::Timer
{
public:
    explicit MainComponent (Song&);

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    void timerCallback() override;

    void togglePlay();
    void exportBeat();
    void updateTexts();

    Song& song;
    Language language = Language::spanish;

    juce::TextButton playButton, starterButton, clearButton, exportButton, languageButton;
    juce::Slider tempoSlider;
    juce::Label tempoLabel;
    StepGrid grid { song };

    std::unique_ptr<juce::FileChooser> chooser;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

} // namespace loonitunes
