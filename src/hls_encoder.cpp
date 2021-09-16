//
// Created by degree on 11.09.2021.
//

#include "hls_encoder.h"

hls_encoder::hls_encoder( const std::string &video_src, const std::string &audio_src, const std::string& output_dir )
    : m_video_src( video_src )
    , m_audio_src( audio_src )
    , m_output_dir( output_dir )
{
}

