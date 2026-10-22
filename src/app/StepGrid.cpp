/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "StepGrid.h"

namespace loonitunes
{

namespace
{
    constexpr float labelWidth = 96.0f;
    constexpr float gap = 6.0f;
}

StepGrid::StepGrid (Song& s)
    : song (s)
{
    startTimerHz (30);
}

juce::Colour StepGrid::getRowColour (int row)
{
    static const juce::Colour colours[] = {
        juce::Colour (0xffff5a5f),   // kick: red
        juce::Colour (0xffffb238),   // snare: orange
        juce::Colour (0xff3ad4c4),   // hat: teal
        juce::Colour (0xffa77bff)    // clap: purple
    };

    return colours[(size_t) row % std::size (colours)];
}

juce::Rectangle<float> StepGrid::getPadArea() const
{
    return getLocalBounds().toFloat().reduced (gap).withTrimmedLeft (labelWidth);
}

juce::Rectangle<float> StepGrid::getPadBounds (int row, int step) const
{
    const auto area = getPadArea();
    const auto rows = std::max (1, song.getNumChannels());

    // An extra gap every 4 steps makes the beats easy to count.
    const auto groupGaps = 3.0f * gap;
    const auto padWidth = (area.getWidth() - groupGaps - gap * (Song::numSteps - 1)) / Song::numSteps;
    const auto padHeight = (area.getHeight() - gap * float (rows - 1)) / float (rows);

    const auto x = area.getX() + float (step) * (padWidth + gap) + float (step / 4) * gap;
    const auto y = area.getY() + float (row) * (padHeight + gap);

    return { x, y, padWidth, padHeight };
}

bool StepGrid::getPadAt (juce::Point<float> p, int& row, int& step) const
{
    for (int r = 0; r < song.getNumChannels(); ++r)
        for (int s = 0; s < Song::numSteps; ++s)
            if (getPadBounds (r, s).contains (p))
            {
                row = r;
                step = s;
                return true;
            }

    return false;
}

void StepGrid::paint (juce::Graphics& g)
{
    for (int row = 0; row < song.getNumChannels(); ++row)
    {
        const auto colour = getRowColour (row);
        const auto first = getPadBounds (row, 0);

        g.setColour (colour);
        g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        g.drawText (song.getChannelName (row),
                    juce::Rectangle<float> (gap, first.getY(), labelWidth - gap * 2.0f, first.getHeight()),
                    juce::Justification::centredLeft);

        for (int step = 0; step < Song::numSteps; ++step)
        {
            const auto pad = getPadBounds (row, step);
            const bool on = song.getStep (row, step);
            const bool downbeat = step % 4 == 0;

            auto fill = on ? colour : juce::Colour (downbeat ? 0xff3a3a48 : 0xff2c2c38);

            if (step == playingStep)
                fill = on ? colour.brighter (0.6f) : fill.brighter (0.25f);

            g.setColour (fill);
            g.fillRoundedRectangle (pad, 6.0f);

            if (on)
            {
                g.setColour (juce::Colours::white.withAlpha (0.25f));
                g.drawRoundedRectangle (pad.reduced (1.0f), 6.0f, 2.0f);
            }
        }
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    int row = 0, step = 0;

    if (! getPadAt (e.position, row, step))
        return;

    paintValue = ! song.getStep (row, step);
    song.setStep (row, step, paintValue);
    lastRow = row;
    lastStep = step;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    int row = 0, step = 0;

    if (! getPadAt (e.position, row, step) || (row == lastRow && step == lastStep))
        return;

    song.setStep (row, step, paintValue);
    lastRow = row;
    lastStep = step;
    repaint();
}

void StepGrid::timerCallback()
{
    if (const auto step = song.getPlayingStep(); step != playingStep)
    {
        playingStep = step;
        repaint();
    }
}

} // namespace loonitunes
