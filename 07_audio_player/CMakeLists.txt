file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

add_executable(audio_player ${MEDIA_PLAYER_SOURCES})
target_link_libraries(audio_player PRIVATE ${LIBS})

target_include_directories(audio_player
    PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        ${PROJECT_SOURCE_DIR}/utils
        ${PROJECT_SOURCE_DIR}/07_audio_player
)