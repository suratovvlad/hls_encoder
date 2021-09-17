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
    int gop_size = 60;
    int max_b_frames = 1;
};

#endif //HLS_ENCODER_SCALING_OPTIONS_H
