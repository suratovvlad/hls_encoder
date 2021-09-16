//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_ENCODER_H
#define HLS_ENCODER_ENCODER_H

#include "libav_include.h"
#include "output_stream_info.h"
#include "scaling_options.h"
#include "output_stream.h"

struct hls_stream_encoder: public coder
{
public:
    hls_stream_encoder( const std::unique_ptr< output_stream_info > & _output_stream_info,
                        const AVRational& _input_framerate,
                        const AVRational& _input_samplerate,
                        const std::vector< scaling_options >& _scaling_options )
    {
        for( const auto& _scaling_option : _scaling_options )
        {
            m_video_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_option ) );
        }

        for( const auto& _scaling_option : _scaling_options )
        {
            m_audio_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_option ) );
        }

        m_streams_count = _scaling_options.size();
    }

    using stream_index = int;
    using stream_ptr = std::unique_ptr< output_stream >;
    using streams_map = std::vector< stream_ptr >;

    enum class stream_type
    {
        video,
        audio
    };

    template< stream_type TStreamType >
    const stream_ptr& get_stream_by_index( stream_index idx ) const
    {
        if constexpr( TStreamType == stream_type::video )
        {
            return m_video_streams.at( idx );
        }
        else
        {
            return m_audio_streams.at( idx );
        }
    }

    const stream_ptr& get_video_stream( stream_index idx ) const
    {
        return get_stream_by_index< stream_type::video >( idx );
    }

    const stream_ptr& get_audio_stream( stream_index idx ) const
    {
        return get_stream_by_index< stream_type::audio >( idx );
    }

    stream_index get_streams_count() const
    {
        return m_streams_count;
    }

private:
    stream_index m_streams_count;
    streams_map m_video_streams;
    streams_map m_audio_streams;
};

#endif //HLS_ENCODER_ENCODER_H
