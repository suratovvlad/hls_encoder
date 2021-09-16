//
// Created by degree on 11.09.2021.
//

#ifndef HLS_ENCODER_LIBAV_INCLUDE_H
#define HLS_ENCODER_LIBAV_INCLUDE_H

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include "helpers.h"

#include <stdexcept>

#endif //HLS_ENCODER_LIBAV_INCLUDE_H
