/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <juce_core/juce_core.h>
#include <cstdio>

namespace loonitunes
{
// Keep the old destination until the writer has closed and validated its output.
// A sibling temporary keeps replacement on the same filesystem. Its destructor
// cleans up only the temporary we own, including on exceptions/failure.
template <typename Writer>
bool writeFileSafely (const juce::File& destination, Writer&& writer)
{
    juce::TemporaryFile temporary (destination);
    if (! writer (temporary.getFile()))
        return false;

   #if JUCE_WINDOWS
    return temporary.overwriteTargetFileWithTemporary();
   #else
    // JUCE's POSIX move helper may fall back to copying after rename fails.
    // Do not allow that fallback to truncate a previously exported file.
    return std::rename (temporary.getFile().getFullPathName().toRawUTF8(),
                        destination.getFullPathName().toRawUTF8()) == 0;
   #endif
}
}
