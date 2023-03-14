if(UNIX AND NOT APPLE)
    file(GLOB_RECURSE PULSE_SOURCES *.cpp)

    add_executable(pulseaudio ${PULSE_SOURCES})
    target_link_libraries(pulseaudio PRIVATE ${LIBS})

    target_include_directories(pulseaudio
        PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        ${PROJECT_SOURCE_DIR}/utils
        ${PROJECT_SOURCE_DIR}/15_linux_pulse
    )
endif(UNIX AND NOT APPLE)