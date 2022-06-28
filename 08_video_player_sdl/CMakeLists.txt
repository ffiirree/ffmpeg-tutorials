file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

add_executable(video_player_sdl ${MEDIA_PLAYER_SOURCES})
target_link_libraries(video_player_sdl PRIVATE ${LIBS} SDL2::SDL2 SDL2::SDL2main)

target_include_directories(video_player_sdl
    PRIVATE
        ${PROJECT_SOURCE_DIR}/utils
        player
)