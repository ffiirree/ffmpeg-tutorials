if(WIN32)
    message(STATUS "FFMPEG-EXAMPLES : Windows API version is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    file(GLOB_RECURSE AM_SOURCES *.cpp)

    add_executable(wgc ${AM_SOURCES})
    target_link_libraries(wgc PRIVATE ${LIBS} dwmapi D3D11)

    target_include_directories(wgc
        PRIVATE
            ${PROJECT_SOURCE_DIR}/3rdparty
            ${PROJECT_SOURCE_DIR}/utils
            ${PROJECT_SOURCE_DIR}/14_windows_mm
    )
endif(WIN32)