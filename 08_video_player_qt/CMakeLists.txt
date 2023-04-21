file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

add_executable(video_player WIN32 ${MEDIA_PLAYER_SOURCES})
target_link_libraries(video_player PRIVATE ${LIBS})

target_include_directories(video_player
    PRIVATE
        ${PROJECT_SOURCE_DIR}/utils
        player
)