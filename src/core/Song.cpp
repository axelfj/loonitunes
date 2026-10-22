/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "Song.h"

#include <tracktion_engine/tracktion_engine.h>

namespace te = tracktion;
using namespace std::literals;

namespace loonitunes
{

namespace
{
    // General MIDI drum notes, so a pattern can later drive any drum plugin.
    constexpr int noteNumbers[numDrumSounds] = { 36, 38, 42, 39 };
    const char* const channelNames[numDrumSounds] = { "Kick", "Snare", "Hat", "Clap" };

    te::TimeRange songRange (te::Edit& edit)
    {
        return { 0s, edit.tempoSequence.toTime ({ Song::numBars, te::BeatDuration() }) };
    }
}

Song::Song (te::Engine& e, const juce::File& workFolder)
    : engine (e), edit (te::Edit::createSingleTrackEdit (e))
{
    edit->tempoSequence.getTempos()[0]->setBpm (defaultTempo);

    auto track = te::getAudioTracks (*edit)[0];
    track->setName ("Drums");

    auto clip = dynamic_cast<te::StepClip*> (track->insertNewClip (te::TrackItem::Type::step, "Pattern 1",
                                                                   songRange (*edit), nullptr));

    if (clip == nullptr)
        return;

    clip->repeatSequence = true;

    while (clip->getChannels().size() > numDrumSounds)
        clip->removeChannel (clip->getChannels().size() - 1);

    while (clip->getChannels().size() < numDrumSounds)
        clip->insertNewChannel (-1);

    clip->getPattern (0).setNumNotes (numSteps);
    clip->getPattern (0).clear();

    const auto files = DrumKit::writeToFolder (workFolder.getChildFile ("kit"));

    if (files.size() != numDrumSounds)
        return;

    auto sampler = dynamic_cast<te::SamplerPlugin*> (edit->getPluginCache().createNewPlugin (te::SamplerPlugin::xmlTypeName, {}).get());

    if (sampler == nullptr)
        return;

    track->pluginList.insertPlugin (*sampler, 0, nullptr);

    for (int i = 0; i < numDrumSounds; ++i)
    {
        auto channel = clip->getChannels()[i];
        channel->noteNumber = noteNumbers[i];
        channel->name = channelNames[i];

        const auto error = sampler->addSound (files[i].getFullPathName(), channelNames[i], 0.0, 0.0, 0.0f);

        if (error.isNotEmpty())
            return;

        sampler->setSoundParams (sampler->getNumSounds() - 1, noteNumbers[i], noteNumbers[i], noteNumbers[i]);
    }

    // The sampler loads its sounds asynchronously; load them now so the first
    // Play or export already has audio.
    sampler->flushPendingUpdates();

    auto& transport = edit->getTransport();
    transport.setLoopRange (clip->getEditTimeRange());
    transport.looping = true;

    ready = true;
}

Song::~Song()
{
    if (edit != nullptr)
        edit->getTransport().stop (false, true);
}

te::StepClip* Song::getClip() const
{
    if (auto track = te::getAudioTracks (*edit)[0])
        return dynamic_cast<te::StepClip*> (track->getClips()[0]);

    return nullptr;
}

//==============================================================================
int Song::getNumChannels() const
{
    if (auto clip = getClip())
        return clip->getChannels().size();

    return 0;
}

juce::String Song::getChannelName (int channel) const
{
    if (auto clip = getClip())
        if (auto c = clip->getChannels()[channel])
            return c->name.get();

    return {};
}

bool Song::getStep (int channel, int step) const
{
    if (auto clip = getClip())
        return clip->getPattern (0).getNote (channel, step);

    return false;
}

void Song::setStep (int channel, int step, bool on)
{
    if (! juce::isPositiveAndBelow (channel, getNumChannels()) || ! juce::isPositiveAndBelow (step, numSteps))
        return;

    if (auto clip = getClip())
        clip->getPattern (0).setNote (channel, step, on);
}

void Song::toggleStep (int channel, int step)
{
    setStep (channel, step, ! getStep (channel, step));
}

void Song::clearPattern()
{
    if (auto clip = getClip())
        clip->getPattern (0).clear();
}

void Song::loadStarterBeat()
{
    clearPattern();

    for (int step = 0; step < numSteps; ++step)
    {
        setStep (0, step, step % 4 == 0);              // kick on every beat
        setStep (1, step, step == 4 || step == 12);    // snare on 2 and 4
        setStep (2, step, step % 4 == 2);              // offbeat hats
    }

    setStep (3, 12, true);                             // clap doubles the last snare
}

//==============================================================================
double Song::getTempo() const
{
    return edit->tempoSequence.getTempos()[0]->getBpm();
}

void Song::setTempo (double bpm)
{
    edit->tempoSequence.getTempos()[0]->setBpm (juce::jlimit (minTempo, maxTempo, bpm));

    // Keep the pattern exactly numBars long at the new tempo.
    if (auto clip = getClip())
    {
        clip->setPosition ({ songRange (*edit), {} });
        edit->getTransport().setLoopRange (clip->getEditTimeRange());
    }
}

void Song::play()
{
    edit->getTransport().play (false);
}

void Song::stop()
{
    edit->getTransport().stop (false, false);
}

bool Song::isPlaying() const
{
    return edit->getTransport().isPlaying();
}

int Song::getPlayingStep() const
{
    auto& transport = edit->getTransport();

    if (! transport.isPlaying())
        return -1;

    const auto beat = edit->tempoSequence.toBeats (transport.getPosition()).inBeats();
    return juce::jlimit (0, numSteps - 1, int (std::floor (beat * 4.0)) % numSteps);
}

//==============================================================================
bool Song::exportWav (const juce::File& destination)
{
    stop();
    destination.deleteFile();

    juce::BigInteger tracks;
    tracks.setRange (0, te::getAllTracks (*edit).size(), true);

    return te::Renderer::renderToFile ("Export", destination, *edit, songRange (*edit), tracks,
                                       true, false, {}, false);
}

} // namespace loonitunes
