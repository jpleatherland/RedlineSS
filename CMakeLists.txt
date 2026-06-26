cmake_minimum_required(VERSION 3.22)

project(RedlineSS VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(RedlineSS
    COMPANY_NAME "Jack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Jlag
    PLUGIN_CODE Mfas
    FORMATS VST3 Standalone
    PRODUCT_NAME "RedlineSS"
)

target_sources(RedlineSS
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/AmpPreamp/AmpPreamp.h
        Source/AmpPreamp/AmpPreamp.cpp
        Source/AmpPreamp/InputConditioning.h
        Source/AmpPreamp/InputConditioning.cpp
        Source/AmpPreamp/LeadPreGain.h
        Source/AmpPreamp/LeadPreGain.cpp
        Source/AmpPreamp/Q5Q6GainStage.h
        Source/AmpPreamp/Q5Q6GainStage.cpp
        Source/AmpPreamp/Q7Q8GainStage.h
        Source/AmpPreamp/Q7Q8GainStage.cpp
        Source/AmpPreamp/BiasedClamp.h
        Source/AmpPreamp/BiasedClamp.cpp
        Source/AmpPreamp/RecoveryStage.h
        Source/AmpPreamp/RecoveryStage.cpp
        Source/AmpPreamp/Q11DriverStage.h
        Source/AmpPreamp/Q11DriverStage.cpp
        # Source/AmpPreamp/GainStage1.cpp
        # Source/AmpPreamp/GainStage1.h
        # Source/AmpPreamp/GainStage2.cpp
        # Source/AmpPreamp/GainStage2.h
        # Source/AmpPreamp/GainStage3.cpp
        # Source/AmpPreamp/GainStage3.h
        # Source/AmpPreamp/ToneStack.h
        # Source/AmpPreamp/ToneStack.cpp
        # Source/AmpPowerAmp/AmpPowerAmp.h
        # Source/AmpPowerAmp/AmpPowerAmp.cpp
        # Source/Utility/NoiseGate.h
        # Source/Utility/NoiseGate.cpp
        # Source/AmpPreamp/VintageLeadPreamp.h
        # Source/AmpPreamp/VintageLeadPreamp.cpp
        # Source/AmpPreamp/ModernLeadPreamp.h
        # Source/AmpPreamp/ModernLeadPreamp.cpp
)

target_compile_definitions(RedlineSS
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RedlineSS
    PRIVATE
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_gui_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
