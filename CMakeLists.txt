cmake_minimum_required(VERSION 3.12)

project(ffmpeg_examples VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

########################################################################################################################
# dependencies
########################################################################################################################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

find_package(FFmpeg COMPONENTS avutil avcodec avformat avdevice avfilter swscale REQUIRED)

if(WIN32)
    # Windows API
    add_definitions(-DUNICODE -DNOMINMAX)
endif()

########################################################################################################################
# Create the executable files
########################################################################################################################

add_definitions(-D__STDC_CONSTANT_MACROS)

function(create_exe name source_file)
    add_executable(${name} ${source_file})
    target_link_libraries(${name} PRIVATE FFmpeg::avutil FFmpeg::avcodec FFmpeg::avformat FFmpeg::avdevice FFmpeg::avfilter FFmpeg::swscale)
endfunction(create_exe)


create_exe(remuxing 01_remuxing/remuxing.cpp)
create_exe(trancoding 02_trancoding/transcoding.cpp)
create_exe(recording 03_recording/recording.cpp)
create_exe(filtering 04_simple_filters/simple_filters.cpp)