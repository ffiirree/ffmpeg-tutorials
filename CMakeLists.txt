cmake_minimum_required(VERSION 3.16)

project(ffmpeg_examples VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

# #######################################################################################################################
# dependencies
# #######################################################################################################################
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/;${CMAKE_MODULE_PATH}")

# 3rdparty: glog fmt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "build static libraries.")
set(WITH_GFLAGS OFF CACHE BOOL "Use gflags")
add_subdirectory(3rdparty/glog EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/fmt EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/SDL EXCLUDE_FROM_ALL)

# Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Gui Multimedia REQUIRED)
find_package(FFmpeg COMPONENTS avutil avcodec avformat avdevice avfilter swscale swresample REQUIRED)

if(WIN32)
    # GUI/console
    set(CMAKE_WIN32_EXECUTABLE ON)

    # Windows API
    add_definitions(-DUNICODE -DNOMINMAX)
elseif(UNIX AND NOT APPLE)
    find_package(Qt5 COMPONENTS X11Extras REQUIRED)
endif()

# #######################################################################################################################
# Create the executable files
# #######################################################################################################################
add_definitions(-D__STDC_CONSTANT_MACROS)

list(
    APPEND
        LIBS
        Qt5::Widgets Qt5::Gui Qt5::Multimedia
        glog::glog fmt::fmt
        FFmpeg::avutil FFmpeg::avcodec FFmpeg::avformat FFmpeg::avdevice FFmpeg::avfilter FFmpeg::swscale FFmpeg::swresample
        $<$<PLATFORM_ID:Linux>:Qt5::X11Extras>
        $<$<PLATFORM_ID:Windows>:strmiids>
)

function(create_exe name source_file)
    add_executable(${name} ${source_file})
    target_link_libraries(${name} PRIVATE ${LIBS})
    target_include_directories(${name} PRIVATE utils)
endfunction(create_exe)

create_exe(remuxing 01_remuxing/remuxing.cpp)
create_exe(transcoding 02_transcoding/transcoding.cpp)
create_exe(recording 03_recording/recording.cpp)
create_exe(recording_mic 03_recording/recording_mic.cpp)
create_exe(filter 04_simple_filter/filter.cpp)
create_exe(cuda_decode 13_hwaccel/cuda_decode.cpp)
create_exe(dshow 17_win_dshow/main.cpp)

add_subdirectory(05_complex_filter)
add_subdirectory(07_audio_player)
add_subdirectory(08_video_player_qt)
add_subdirectory(08_video_player_sdl)
add_subdirectory(09_media_player)
add_subdirectory(10_streaming)
add_subdirectory(11_wasapi_capture)
add_subdirectory(12_wasapi_render)
add_subdirectory(15_linux_pulse)
add_subdirectory(16_linux_v4l2)