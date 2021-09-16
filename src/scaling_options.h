//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_SCALING_OPTIONS_H
#define HLS_ENCODER_SCALING_OPTIONS_H

#include "libav_include.h"

struct scaling_options final
{
    int source_width = 0;
    int source_height = 0;
    int target_width = 0;
    int target_height = 0;
    AVPixelFormat source_pixel_format = AV_PIX_FMT_YUV420P;
    AVPixelFormat target_pixel_format = AV_PIX_FMT_YUV420P;
    int64_t bit_rate = 0;

    scaling_options() =default;
    scaling_options( const scaling_options& ) =default;
    scaling_options( scaling_options&& )= default;

    scaling_options& operator=( const scaling_options& ) =default;
    scaling_options& operator=( scaling_options&& )= default;
    ~scaling_options() = default;
};

#endif //HLS_ENCODER_SCALING_OPTIONS_H
