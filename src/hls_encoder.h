//
// Created by degree on 11.09.2021.
//

#ifndef HLS_ENCODER_HLS_ENCODER_H
#define HLS_ENCODER_HLS_ENCODER_H

#include <string>
#include <exception>
#include <stdexcept>
#include <memory>
#include <filesystem>
#include <vector>
#include <iostream>

#include "libav_include.h"

#include "frame.h"
#include "packet.h"
#include "format_context.h"
#include "input_stream_info.h"
#include "stream.h"
#include "input_stream.h"
#include "coder.h"
#include "decoder.h"
#include "input_stream_context.h"

#include "output_stream_info.h"
#include "output_stream.h"
#include "encoder.h"

#include "scaling_options.h"
#include "video_scaler.h"
#include "output_stream_context.h"





//
//
//struct encoder : public coder
//{
//public:
//    encoder( const std::unique_ptr<output_stream_info>& _output_stream, const AVRational& _input_framerate, const AVRational& _input_samplerate, const scaling_options& _scaling_options )
//    {
//        m_video_stream = std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options );
//        m_audio_stream = std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options );
//    }
//};
//
//
//
//
//
//struct output_stream_context
//{
//public:
//    output_stream_context( const std::string& output_path, const AVRational& _input_framerate, const AVRational& _input_samplerate, const scaling_options& _scaling_options )
//        : m_scaling_options( _scaling_options )
//        , m_output_path( output_path )
//    {
//        m_scaler = std::make_unique< video_scaler >( m_scaling_options );
//        m_filename_prefix = std::to_string( m_scaling_options.target_height );
//        m_filename_pattern = m_filename_prefix + "p_%03d.ts";
//        m_filename_playlist = m_filename_prefix + "p.m3u8";
//
//        std::filesystem::path path(m_output_path);
//        m_absolute_path = path / m_filename_playlist;
//
//
//        m_stream_info = std::make_unique< output_stream_info >( m_absolute_path );
//        m_encoder = std::make_unique< encoder >( m_stream_info, _input_framerate, _input_samplerate, _scaling_options );
//
//        auto _av_output_format_context =  m_stream_info->get_format_context().get();
//
//        if (_av_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
//            _av_output_format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//
//        if (!(_av_output_format_context->oformat->flags & AVFMT_NOFILE)) {
//            if (avio_open(&_av_output_format_context->pb, m_absolute_path.c_str(), AVIO_FLAG_WRITE) < 0) {
//                throw std::runtime_error("could not open the output file");
//            }
//        }
//    }
//
//    void write_header()
//    {
//        AVDictionary* headerOptions(0);
////        av_dict_set(&headerOptions, "segment_format", "mpegts", 0);
////        av_dict_set(&headerOptions, "segment_list_type", "m3u8", 0);
////        av_dict_set(&headerOptions, "segment_list", args[2].c_str(), 0);
////        av_dict_set_int(&headerOptions, "segment_list_size", 0, 0);
////        av_dict_set(&headerOptions, "segment_time_delta", "1.00", 0);
////        av_dict_set(&headerOptions, "segment_time", "10", 0);
////        av_dict_set_int(&headerOptions, "reference_stream", _input_stream_context_1.get_decoder()->get_video_stream()->get_stream_index(), 0);
////        av_dict_set(&headerOptions, "segment_list_flags", "cache+live", 0);
//        av_dict_set(&headerOptions, "hls_segment_type", "mpegts", 0);
//        av_dict_set(&headerOptions, "hls_playlist_type", "vod", 0);
//        av_dict_set_int(&headerOptions, "hls_list_size", 0, 0);
//        av_dict_set(&headerOptions, "segment_time_delta", "1.0", 0);
//        av_dict_set(&headerOptions, "hls_flags", "append_list", 0);
//        av_dict_set(&headerOptions, "hls_time", "10", 0);
//        av_dict_set(&headerOptions, "hls_segment_filename", m_filename_pattern.c_str(), 0);
//
//        auto _av_output_format_context =  m_stream_info->get_format_context().get();
//
//        /* init muxer, write output file header */
//        auto ret = avformat_write_header( _av_output_format_context, &headerOptions );
//        if (ret < 0) {
//            throw std::runtime_error( "Error occurred when opening output file" );
//        }
//    }
//
//    void write_trailer()
//    {
//        av_write_trailer( m_stream_info->get_format_context().get() );
//    }
//
//    const std::unique_ptr< encoder >& get_encoder() const
//    {
//        return m_encoder;
//    }
//
//    const std::unique_ptr< output_stream_info >& get_stream_info() const
//    {
//        return m_stream_info;
//    }
//
//    const std::unique_ptr< video_scaler >& get_scaler() const
//    {
//        return m_scaler;
//    }
//
//private:
//    std::unique_ptr< output_stream_info > m_stream_info;
//    std::unique_ptr< encoder > m_encoder;
//    scaling_options m_scaling_options;
//    std::string m_filename_prefix;
//    std::string m_filename_pattern;
//    std::string m_filename_playlist;
//    std::string m_output_path;
//    std::string m_absolute_path;
//    std::unique_ptr< video_scaler > m_scaler;
//};








