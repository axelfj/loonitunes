/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "core/Song.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace loonitunes
{

/** The Channel Rack: one row of 16 pads per drum. Click a pad to toggle it,
    drag across pads to paint or erase several at once.
*/
class StepGrid : public juce::Component,
                 private juce::Timer
{
public:
    explicit StepGrid (Song&);

    static juce::Colour getRowColour (int row);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    juce::Rectangle<float> getPadArea() const;
    juce::Rectangle<float> getPadBounds (int row, int step) const;
    bool getPadAt (juce::Point<float>, int& row, int& step) const;

    Song& song;
    int playingStep = -1;
    bool paintValue = true;
    int lastRow = -1, lastStep = -1;
};

} // namespace loonitunes
