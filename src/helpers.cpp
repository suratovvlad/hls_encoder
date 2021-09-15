//
// Created by degree on 11.09.2021.
//
#include "helpers.h"
#include "libav_include.h"

namespace helpers {
    std::string error2string( int error ) {
        char errorBuf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
        av_strerror(error, errorBuf, AV_ERROR_MAX_STRING_SIZE);
        return std::string( errorBuf );
    }
}
