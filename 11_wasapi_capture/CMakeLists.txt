if(WIN32)
    file(GLOB_RECURSE AM_SOURCES *.cpp)

    add_executable(wasapi_capture ${AM_SOURCES})
    target_link_libraries(wasapi_capture PRIVATE ${LIBS})

    target_include_directories(wasapi_capture
        PRIVATE
            ${PROJECT_SOURCE_DIR}/3rdparty
            ${PROJECT_SOURCE_DIR}/utils
            ${PROJECT_SOURCE_DIR}/11_wasapi_capture
    )
endif(WIN32)