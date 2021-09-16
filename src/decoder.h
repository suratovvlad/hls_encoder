//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_DECODER_H
#define HLS_ENCODER_DECODER_H

#include "libav_include.h"
#include "format_context.h"
#include "stream.h"
#include "input_stream_info.h"
#include "coder.h"

struct decoder: public coder
{
public:
    decoder( const std::unique_ptr< input_stream_info >& _input_stream ) : coder()
    {
        const auto& format_context = _input_stream->get_format_context();

        for( unsigned i = 0; i < format_context.number_of_streams(); ++i )
        {
            AVStream* av_stream = format_context.get_stream_by_idx(i);
            auto _stream = std::make_unique< input_stream >( av_stream, i );
            if( _stream->get_stream_type() == stream::stream_type::video )
            {
                m_video_stream = std::move( _stream );
            }
            else if( _stream->get_stream_type() == stream::stream_type::audio )
            {
                m_audio_stream = std::move( _stream );
            }

        }
    }
};

#endif //HLS_ENCODER_DECODER_H
