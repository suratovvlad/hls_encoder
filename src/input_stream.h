//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_INPUT_STREAM_H
#define HLS_ENCODER_INPUT_STREAM_H

#include "libav_include.h"
#include "stream.h"

struct input_stream : public stream
{
public:
    input_stream( AVStream* _stream, unsigned stream_index ) : stream()
    {
        if( !_stream )
        {
            throw std::runtime_error( "AVStream is nullptr" );
        }

        m_stream = _stream;
        m_stream_index = stream_index;

        if( m_stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            m_stream_type = stream_type::video;
        }
        else if( m_stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
        {
            m_stream_type = stream_type::audio;
        }
        else
        {
            m_stream_type = stream_type::unknown;
        }

        if( m_stream_type == stream_type::video || m_stream_type == stream_type::audio )
        {
            m_codec = avcodec_find_decoder( m_stream->codecpar->codec_id );

            if( !m_codec )
            {
                throw std::runtime_error( "Failed to find a codec" );
            }

            m_codec_context = avcodec_alloc_context3( m_codec );

            if( !m_codec_context )
            {
                throw std::runtime_error( "Failed to allocate a codec context" );
            }

            auto ret = avcodec_parameters_to_context( m_codec_context, m_stream->codecpar );
            if( ret < 0 )
            {
                throw std::runtime_error( "Failed to fill codec context. Error: [" + helpers::error2string( ret ) + "] " );
            }

            ret = avcodec_open2( m_codec_context, m_codec, nullptr );
            if( ret < 0 )
            {
                throw std::runtime_error( "Failed to open codec. Error: [" + helpers::error2string( ret ) + "] " );
            }
        }
    }
};

#endif //HLS_ENCODER_INPUT_STREAM_H
