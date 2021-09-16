//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_OUTPUT_STREAM_H
#define HLS_ENCODER_OUTPUT_STREAM_H

#include "libav_include.h"
#include "format_context.h"
#include "stream.h"
#include "scaling_options.h"

struct output_stream : public stream
{
public:
    output_stream( const format_context& _format_context, const stream_type& _stream_type, const AVRational& _input_framerate, const scaling_options& _scaling_options ) : stream()
    {
        m_stream_type = _stream_type;
        m_stream = avformat_new_stream( _format_context.get(), nullptr );
        if( !m_stream )
        {
            throw std::runtime_error( "Failed to allocate output stream" );
        }

        if( m_stream_type == stream::stream_type::video )
        {
            m_codec = avcodec_find_encoder( AV_CODEC_ID_H264 );
        }
        else if( m_stream_type == stream::stream_type::audio )
        {
            m_codec = avcodec_find_encoder( AV_CODEC_ID_AAC );
        }

        if( !m_codec )
        {
            throw std::runtime_error( "Failed to find proper output codec" );
        }

        m_codec_context = avcodec_alloc_context3( m_codec );

        if( !m_codec_context )
        {
            throw std::runtime_error( "Failed to allocate a codec context" );
        }

        // TODO: setup parameters
        if( m_stream_type == stream::stream_type::video )
        {
//            av_opt_set(sc->video_avcc->priv_data, "preset", "fast", 0);
//            if (sp.codec_priv_key && sp.codec_priv_value)
//                av_opt_set(sc->video_avcc->priv_data, sp.codec_priv_key, sp.codec_priv_value, 0);
//
//            sc->video_avcc->height = decoder_ctx->height;
//            sc->video_avcc->width = decoder_ctx->width;
//            sc->video_avcc->sample_aspect_ratio = decoder_ctx->sample_aspect_ratio;
//            if (sc->video_avc->pix_fmts)
//                sc->video_avcc->pix_fmt = sc->video_avc->pix_fmts[0];
//            else
//                sc->video_avcc->pix_fmt = decoder_ctx->pix_fmt;
//
//            sc->video_avcc->bit_rate = 2 * 1000 * 1000;
//            sc->video_avcc->rc_buffer_size = 4 * 1000 * 1000;
//            sc->video_avcc->rc_max_rate = 2 * 1000 * 1000;
//            sc->video_avcc->rc_min_rate = 2.5 * 1000 * 1000;
//
            m_codec_context->width = _scaling_options.target_width;
            m_codec_context->height = _scaling_options.target_height;
            m_codec_context->sample_aspect_ratio = AVRational{ 1, 1 };
            if( m_codec->pix_fmts )
            {
                m_codec_context->pix_fmt = m_codec->pix_fmts[ 0 ];
            }
            else
            {
                m_codec_context->pix_fmt = _scaling_options.target_pixel_format;
            }

            m_codec_context->bit_rate = _scaling_options.target_width * _scaling_options.target_height * _input_framerate.num * 0.1;

//            m_codec_context->bit_rate = _scaling_options.bit_rate;
//            m_codec_context->rc_max_rate = 2 * m_codec_context->bit_rate;
//            m_codec_context->rc_min_rate = _scaling_options.bit_rate;
//            m_codec_context->rc_buffer_size = 4 * m_codec_context->bit_rate;

            m_codec_context->time_base = av_inv_q( _input_framerate );
            m_codec_context->gop_size = 60;
            m_codec_context->max_b_frames = 1;
            m_stream->time_base = m_codec_context->time_base;


            av_opt_set(m_codec_context->priv_data, "preset", "ultrafast", 0);


//            av_opt_set(m_codec_context->priv_data, "hls_time", "4", 0);
        }
        else if( m_stream_type == stream::stream_type::audio )
        {
//            int OUTPUT_CHANNELS = 2;
//            int OUTPUT_BIT_RATE = 196000;
//            sc->audio_avcc->channels       = OUTPUT_CHANNELS;
//            sc->audio_avcc->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
//            sc->audio_avcc->sample_rate    = sample_rate;
//            sc->audio_avcc->sample_fmt     = sc->audio_avc->sample_fmts[0];
//            sc->audio_avcc->bit_rate       = OUTPUT_BIT_RATE;
//            sc->audio_avcc->time_base      = (AVRational){1, sample_rate};
//
//            sc->audio_avcc->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
//
//            sc->audio_avs->time_base = sc->audio_avcc->time_base;

            const int OUTPUT_CHANNELS = 2;
            m_codec_context->channels       = OUTPUT_CHANNELS;
            m_codec_context->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
            m_codec_context->sample_rate    = _input_framerate.den;
            m_codec_context->sample_fmt     = m_codec->sample_fmts[0];
            m_codec_context->bit_rate       = 128000;


            m_codec_context->time_base = _input_framerate; // sample_rate

        }

        auto ret = avcodec_open2( m_codec_context, m_codec, nullptr );
        if( ret < 0 )
        {
            throw std::runtime_error( "Failed to open codec. Error: [" + helpers::error2string( ret ) + "] " );
        }

        ret = avcodec_parameters_from_context( m_stream->codecpar, m_codec_context );
        if( ret < 0 )
        {
            throw std::runtime_error( "Failed to fill codec context. Error: [" + helpers::error2string( ret ) + "] " );
        }
        m_stream->time_base = m_codec_context->time_base;
    }
};

#endif //HLS_ENCODER_OUTPUT_STREAM_H
