cmake_minimum_required(VERSION 3.18)
project(hls_encoder)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED yes)

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

add_executable(hls_encoder src/main.cpp src/helpers.cpp src/hls_encoder.cpp)

target_link_libraries(hls_encoder PkgConfig::LIBAV)


