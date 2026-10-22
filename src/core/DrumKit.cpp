/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "DrumKit.h"

#include <juce_audio_formats/juce_audio_formats.h>

namespace loonitunes
{

namespace
{
    constexpr float twoPi = juce::MathConstants<float>::twoPi;

    float decay (float t, float timeConstant)
    {
        return std::exp (-t / timeConstant);
    }

    juce::AudioBuffer<float> makeBuffer (double seconds)
    {
        juce::AudioBuffer<float> buffer (1, juce::roundToInt (seconds * DrumKit::sampleRate));
        buffer.clear();
        return buffer;
    }

    void filter (juce::AudioBuffer<float>& buffer, const juce::IIRCoefficients& coefficients)
    {
        juce::IIRFilter f;
        f.setCoefficients (coefficients);
        f.processSamples (buffer.getWritePointer (0), buffer.getNumSamples());
    }

    // A short fade at the end stops clicks when the sampler cuts the sound.
    void finish (juce::AudioBuffer<float>& buffer)
    {
        const int fade = std::min (buffer.getNumSamples(), juce::roundToInt (0.005 * DrumKit::sampleRate));
        buffer.applyGainRamp (buffer.getNumSamples() - fade, fade, 1.0f, 0.0f);
        buffer.setSample (0, buffer.getNumSamples() - 1, 0.0f);

        if (auto peak = buffer.getMagnitude (0, buffer.getNumSamples()); peak > 0.0f)
            buffer.applyGain (juce::Decibels::decibelsToGain (-1.0f) / peak);
    }

    juce::AudioBuffer<float> kick()
    {
        auto buffer = makeBuffer (0.5);
        auto* d = buffer.getWritePointer (0);
        juce::Random random (1);
        float phase = 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto t = float (i / DrumKit::sampleRate);
            const auto frequency = 48.0f + 112.0f * decay (t, 0.035f);
            phase += twoPi * frequency / float (DrumKit::sampleRate);

            const auto body = std::sin (phase) * decay (t, 0.22f);
            const auto click = (random.nextFloat() * 2.0f - 1.0f) * decay (t, 0.0015f) * 0.4f;
            d[i] = std::tanh (1.5f * (body + click));
        }

        finish (buffer);
        return buffer;
    }

    juce::AudioBuffer<float> snare()
    {
        auto noise = makeBuffer (0.3);
        juce::Random random (2);

        for (int i = 0; i < noise.getNumSamples(); ++i)
            noise.setSample (0, i, random.nextFloat() * 2.0f - 1.0f);

        filter (noise, juce::IIRCoefficients::makeHighPass (DrumKit::sampleRate, 1500.0));

        auto buffer = makeBuffer (0.3);
        auto* d = buffer.getWritePointer (0);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto t = float (i / DrumKit::sampleRate);
            const auto tone = std::sin (twoPi * 185.0f * t) * decay (t, 0.05f) * 0.6f;
            d[i] = tone + noise.getSample (0, i) * decay (t, 0.08f);
        }

        finish (buffer);
        return buffer;
    }

    juce::AudioBuffer<float> hat()
    {
        auto buffer = makeBuffer (0.09);
        auto* d = buffer.getWritePointer (0);
        juce::Random random (3);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            d[i] = (random.nextFloat() * 2.0f - 1.0f) * decay (float (i / DrumKit::sampleRate), 0.018f);

        filter (buffer, juce::IIRCoefficients::makeHighPass (DrumKit::sampleRate, 7000.0));
        finish (buffer);
        return buffer;
    }

    juce::AudioBuffer<float> clap()
    {
        auto buffer = makeBuffer (0.35);
        auto* d = buffer.getWritePointer (0);
        juce::Random random (4);
        const float burstStarts[] = { 0.0f, 0.011f, 0.022f };

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto t = float (i / DrumKit::sampleRate);
            float envelope = 0.0f;

            for (auto start : burstStarts)
                if (t >= start)
                    envelope = std::max (envelope, decay (t - start, 0.006f));

            if (t >= burstStarts[2])
                envelope = std::max (envelope, 0.6f * decay (t - burstStarts[2], 0.11f));

            d[i] = (random.nextFloat() * 2.0f - 1.0f) * envelope;
        }

        filter (buffer, juce::IIRCoefficients::makeBandPass (DrumKit::sampleRate, 1300.0, 1.2));
        finish (buffer);
        return buffer;
    }
}

juce::AudioBuffer<float> DrumKit::render (DrumSound sound)
{
    switch (sound)
    {
        case DrumSound::kick:   return kick();
        case DrumSound::snare:  return snare();
        case DrumSound::hat:    return hat();
        case DrumSound::clap:   return clap();
    }

    jassertfalse;
    return {};
}

juce::String DrumKit::getFileName (DrumSound sound)
{
    switch (sound)
    {
        case DrumSound::kick:   return "Kick.wav";
        case DrumSound::snare:  return "Snare.wav";
        case DrumSound::hat:    return "Hat.wav";
        case DrumSound::clap:   return "Clap.wav";
    }

    jassertfalse;
    return {};
}

juce::Array<juce::File> DrumKit::writeToFolder (const juce::File& folder)
{
    if (! folder.createDirectory())
        return {};

    juce::Array<juce::File> files;
    juce::WavAudioFormat wav;

    for (int i = 0; i < numDrumSounds; ++i)
    {
        const auto sound = static_cast<DrumSound> (i);
        const auto buffer = render (sound);
        const auto file = folder.getChildFile (getFileName (sound));
        file.deleteFile();

        std::unique_ptr<juce::OutputStream> stream (file.createOutputStream());

        if (stream == nullptr)
            return {};

        const auto options = juce::AudioFormatWriterOptions{}
                                 .withSampleRate (sampleRate)
                                 .withNumChannels (1)
                                 .withBitsPerSample (24);

        auto writer = wav.createWriterFor (stream, options);

        if (writer == nullptr || ! writer->writeFromAudioSampleBuffer (buffer, 0, buffer.getNumSamples()))
            return {};

        files.add (file);
    }

    return files;
}

} // namespace loonitunes
