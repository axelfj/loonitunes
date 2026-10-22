/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

namespace loonitunes
{

/** The built-in drum sounds. They are synthesised in code, so the kit is ours
    and ships under the same licence as the rest of Loonitunes.
*/
enum class DrumSound
{
    kick,
    snare,
    hat,
    clap
};

constexpr int numDrumSounds = 4;

struct DrumKit
{
    static constexpr double sampleRate = 44100.0;

    /** Renders one drum hit as a mono buffer, peak-normalised to -1 dBFS. */
    static juce::AudioBuffer<float> render (DrumSound);

    /** Writes every drum to `folder` as a 24-bit WAV and returns the files in
        DrumSound order. Returns an empty array if a file could not be written.
    */
    static juce::Array<juce::File> writeToFolder (const juce::File& folder);

    static juce::String getFileName (DrumSound);
};

} // namespace loonitunes
