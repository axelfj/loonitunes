/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "core/SafeExport.h"
#include <iostream>

int main()
{
    juce::TemporaryFile root ("-safe-export-tests");
    const auto folder = root.getFile();
    if (folder.createDirectory().failed()) return 1;
    const auto target = folder.getChildFile ("beat.wav");
    int failures = 0;
    const auto check = [&] (bool ok, const char* message) {
        std::cout << (ok ? "PASS " : "FAIL ") << message << '\n';
        if (! ok) ++failures;
    };
    check (target.replaceWithText ("original"), "fixture created");
    check (! loonitunes::writeFileSafely (target, [] (const juce::File& temporary) {
        temporary.replaceWithText ("partial render");
        return false;
    }), "render failure returned");
    check (target.loadFileAsString() == "original", "render failure preserves bytes");
    check (folder.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 1,
           "failed render cleans its temporary");
    check (loonitunes::writeFileSafely (target, [] (const juce::File& temporary) {
        return temporary.replaceWithText ("completed render");
    }), "successful replacement returned");
    check (target.loadFileAsString() == "completed render", "success replaces bytes");
    const auto blocked = folder.getChildFile ("directory.wav");
    blocked.createDirectory();
    const auto sentinel = blocked.getChildFile ("keep");
    sentinel.replaceWithText ("keep me");
    check (! loonitunes::writeFileSafely (blocked, [] (const juce::File& temporary) {
        return temporary.replaceWithText ("completed render");
    }), "replacement failure returned");
    check (sentinel.loadFileAsString() == "keep me", "replacement failure preserves destination");
    check (folder.getNumberOfChildFiles (juce::File::findFilesAndDirectories) == 2,
           "replacement failure cleans its temporary");
    return failures == 0 ? 0 : 1;
}
