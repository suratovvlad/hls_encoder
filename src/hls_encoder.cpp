//
// Created by degree on 11.09.2021.
//

#include "hls_encoder.h"

hls_encoder::hls_encoder( const std::string &video_src, const std::string &audio_src )
    : m_video_src( video_src )
    , m_audio_src( audio_src )
{
}

