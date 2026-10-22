cmake_minimum_required (VERSION 3.22)

project (Loonitunes VERSION 0.1.0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

option (LOONITUNES_BUILD_TESTS "Build the headless test runner" ON)

if (MSVC)
    set (CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set (CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "" FORCE)
endif()

# JUCE first, so Tracktion Engine picks it up instead of its own submodule.
add_subdirectory (external/JUCE)
add_subdirectory (external/tracktion_engine/modules)

#==============================================================================
# loonitunes_core: everything that is not a window. Shared by the app and tests,
# so the (large) engine is compiled once.
add_library (loonitunes_core STATIC
    src/core/DrumKit.cpp
    src/core/Song.cpp)

target_include_directories (loonitunes_core PUBLIC src)

target_link_libraries (loonitunes_core
    PRIVATE
        tracktion::tracktion_core
        tracktion::tracktion_engine
        tracktion::tracktion_graph
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

target_compile_definitions (loonitunes_core
    PUBLIC
        JUCE_STANDALONE_APPLICATION=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_AU=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1
        LOONITUNES_VERSION="${PROJECT_VERSION}"
    INTERFACE
        $<TARGET_PROPERTY:loonitunes_core,COMPILE_DEFINITIONS>)

target_include_directories (loonitunes_core
    INTERFACE
        $<TARGET_PROPERTY:loonitunes_core,INCLUDE_DIRECTORIES>)

set_target_properties (loonitunes_core PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
    VISIBILITY_INLINES_HIDDEN TRUE
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden)

if (APPLE)
    target_compile_options (loonitunes_core PUBLIC "-fno-aligned-allocation")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries (loonitunes_core PUBLIC atomic)
endif()

#==============================================================================
juce_add_gui_app (Loonitunes
    PRODUCT_NAME "Loonitunes"
    COMPANY_NAME "ROMMUSER Studio"
    BUNDLE_ID "com.rommuser.loonitunes"
    VERSION "${PROJECT_VERSION}"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Loonitunes usa el micrófono solo si grabás audio. / Loonitunes only uses the microphone when you record audio."
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE)

target_sources (Loonitunes PRIVATE
    src/app/Main.cpp
    src/app/MainComponent.cpp
    src/app/StepGrid.cpp)

target_link_libraries (Loonitunes PRIVATE
    loonitunes_core
    juce::juce_recommended_warning_flags)

#==============================================================================
if (LOONITUNES_BUILD_TESTS)
    enable_testing()

    juce_add_console_app (LoonitunesTests PRODUCT_NAME "LoonitunesTests")
    target_sources (LoonitunesTests PRIVATE tests/TestMain.cpp)
    target_link_libraries (LoonitunesTests PRIVATE
        loonitunes_core
        juce::juce_recommended_warning_flags)

    add_test (NAME LoonitunesTests COMMAND LoonitunesTests)
endif()
