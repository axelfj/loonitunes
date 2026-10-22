/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "DrumKit.h"

namespace tracktion::inline engine
{
    class Engine;
    class Edit;
    class StepClip;
}

namespace loonitunes
{

/** One song: a 16-step drum pattern (the Channel Rack) that loops for
    `numBars` bars. Wraps a Tracktion Edit so the UI never touches the engine.
*/
class Song
{
public:
    static constexpr int numSteps = 16;
    static constexpr int numBars = 4;
    static constexpr double minTempo = 60.0, maxTempo = 200.0, defaultTempo = 120.0;

    /** `workFolder` holds the generated drum kit and temporary files. */
    Song (tracktion::Engine&, const juce::File& workFolder);
    ~Song();

    /** False if the drum kit could not be written or loaded. */
    bool isReady() const noexcept               { return ready; }

    tracktion::Edit& getEdit() noexcept         { return *edit; }

    //==============================================================================
    int getNumChannels() const;
    juce::String getChannelName (int channel) const;

    bool getStep (int channel, int step) const;
    void setStep (int channel, int step, bool on);
    void toggleStep (int channel, int step);

    void clearPattern();
    /** A simple four-on-the-floor beat so the first Play already sounds good. */
    void loadStarterBeat();

    //==============================================================================
    double getTempo() const;
    void setTempo (double bpm);

    void play();
    void stop();
    bool isPlaying() const;

    /** The step under the playhead, or -1 when stopped. */
    int getPlayingStep() const;

    //==============================================================================
    /** Renders all `numBars` bars to a stereo WAV. Blocks until done. */
    bool exportWav (const juce::File& destination);

private:
    tracktion::StepClip* getClip() const;

    tracktion::Engine& engine;
    std::unique_ptr<tracktion::Edit> edit;
    bool ready = false;

    JUCE_DECLARE_NON_COPYABLE (Song)
};

} // namespace loonitunes
