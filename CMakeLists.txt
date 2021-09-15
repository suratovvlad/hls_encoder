cmake_minimum_required(VERSION 3.21)
project(hls_encoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED yes)

#set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# FindFFmpeg taken from https://github.com/KDE/ffmpegthumbs/blob/master/cmake/FindFFmpeg.cmake
#find_package(FFmpeg COMPONENTS AVCODEC AVFORMAT AVUTIL AVDEVICE AVFILTER SWSCALE REQUIRED)

# Solution taken from here https://stackoverflow.com/a/61427931/8369974
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
        libavdevice
        libavfilter
        libavformat
        libavcodec
        libswresample
        libswscale
        libavutil
        )

#set(THREADS_PREFER_PTHREAD_FLAG ON)
#find_package(Threads)

add_executable(hls_encoder src/main.cpp src/helpers.cpp src/hls_encoder.cpp)

#target_link_libraries(hls_encoder ${AVUTIL_LIBRARIES} ${AVFILTER_LIBRARIES} ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${SWSCALE_LIBRARIES} )
target_link_libraries(hls_encoder PkgConfig::LIBAV)


