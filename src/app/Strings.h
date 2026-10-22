/*
    Loonitunes - free open-source DAW
    Copyright (C) 2026 ROMMUSER Studio
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <juce_core/juce_core.h>

namespace loonitunes
{

/** Every word on screen, in Spanish and English. Kept in one place so adding a
    language is a single edit.
*/
enum class Language { spanish, english };

enum class Text
{
    play,
    stop,
    tempo,
    starterBeat,
    clear,
    exportWav,
    exportTitle,
    exportDone,
    exportFailed,
    hint,
    otherLanguage,
    kitFailed
};

inline juce::String tr (Text text, Language language)
{
    const bool es = language == Language::spanish;

    switch (text)
    {
        case Text::play:          return es ? "Play" : "Play";
        case Text::stop:          return es ? "Stop" : "Stop";
        case Text::tempo:         return "Tempo";
        case Text::starterBeat:   return es ? "Beat de ejemplo" : "Starter beat";
        case Text::clear:         return es ? "Borrar todo" : "Clear all";
        case Text::exportWav:     return es ? "Exportar WAV" : "Export WAV";
        case Text::exportTitle:   return es ? "Guardar tu beat como WAV" : "Save your beat as WAV";
        case Text::exportDone:    return es ? juce::String::fromUTF8 ("Listo. Tu beat se guardó en:") : "Done. Your beat was saved to:";
        case Text::exportFailed:  return es ? juce::String ("No se pudo exportar. Prueba otra carpeta.") : "Export failed. Try another folder.";
        case Text::hint:          return es ? "Toca los cuadros para armar tu beat. Barra espaciadora = Play/Stop."
                                            : "Click the squares to build your beat. Space bar = Play/Stop.";
        case Text::otherLanguage: return es ? juce::String ("English") : juce::String::fromUTF8 ("Español");
        case Text::kitFailed:     return es ? "No se pudo cargar el kit de drums."
                                            : "The drum kit could not be loaded.";
    }

    jassertfalse;
    return {};
}

} // namespace loonitunes
