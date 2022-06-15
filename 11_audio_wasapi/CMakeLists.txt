if(WIN32)
    file(GLOB_RECURSE AM_SOURCES *.cpp)

    add_executable(audio_wasapi ${AM_SOURCES})
    target_link_libraries(audio_wasapi PRIVATE ${LIBS})

    target_include_directories(audio_wasapi
        PRIVATE
            ${PROJECT_SOURCE_DIR}/3rdparty
            ${PROJECT_SOURCE_DIR}/utils
            ${PROJECT_SOURCE_DIR}/11_audio_wasapi
    )
endif(WIN32)