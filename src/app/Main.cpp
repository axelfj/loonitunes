/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "MainComponent.h"

#include <tracktion_engine/tracktion_engine.h>

namespace loonitunes
{

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow (Song& song)
        : DocumentWindow ("Loonitunes", juce::Colour (0xff1c1c26), DocumentWindow::allButtons)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (new MainComponent (song), true);
        setResizable (true, true);
        setResizeLimits (820, 420, 4000, 2000);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class Application : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return "Loonitunes"; }
    const juce::String getApplicationVersion() override { return LOONITUNES_VERSION; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        const auto workFolder = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                    .getChildFile ("Loonitunes");

        engine = std::make_unique<tracktion::Engine> ("Loonitunes");
        song = std::make_unique<Song> (*engine, workFolder);
        window = std::make_unique<MainWindow> (*song);
    }

    void shutdown() override
    {
        window = nullptr;
        song = nullptr;
        engine = nullptr;
    }

private:
    std::unique_ptr<tracktion::Engine> engine;
    std::unique_ptr<Song> song;
    std::unique_ptr<MainWindow> window;
};

} // namespace loonitunes

START_JUCE_APPLICATION (loonitunes::Application)
