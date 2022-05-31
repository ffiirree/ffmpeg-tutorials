file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

list(APPEND MEDIA_PLAYER_SOURCES $<$<PLATFORM_ID:Windows>:ico_win32.rc>)

add_executable(audio_player ${MEDIA_PLAYER_SOURCES})
target_link_libraries(audio_player PRIVATE ${LIBS})

target_include_directories(audio_player
    PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        ${PROJECT_SOURCE_DIR}/utils
        player
)