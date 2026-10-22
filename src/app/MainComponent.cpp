/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "MainComponent.h"

namespace loonitunes
{

namespace
{
    const juce::Colour background (0xff1c1c26);
    const juce::Colour panel (0xff252532);
    const juce::Colour playColour (0xff2ecc71);
    const juce::Colour stopColour (0xffe74c3c);
}

MainComponent::MainComponent (Song& s)
    : song (s)
{
    for (auto* b : { &playButton, &starterButton, &clearButton, &exportButton, &languageButton })
        addAndMakeVisible (b);

    addAndMakeVisible (tempoSlider);
    addAndMakeVisible (tempoLabel);
    addAndMakeVisible (grid);

    playButton.onClick     = [this] { togglePlay(); };
    starterButton.onClick  = [this] { song.loadStarterBeat(); grid.repaint(); };
    clearButton.onClick    = [this] { song.clearPattern(); grid.repaint(); };
    exportButton.onClick   = [this] { exportBeat(); };
    languageButton.onClick = [this]
    {
        language = language == Language::spanish ? Language::english : Language::spanish;
        updateTexts();
    };

    tempoSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 28);
    tempoSlider.setRange (Song::minTempo, Song::maxTempo, 1.0);
    tempoSlider.setTextValueSuffix (" BPM");
    tempoSlider.setValue (song.getTempo(), juce::dontSendNotification);
    tempoSlider.setDoubleClickReturnValue (true, Song::defaultTempo);
    tempoSlider.onDragEnd = [this] { song.setTempo (tempoSlider.getValue()); };
    tempoSlider.onValueChange = [this]
    {
        if (! tempoSlider.isMouseButtonDown())
            song.setTempo (tempoSlider.getValue());
    };

    tempoLabel.setJustificationType (juce::Justification::centredRight);
    tempoLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));

    song.loadStarterBeat();
    updateTexts();
    setWantsKeyboardFocus (true);
    setSize (1100, 520);
    startTimerHz (10);
}

void MainComponent::updateTexts()
{
    const bool playing = song.isPlaying();
    playButton.setButtonText (tr (playing ? Text::stop : Text::play, language));
    playButton.setColour (juce::TextButton::buttonColourId, playing ? stopColour : playColour);

    starterButton.setButtonText (tr (Text::starterBeat, language));
    clearButton.setButtonText (tr (Text::clear, language));
    exportButton.setButtonText (tr (Text::exportWav, language));
    languageButton.setButtonText (tr (Text::otherLanguage, language));
    tempoLabel.setText (tr (Text::tempo, language), juce::dontSendNotification);
    repaint();
}

void MainComponent::togglePlay()
{
    if (song.isPlaying())
        song.stop();
    else
        song.play();

    updateTexts();
}

void MainComponent::exportBeat()
{
    const auto startFolder = juce::File::getSpecialLocation (juce::File::userMusicDirectory);
    chooser = std::make_unique<juce::FileChooser> (tr (Text::exportTitle, language),
                                                   startFolder.getChildFile ("Loonitunes beat.wav"), "*.wav");

    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();

                              if (file == juce::File())
                                  return;

                              file = file.withFileExtension ("wav");
                              const bool ok = song.exportWav (file);
                              updateTexts();

                              juce::AlertWindow::showMessageBoxAsync (
                                  ok ? juce::MessageBoxIconType::InfoIcon : juce::MessageBoxIconType::WarningIcon,
                                  "Loonitunes",
                                  ok ? tr (Text::exportDone, language) + "\n" + file.getFullPathName()
                                     : tr (Text::exportFailed, language));
                          });
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
    {
        togglePlay();
        return true;
    }

    return false;
}

void MainComponent::timerCallback()
{
    // The transport can also stop on its own (e.g. when the audio device changes).
    if (const auto playing = song.isPlaying(); playing != wasPlaying)
    {
        wasPlaying = playing;
        updateTexts();
    }
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (background);

    auto r = getLocalBounds();
    auto header = r.removeFromTop (72);
    g.setColour (panel);
    g.fillRect (header);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText ("Loonitunes", header.removeFromLeft (200).withTrimmedLeft (16), juce::Justification::centredLeft);

    auto footer = r.removeFromBottom (36);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (juce::FontOptions (15.0f));
    g.drawText (song.isReady() ? tr (Text::hint, language) : tr (Text::kitFailed, language),
                footer.reduced (16, 0), juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    auto r = getLocalBounds();
    auto header = r.removeFromTop (72).reduced (12, 14);
    header.removeFromLeft (200);

    languageButton.setBounds (header.removeFromRight (90));
    header.removeFromRight (12);
    exportButton.setBounds (header.removeFromRight (130));
    header.removeFromRight (12);
    clearButton.setBounds (header.removeFromRight (120));
    header.removeFromRight (8);
    starterButton.setBounds (header.removeFromRight (140));
    header.removeFromRight (12);

    playButton.setBounds (header.removeFromLeft (110));
    header.removeFromLeft (12);
    tempoLabel.setBounds (header.removeFromLeft (70));
    tempoSlider.setBounds (header.removeFromLeft (std::min (header.getWidth(), 320)));

    r.removeFromBottom (36);
    grid.setBounds (r.reduced (8));
}

} // namespace loonitunes
