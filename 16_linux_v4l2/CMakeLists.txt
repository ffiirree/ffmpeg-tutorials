if(UNIX AND NOT APPLE)
    find_package(Libv4l2 REQUIRED)

    file(GLOB_RECURSE V4L2_SOURCES *.cpp)

    add_executable(v4l2 ${V4L2_SOURCES})
    target_link_libraries(v4l2 PRIVATE ${LIBS} Libv4l2::Libv4l2)

    target_include_directories(v4l2
        PRIVATE
        ${PROJECT_SOURCE_DIR}/3rdparty
        ${PROJECT_SOURCE_DIR}/utils
        ${PROJECT_SOURCE_DIR}/16_linux_v4l2
    )
endif(UNIX AND NOT APPLE)