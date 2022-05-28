file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

list(APPEND MEDIA_PLAYER_SOURCES $<$<PLATFORM_ID:Windows>:ico_win32.rc>)

add_executable(video_player ${MEDIA_PLAYER_SOURCES})
target_link_libraries(video_player PRIVATE ${LIBS})

target_include_directories(video_player
    PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        core
        player
)