struct hls_encoder {
public:
    hls_encoder( const std::string& video_src, const std::string& audio_src, const std::string& output_dir  );

    bool initialize_input_output()
    {
        try {
            if( m_scaling_options.empty() )
            {
                throw std::runtime_error( "No output scaling is initialized" );
            }

            m_video_src_input_context = std::make_unique< input_stream_context >( m_video_src );
            m_audio_src_input_context = std::make_unique< input_stream_context >( m_audio_src );

            const auto input_framerate = m_video_src_input_context->get_input_framerate();
            const auto input_samplerate = m_audio_src_input_context->get_input_samplerate();
            m_hls_stream_context = std::make_unique< hls_stream_context >( m_output_dir, input_framerate, input_samplerate, m_scaling_options );

            m_initialized = true;
        }
        catch ( const std::runtime_error& ex )
        {
            std::cout << "Error: " << ex.what();
            return false;
        }
        return true;
    }

    void add_scaling_option( const scaling_options& _scaling_option )
    {
        m_scaling_options.push_back( _scaling_option );
    }

    bool process_transcoding()
    {
        try {
            if( !m_initialized ||
                !m_video_src_input_context ||
                !m_audio_src_input_context ||
                !m_hls_stream_context )
            {
                throw std::runtime_error( "Not initialized" );
            }

            m_hls_stream_context->write_header();

            auto video_input_frame = std::make_unique< frame >();
            auto video_input_packet = std::make_unique< packet >();

            auto audio_input_frame = std::make_unique< frame >();
            auto audio_input_packet = std::make_unique< packet >();

            std::vector< std::unique_ptr< frame > > _scaling_frames;
            _scaling_frames.reserve( m_scaling_options.size() );
            for( int i = 0; i < m_scaling_options.size(); ++i )
            {
                _scaling_frames.emplace_back( std::make_unique< frame >() );
            }

            bool no_video = false;
            bool no_audio = false;
            while( true )
            {
                if( const auto& _format_context_1 = m_video_src_input_context->get_stream_info()->get_format_context();
                    av_read_frame( _format_context_1.get(), video_input_packet->get() ) >= 0 )
                {
                    if( _format_context_1.get_stream_by_idx( video_input_packet->get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
                    {
                        transcode_video( video_input_packet, video_input_frame, _scaling_frames );
                    }
                }
                else
                {
                    no_video = true;
                }
                if( const auto& _format_context_2 = m_audio_src_input_context->get_stream_info()->get_format_context();
                    av_read_frame( _format_context_2.get(), audio_input_packet->get() ) >= 0 )
                {
                    if( _format_context_2.get_stream_by_idx( audio_input_packet->get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
                    {
                        transcode_audio( audio_input_packet, audio_input_frame );
                    }
                }
                else
                {
                    no_audio = true;
                }
                if (no_video && no_audio)
                {
                    break;
                }
            }

            m_hls_stream_context->write_trailer();

        }
        catch ( const std::runtime_error& ex )
        {
            std::cout << "Error: " << ex.what();
            return false;
        }
        return true;
    }

private:

    int encode_video( const std::unique_ptr< output_stream>& _video_stream, const std::unique_ptr< frame >& input_frame, int _stream_index ) {

        if (input_frame)
            input_frame->get()->pict_type = AV_PICTURE_TYPE_NONE;

        packet output_packet;

        if ( !output_packet.get() ) {
            throw std::runtime_error("could not allocate memory for output packet");
        }

        const auto& _decoder = m_video_src_input_context->get_decoder();
        const auto& _stream_info = m_hls_stream_context->get_stream_info();

        auto _decoder_video_stream = _decoder->get_video_stream()->get_av_stream();

        auto _encoder_video_codec_context = _video_stream->get_av_codec_context();
        auto _encoder_video_stream = _video_stream->get_av_stream();


        int response = avcodec_send_frame( _encoder_video_codec_context, input_frame->get() );

        while (response >= 0) {
            response = avcodec_receive_packet( _encoder_video_codec_context, output_packet.get() );
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            } else if (response < 0) {
                throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            }

            output_packet.get()->stream_index = _stream_index;
            output_packet.get()->duration = _encoder_video_stream->time_base.den / _encoder_video_stream->time_base.num / _decoder_video_stream->avg_frame_rate.num * _decoder_video_stream->avg_frame_rate.den;

            av_packet_rescale_ts(output_packet.get(), _decoder_video_stream->time_base, _encoder_video_stream->time_base);
            response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet.get() );
            if (response != 0 && response != AVERROR(EINVAL)) {
                throw std::runtime_error("Error while writing packet: [" + helpers::error2string(response) + "]");
            }
        }
        av_packet_unref( output_packet.get() );
        return 0;
    }

    int transcode_video( const std::unique_ptr< packet >& input_packet, const std::unique_ptr< frame >& input_frame,
                         const std::vector< std::unique_ptr< frame > >& _scaling_frames )
    {
        const auto& _decoder = m_video_src_input_context->get_decoder();
        auto _decoder_video_codec_context = _decoder->get_video_stream()->get_av_codec_context();

        const auto& _encoder = m_hls_stream_context->get_encoder();

        int response = avcodec_send_packet( _decoder_video_codec_context, input_packet->get() );
        if (response < 0){
            throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
        }

        while (response >= 0) {
            response = avcodec_receive_frame( _decoder_video_codec_context, input_frame->get() );
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            } else if (response < 0 && response != AVERROR(EINVAL) ) {
                throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
            }

            const int stream_index = _decoder->get_video_stream()->get_stream_index();
            if (response >= 0) {
                for( hls_stream_encoder::stream_index _idx = 0; _idx < _encoder->get_streams_count(); ++_idx )
                {
                    const auto& _scaling_frame = _scaling_frames.at( _idx );
                    m_hls_stream_context->get_video_scaler_by_index( _idx )->scale( input_frame, _scaling_frame );
                    encode_video( _encoder->get_video_stream( _idx ), _scaling_frame, stream_index + _idx );
                }
            }

            av_frame_unref( input_frame->get() );
        }

        return 0;
    }

    int encode_audio( const std::unique_ptr< output_stream >& _audio_stream, const std::unique_ptr< frame >& input_frame, int _stream_index ) {

        packet output_packet;
        if ( !output_packet.get() ) {
            throw std::runtime_error("could not allocate memory for output packet");
        }

        const auto& _decoder = m_audio_src_input_context->get_decoder();
        const auto& _stream_info = m_hls_stream_context->get_stream_info();

        auto _decoder_audio_stream = _decoder->get_audio_stream()->get_av_stream();

        auto _encoder_audio_codec_context = _audio_stream->get_av_codec_context();
        auto _encoder_audio_stream = _audio_stream->get_av_stream();

        int response = avcodec_send_frame( _encoder_audio_codec_context, input_frame->get() );

        while (response >= 0) {
            response = avcodec_receive_packet( _encoder_audio_codec_context, output_packet.get() );
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            } else if (response < 0) {
                throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            }

            output_packet.get()->stream_index = _stream_index;

            av_packet_rescale_ts( output_packet.get(), _decoder_audio_stream->time_base, _encoder_audio_stream->time_base );
            response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet.get() );
            if (response != 0) {
                throw std::runtime_error("Error while writing packet: [" + helpers::error2string(response) + "]");
            }
        }
        av_packet_unref( output_packet.get() );

