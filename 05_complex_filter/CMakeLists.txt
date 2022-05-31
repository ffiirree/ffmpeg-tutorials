file(GLOB_RECURSE MEDIA_PLAYER_SOURCES *.cpp)

add_executable(complex_filter ${MEDIA_PLAYER_SOURCES})
target_link_libraries(complex_filter PRIVATE ${LIBS})

target_include_directories(complex_filter
    PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        ${PROJECT_SOURCE_DIR}/utils
)