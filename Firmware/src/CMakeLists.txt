set(OMIPET_SOURCES
    "main.cpp"
    "aht20_sensor.cpp"
    "buzzer.cpp"
    "ics43434_mic.cpp"
    "led_strip.cpp"
    "omi_pet_ui.cpp"
    "pcm_audio_frame_buffer.cpp"
    "voice_controller.cpp"
    "wake_word_detector.cpp"
    "wifi_manager.cpp"
    "../lib/NV3007_Display/NV3007_Display.cpp"
)

idf_component_register(
    SRCS ${OMIPET_SOURCES}
    INCLUDE_DIRS "../include" "../lib/NV3007_Display"
    REQUIRES esp-sr
)
