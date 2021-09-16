//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_STREAM_H
#define HLS_ENCODER_STREAM_H

#include "libav_include.h"

struct stream
{
public:
    enum class stream_type
    {
        not_initialized,
        video,
        audio,
        unknown
    };

    stream()
            : m_stream( nullptr )
            , m_stream_index( 0 )
            , m_stream_type( stream_type::not_initialized )
            , m_codec( nullptr )
            , m_codec_context( nullptr )
    {
    }

    virtual ~stream()
    {
        if( m_codec_context )
        {
            avcodec_free_context( &m_codec_context );
            m_codec_context = nullptr;
        }
    }

    stream_type get_stream_type() const
    {
        return m_stream_type;
    }

    AVStream* get_av_stream() const
    {
        return m_stream;
    }

    AVCodecContext* get_av_codec_context() const
    {
        return m_codec_context;
    }

    unsigned get_stream_index() const
    {
        return m_stream_index;
    }

protected:
    AVStream* m_stream;
    unsigned m_stream_index;
    stream_type m_stream_type;
    AVCodec* m_codec;
    AVCodecContext* m_codec_context;
};

#endif //HLS_ENCODER_STREAM_H