        return 0;
    }

    int transcode_audio( const std::unique_ptr< packet >& input_packet, const std::unique_ptr< frame >& input_frame )
    {
        const auto& _decoder = m_audio_src_input_context->get_decoder();
        auto _decoder_audio_codec_context = _decoder->get_audio_stream()->get_av_codec_context();

        const auto& _encoder = m_hls_stream_context->get_encoder();

        int response = avcodec_send_packet( _decoder_audio_codec_context, input_packet->get() );
        if (response < 0) {
            throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
            return response;
        }

        while (response >= 0) {
            response = avcodec_receive_frame( _decoder_audio_codec_context, input_frame->get() );
            if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                break;
            } else if (response < 0) {
                throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
                return response;
            }

            const int _stream_index = _encoder->get_streams_count() ;
            if (response >= 0) {
                for( hls_stream_encoder::stream_index _idx = 0; _idx < _encoder->get_streams_count(); ++_idx )
                {
                    encode_audio( _encoder->get_audio_stream( _idx ), input_frame, _stream_index + _idx);
                }
            }
            av_frame_unref( input_frame->get() );
        }
        return 0;
    }



    std::string m_video_src;
    std::string m_audio_src;
    std::string m_output_dir;
    std::vector< scaling_options > m_scaling_options;

    std::unique_ptr< input_stream_context > m_video_src_input_context;
    std::unique_ptr< input_stream_context > m_audio_src_input_context;
    std::unique_ptr< hls_stream_context > m_hls_stream_context;
    bool m_initialized = false;
};


#endif //HLS_ENCODER_HLS_ENCODER_H
