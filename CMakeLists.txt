cmake_minimum_required(VERSION 3.16)

project(ffmpeg_examples VERSION 0.0.2 DESCRIPTION "ffmpeg tutorial & examples" LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Default build type: Release." FORCE)
endif()

if(MSVC)
    message(STATUS "Windows SDK version is ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")

    add_compile_options(/W3 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /Zc:preprocessor  /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(DISABLE_WGC "Disable the Windows Graphics Capture Example" OFF)

# #######################################################################################################################
# dependencies
# #######################################################################################################################
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# 3rdparty: glog fmt
set(BUILD_SHARED_LIBS OFF CACHE BOOL "build static libraries.")
set(WITH_GFLAGS OFF CACHE BOOL "Use gflags")
add_subdirectory(3rdparty/glog EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/fmt EXCLUDE_FROM_ALL)

# Qt5
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Gui Multimedia REQUIRED)

if (DISABLE_WGC)
    find_package(FFmpeg 4.3 REQUIRED)
else()
    find_package(FFmpeg 6 REQUIRED)
endif()

if(UNIX AND NOT APPLE)
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
        ffmpeg::ffmpeg
        $<$<PLATFORM_ID:Linux>:Qt5::X11Extras>
        $<$<PLATFORM_ID:Windows>:dwmapi>
        $<$<PLATFORM_ID:Windows>:strmiids>
        $<$<PLATFORM_ID:Windows>:ntdll>
        $<$<PLATFORM_ID:Windows>:Shcore>
        $<$<PLATFORM_ID:Windows>:DXGI>
        $<$<PLATFORM_ID:Windows>:D3D11>
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
create_exe(hw_transcoding 13_hwaccel/hw_transcoding.cpp)
create_exe(hw_encoding 13_hwaccel/hw_encoding.cpp)
if(WIN32)
    create_exe(dshow 17_win_dshow/main.cpp)
endif()

add_subdirectory(05_complex_filter)
add_subdirectory(07_audio_player)
add_subdirectory(08_video_player_qt)
add_subdirectory(09_media_player)
add_subdirectory(10_streaming)
add_subdirectory(11_wasapi_capture)
add_subdirectory(12_wasapi_render)
if (DISABLE_WGC)
    message(STATUS "FFMPEG-EXAMPLES: 'Window Graphics Capture' example is disabled.")
elseif(NOT DISABLE_WGC)
    add_subdirectory(14_windows_wgc)
endif()
add_subdirectory(15_linux_pulse)
add_subdirectory(16_linux_v4l2)