if(WIN32)
    file(GLOB_RECURSE AM_SOURCES *.cpp)

    add_executable(wasapi_render ${AM_SOURCES})
    target_link_libraries(wasapi_render PRIVATE ${LIBS})

    target_include_directories(wasapi_render
        PRIVATE
            ${PROJECT_SOURCE_DIR}/3rdparty
            ${PROJECT_SOURCE_DIR}/utils
            ${PROJECT_SOURCE_DIR}/12_wasapi_render
    )
endif(WIN32)