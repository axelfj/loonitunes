/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later

    Headless checks: no window or sound card needed, so they run in CI.
*/

#include "core/DrumKit.h"
#include "core/Song.h"

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>
#include <tracktion_engine/tracktion_engine.h>

#include <iostream>

using namespace loonitunes;

namespace
{
    int failures = 0;

    void check (bool condition, const juce::String& what)
    {
        std::cout << (condition ? "  ok   " : "  FAIL ") << what << std::endl;

        if (! condition)
            ++failures;
    }

    struct Rendered
    {
        juce::AudioBuffer<float> audio;
        double sampleRate = 0.0;
    };

    Rendered readWav (const juce::File& file)
    {
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatReader> reader (wav.createReaderFor (file.createInputStream().release(), true));

        if (reader == nullptr)
            return {};

        Rendered r;
        r.sampleRate = reader->sampleRate;
        r.audio.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&r.audio, 0, (int) reader->lengthInSamples, 0, true, true);
        return r;
    }

    float rmsOf (const juce::AudioBuffer<float>& b, int start, int num)
    {
        return b.getRMSLevel (0, start, num);
    }

    //==============================================================================
    void testDrumKit (const juce::File& folder)
    {
        std::cout << "DrumKit" << std::endl;

        for (int i = 0; i < numDrumSounds; ++i)
        {
            const auto sound = static_cast<DrumSound> (i);
            const auto name = DrumKit::getFileName (sound);
            const auto buffer = DrumKit::render (sound);
            const auto peak = buffer.getMagnitude (0, buffer.getNumSamples());

            check (buffer.getNumSamples() > 1000, name + " has audio");
            check (std::abs (peak - juce::Decibels::decibelsToGain (-1.0f)) < 0.001f, name + " peaks at -1 dBFS");
            check (std::abs (buffer.getSample (0, buffer.getNumSamples() - 1)) < 1.0e-4f, name + " ends silent (no click)");
        }

        const auto a = DrumKit::render (DrumSound::snare);
        const auto b = DrumKit::render (DrumSound::snare);
        check (std::equal (a.getReadPointer (0), a.getReadPointer (0) + a.getNumSamples(), b.getReadPointer (0)),
               "sounds are identical every time (fixed seed)");

        const auto files = DrumKit::writeToFolder (folder.getChildFile ("kit-test"));
        check (files.size() == numDrumSounds, "writes 4 WAV files");

        if (files.size() == numDrumSounds)
        {
            const auto kick = readWav (files[0]);
            check (juce::approximatelyEqual (kick.sampleRate, DrumKit::sampleRate) && kick.audio.getNumSamples() > 0, "kick WAV reads back");
        }
    }

    //==============================================================================
    void testPattern (Song& song)
    {
        std::cout << "Pattern" << std::endl;

        check (song.isReady(), "song loads the drum kit into the sampler");
        check (song.getNumChannels() == 4, "Channel Rack has 4 channels");
        check (song.getChannelName (0) == "Kick" && song.getChannelName (3) == "Clap", "channels are named");

        song.clearPattern();
        check (! song.getStep (0, 0), "cleared pattern is empty");

        song.toggleStep (1, 5);
        check (song.getStep (1, 5), "toggle turns a step on");
        song.toggleStep (1, 5);
        check (! song.getStep (1, 5), "toggle turns it off again");

        song.setStep (9, 0, true);
        song.setStep (0, 99, true);
        check (true, "out-of-range steps are ignored without crashing");

        song.loadStarterBeat();
        check (song.getStep (0, 0) && song.getStep (0, 4) && song.getStep (0, 8) && song.getStep (0, 12), "starter beat: kick on every beat");
        check (song.getStep (1, 4) && song.getStep (1, 12) && ! song.getStep (1, 0), "starter beat: snare on 2 and 4");

        song.setTempo (500.0);
        check (juce::approximatelyEqual (song.getTempo(), Song::maxTempo), "tempo is clamped to the maximum");
        song.setTempo (10.0);
        check (juce::approximatelyEqual (song.getTempo(), Song::minTempo), "tempo is clamped to the minimum");
        song.setTempo (140.0);
        check (std::abs (song.getTempo() - 140.0) < 1.0e-9, "tempo can be set");
        check (song.getPlayingStep() == -1, "no playing step while stopped");
    }

    //==============================================================================
    void testExport (Song& song, const juce::File& folder)
    {
        std::cout << "Export" << std::endl;

        song.setTempo (120.0);
        song.loadStarterBeat();

        const auto file = folder.getChildFile ("beat.wav");
        check (song.exportWav (file), "renders without error");

        const auto r = readWav (file);
        const auto expectedSeconds = Song::numBars * 4 * 60.0 / 120.0;   // 4 bars of 4/4 at 120 BPM = 8 s
        const auto seconds = r.audio.getNumSamples() / std::max (1.0, r.sampleRate);
        check (std::abs (seconds - expectedSeconds) < 0.1, "export is 4 bars long (" + juce::String (seconds, 2) + " s)");
        check (r.audio.getNumChannels() == 2, "export is stereo");

        const auto peak = r.audio.getMagnitude (0, r.audio.getNumSamples());
        check (peak > 0.1f, "export is not silent (peak " + juce::String (peak, 3) + ")");
        check (peak <= 1.0f, "export does not clip");

        // Every bar should sound, which proves the pattern repeats.
        const auto barLength = r.audio.getNumSamples() / Song::numBars;
        bool allBarsSound = barLength > 0;

        for (int bar = 0; bar < Song::numBars && allBarsSound; ++bar)
            allBarsSound = rmsOf (r.audio, bar * barLength, barLength) > 0.01f;

        check (allBarsSound, "all 4 bars have drums (pattern loops)");

        // A kick on step 0 should land at the very start, within 5 ms.
        const int window = juce::roundToInt (0.005 * r.sampleRate);
        check (window > 0 && r.audio.getMagnitude (0, 0, window) > 0.05f, "first kick is on time");

        song.clearPattern();
        const auto silentFile = folder.getChildFile ("silent.wav");
        song.exportWav (silentFile);
        const auto s = readWav (silentFile);
        check (s.audio.getNumSamples() == 0 || s.audio.getMagnitude (0, s.audio.getNumSamples()) < 1.0e-4f,
               "empty pattern exports silence");
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;

    const auto folder = juce::File::getSpecialLocation (juce::File::tempDirectory)
                            .getChildFile ("LoonitunesTests-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    folder.createDirectory();

    {
        tracktion::Engine engine ("LoonitunesTests");
        Song song (engine, folder);

        testDrumKit (folder);
        testPattern (song);
        testExport (song, folder);
    }

    folder.deleteRecursively();

    std::cout << (failures == 0 ? "All tests passed" : juce::String (failures) + " test(s) failed") << std::endl;
    return failures == 0 ? 0 : 1;
}
