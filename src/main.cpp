//
// Created by degree on 11.09.2021.
//

#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "hls_encoder.h"

std::vector< std::string > parse_arguments( int argc, char *argv[] )
{
    auto args = std::vector< std::string >{};
    args.reserve( argc );

    if( argc < 2 )
    {
        throw std::runtime_error( "Provide video input file name with first argument" );
    }

    if( argc < 3 )
    {
        throw std::runtime_error( "Provide audio input file name with second argument" );
    }

    if( argc < 4 )
    {
        throw std::runtime_error( "Provide video output file name with third argument" );
    }

    for( int i = 1; i < argc; ++i )
    {
        args.push_back( argv[i] );
    }

    return args;
}

//
//int encode_video(const input_stream_context& _decoder_context, const output_stream_context& _encoder_context, AVFrame *input_frame) {
//    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;
//
//    AVPacket *output_packet = av_packet_alloc();
//    av_init_packet(output_packet);
//
//    if (!output_packet) { throw std::runtime_error("could not allocate memory for output packet"); }
//
//    const auto& _decoder = _decoder_context.get_decoder();
//    const auto& _encoder = _encoder_context.get_encoder();
//
//    auto _encoder_video_codec_context = _encoder->get_video_stream()->get_av_codec_context();
//    auto _encoder_video_stream = _encoder->get_video_stream()->get_av_stream();
//
//    auto _decoder_video_stream = _decoder->get_video_stream()->get_av_stream();
//
//    int response = avcodec_send_frame( _encoder_video_codec_context, input_frame);
//
//    while (response >= 0) {
//        response = avcodec_receive_packet(_encoder_video_codec_context, output_packet);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//
//        output_packet->stream_index = _decoder->get_video_stream()->get_stream_index();
//        output_packet->duration = _encoder_video_stream->time_base.den / _encoder_video_stream->time_base.num / _decoder_video_stream->avg_frame_rate.num * _decoder_video_stream->avg_frame_rate.den;
//
//        av_packet_rescale_ts(output_packet, _decoder_video_stream->time_base, _encoder_video_stream->time_base);
//        response = av_interleaved_write_frame( _encoder_context.get_stream_info()->get_format_context().get(), output_packet );
//        if (response != 0 && response != AVERROR(EINVAL)) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//    }
//    av_packet_unref(output_packet);
//    av_packet_free(&output_packet);
//    return 0;
//}
//
//int encode_audio(const input_stream_context& _decoder_context, const output_stream_context& _encoder_context, AVFrame *input_frame) {
//    AVPacket *output_packet = av_packet_alloc();
//    if (!output_packet) {throw std::runtime_error("could not allocate memory for output packet");}
//
//    const auto& _decoder = _decoder_context.get_decoder();
//    const auto& _encoder = _encoder_context.get_encoder();
//    auto _encoder_audio_codec_context = _encoder->get_audio_stream()->get_av_codec_context();
//    auto _encoder_audio_stream = _encoder->get_audio_stream()->get_av_stream();
//    auto _decoder_audio_stream = _decoder->get_audio_stream()->get_av_stream();
//
//    int response = avcodec_send_frame( _encoder_audio_codec_context, input_frame);
//
//    while (response >= 0) {
//        response = avcodec_receive_packet( _encoder_audio_codec_context, output_packet);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//
//        output_packet->stream_index = _decoder->get_audio_stream()->get_stream_index();
//
//        av_packet_rescale_ts(output_packet, _decoder_audio_stream->time_base, _encoder_audio_stream->time_base);
//        response = av_interleaved_write_frame( _encoder_context.get_stream_info()->get_format_context().get(), output_packet);
//        if (response != 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//    }
//    av_packet_unref(output_packet);
//    av_packet_free(&output_packet);
//    return 0;
//}
//
//int transcode_audio(const input_stream_context& _decoder_context,
//                    const output_stream_context& _encoder_context_1,
//                    const output_stream_context& _encoder_context_2,
//                    const output_stream_context& _encoder_context_3,
//                    AVPacket *input_packet, AVFrame *input_frame)
//{
//    const auto& _decoder = _decoder_context.get_decoder();
//    auto _decoder_audio_codec_context = _decoder->get_audio_stream()->get_av_codec_context();
//
//    int response = avcodec_send_packet( _decoder_audio_codec_context, input_packet);
//    if (response < 0) {
//        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
//        return response;
//    }
//
//    while (response >= 0) {
//        response = avcodec_receive_frame(_decoder_audio_codec_context, input_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
//            return response;
//        }
//
//        if (response >= 0) {
//            encode_audio(_decoder_context, _encoder_context_1, input_frame);
//            encode_audio(_decoder_context, _encoder_context_2, input_frame);
//            encode_audio(_decoder_context, _encoder_context_3, input_frame);
//        }
//        av_frame_unref(input_frame);
//    }
//    return 0;
//}
//
//int transcode_video(const input_stream_context& _decoder_context,
//                    const output_stream_context& _encoder_context_1,
//                    const output_stream_context& _encoder_context_2,
//                    const output_stream_context& _encoder_context_3,
//                    AVPacket *input_packet,
//                    AVFrame* input_frame,
//                    AVFrame* scaled_frame_1,
//                    AVFrame* scaled_frame_2,
//                    AVFrame* scaled_frame_3
//                    )
//{
//    const auto& _decoder = _decoder_context.get_decoder();
//    auto _decoder_video_codec_context = _decoder->get_video_stream()->get_av_codec_context();
//
//    int response = avcodec_send_packet(_decoder_video_codec_context, input_packet);
//    if (response < 0){
//        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
//        return response;
//    }
//
//    while (response >= 0) {
//        response = avcodec_receive_frame(_decoder_video_codec_context, input_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0 && response != AVERROR(EINVAL) ) {
//            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
//            return response;
//        }
//
//
//        if (response >= 0) {
//            {
//                _encoder_context_1.get_scaler()->scale(input_frame, scaled_frame_1);
//                encode_video(_decoder_context, _encoder_context_1, scaled_frame_1);
//            }
//            {
//                _encoder_context_2.get_scaler()->scale( input_frame, scaled_frame_2 );
//                encode_video(_decoder_context, _encoder_context_2, scaled_frame_2);
//            }
//            {
//                _encoder_context_3.get_scaler()->scale( input_frame, scaled_frame_3 );
//                encode_video(_decoder_context, _encoder_context_3, scaled_frame_3);
//            }
//        }
//
//        av_frame_unref(input_frame);
//    }
//
//    return 0;
//}
//
//
//int to_hls( int argc, char *argv[] )
//{
//    try {
//        int64_t pts = 0;
////        avcodec_register_all();
//
//        const auto args = parse_arguments( argc, argv );
//        input_stream_context _input_stream_context_1( args.front() );
//        input_stream_context _input_stream_context_2( args[1] );
//
//        const auto input_framerate = _input_stream_context_1.get_input_framerate();
//        const auto input_samplerate = _input_stream_context_2.get_input_samplerate();
//
//
//        scaling_options _scale_1 = { 1920, 1080, 1280, 720, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };
//        scaling_options _scale_2 = { 1920, 1080, 840, 480, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };
//        scaling_options _scale_3 = { 1920, 1080, 420, 240, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };
//
//        output_stream_context _output_stream_context_1( args[2], input_framerate, input_samplerate, _scale_1 );
//        output_stream_context _output_stream_context_2( args[2], input_framerate, input_samplerate, _scale_2 );
//        output_stream_context _output_stream_context_3( args[2], input_framerate, input_samplerate, _scale_3 );
////        output_stream_info _output_stream_info( args[2] );
////        encoder _encoder( _output_stream_info, input_framerate, input_samplerate );
//
//
////        auto _output_format_context = _output_stream_context.get_stream_info()->get_format_context();
////        auto _av_output_format_context = _output_format_context.get();
//
////        if (_av_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
////            _av_output_format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
////
////        if (!(_av_output_format_context->oformat->flags & AVFMT_NOFILE)) {
////            if (avio_open(&_av_output_format_context->pb, args[2].c_str(), AVIO_FLAG_WRITE) < 0) {
////                throw std::runtime_error("could not open the output file");
////                return -1;
////            }
////        }
//
//        _output_stream_context_1.write_header();
//        _output_stream_context_2.write_header();
//        _output_stream_context_3.write_header();
//
//        frame video_input_frame;
//        frame audio_input_frame;
//        packet video_input_packet;
//        packet audio_input_packet;
//
//        const auto& _format_context_1 = _input_stream_context_1.get_stream_info()->get_format_context();
//        const auto& _format_context_2 = _input_stream_context_2.get_stream_info()->get_format_context();
//
//        frame scaled_frame_1;
//        frame scaled_frame_2;
//        frame scaled_frame_3;
//
//        bool no_video = false;
//        bool no_audio = false;
//        while( true )
//        {
//            if( av_read_frame( _format_context_1.get(), video_input_packet.get() ) >= 0 )
//            {
//                if( _format_context_1.get_stream_by_idx( video_input_packet.get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
//                {
////                    transcode_video(_input_stream_context_1, _output_stream_context_1, video_input_packet.get(), video_input_frame_1.get() );
////                    transcode_video(_input_stream_context_1, _output_stream_context_2, video_input_packet.get(), video_input_frame_2.get() );
//                    transcode_video(_input_stream_context_1,
//                                    _output_stream_context_1,
//                                    _output_stream_context_2,
//                                    _output_stream_context_3,
//                                    video_input_packet.get(),
//                                    video_input_frame.get(),
//                                    scaled_frame_1.get(),
//                                    scaled_frame_2.get(),
//                                    scaled_frame_3.get() );
//                }
//            }
//            else
//            {
//                no_video = true;
//            }
//            if( av_read_frame( _format_context_2.get(), audio_input_packet.get() ) >= 0 )
//            {
//                if (_format_context_2.get_stream_by_idx(audio_input_packet.get()->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
//                {
////                    transcode_audio(_input_stream_context_2, _output_stream_context_1, audio_input_packet.get(),  audio_input_frame_1.get());
////                    transcode_audio(_input_stream_context_2, _output_stream_context_2, audio_input_packet.get(),  audio_input_frame_2.get());
//                    transcode_audio(_input_stream_context_2,  _output_stream_context_1, _output_stream_context_2, _output_stream_context_3, audio_input_packet.get(),  audio_input_frame.get());
//                }
//            }
//            else
//            {
//                no_audio = true;
//            }
//            if (no_video && no_audio)
//            {
//                break;
//            }
//        }
//
//        _output_stream_context_1.write_trailer();
//        _output_stream_context_2.write_trailer();
//        _output_stream_context_3.write_trailer();
//
//    }
//    catch ( const std::runtime_error& ex )
//    {
//        std::cout << "Error: " << ex.what();
//        return -1;
//    }
//
//    return 0;
//}
//
//
//int encode_video_hls(const std::unique_ptr<input_stream_context>& _decoder_context,
//                     const std::unique_ptr< output_stream>& _video_stream, const std::unique_ptr< output_stream_info >& _stream_info, AVFrame *input_frame, int _stream_index) {
//    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;
//
//    AVPacket *output_packet = av_packet_alloc();
//    av_init_packet(output_packet);
//
//    if (!output_packet) { throw std::runtime_error("could not allocate memory for output packet"); }
//
//    const auto& _decoder = _decoder_context->get_decoder();
//    auto _decoder_video_stream = _decoder->get_video_stream()->get_av_stream();
//
//    auto _encoder_video_codec_context = _video_stream->get_av_codec_context();
//    auto _encoder_video_stream = _video_stream->get_av_stream();
//
//
//    int response = avcodec_send_frame( _encoder_video_codec_context, input_frame);
//
//    while (response >= 0) {
//        response = avcodec_receive_packet(_encoder_video_codec_context, output_packet);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//
//        output_packet->stream_index = _stream_index;
//        output_packet->duration = _encoder_video_stream->time_base.den / _encoder_video_stream->time_base.num / _decoder_video_stream->avg_frame_rate.num * _decoder_video_stream->avg_frame_rate.den;
//
//        av_packet_rescale_ts(output_packet, _decoder_video_stream->time_base, _encoder_video_stream->time_base);
//        response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet );
//        if (response != 0 && response != AVERROR(EINVAL)) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//    }
//    av_packet_unref(output_packet);
//    av_packet_free(&output_packet);
//    return 0;
//}
//
//int transcode_video_hls(const std::unique_ptr< input_stream_context >& _decoder_context,
//                        const std::unique_ptr< hls_stream_context >& _encoder_context,
//                    AVPacket *input_packet,
//                    AVFrame* input_frame,
//                    const std::vector< std::unique_ptr< frame > >& _scaling_frames )
//{
//    const auto& _decoder = _decoder_context->get_decoder();
//    auto _decoder_video_codec_context = _decoder->get_video_stream()->get_av_codec_context();
//
//
//    const auto& _encoder = _encoder_context->get_encoder();
//
//    int response = avcodec_send_packet(_decoder_video_codec_context, input_packet);
//    if (response < 0){
//        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
//        return response;
//    }
//
//    while (response >= 0) {
//        response = avcodec_receive_frame(_decoder_video_codec_context, input_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0 && response != AVERROR(EINVAL) ) {
//            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
//            return response;
//        }
//
//
//        const int stream_index = _decoder->get_video_stream()->get_stream_index();
//        if (response >= 0) {
//
////            std::vector< AVFrame* > _scaled_frames;
////            _scaled_frames.push_back( scaled_frame_1 );
////            _scaled_frames.push_back( scaled_frame_2 );
////            _scaled_frames.push_back( scaled_frame_3 );
//            for( hls_stream_encoder::stream_index _idx = 0; _idx < _encoder->get_streams_count(); ++_idx )
//            {
//                auto _scaling_frame = _scaling_frames.at( _idx )->get();
//                _encoder_context->get_video_scaler_by_index( _idx )->scale( input_frame, _scaling_frame );
//                encode_video_hls(_decoder_context, _encoder->get_video_stream( _idx ), _encoder_context->get_stream_info(),_scaling_frame, stream_index + _idx );
//            }
//
////            {
////                _encoder_context.get_video_scaler_by_index( 0 )->scale(input_frame, scaled_frame_1);
////                encode_video_hls(_decoder_context, _encoder->get_video_stream( 0 ), _encoder_context.get_stream_info(), scaled_frame_1, stream_index);
////            }
////            {
////                _encoder_context.get_video_scaler_by_index( 1 )->scale(input_frame, scaled_frame_2);
////                encode_video_hls(_decoder_context,  _encoder->get_video_stream( 1 ), _encoder_context.get_stream_info(), scaled_frame_2, stream_index + 1);
////            }
////            {
////                _encoder_context.get_video_scaler_by_index( 2 )->scale(input_frame, scaled_frame_3);
////                encode_video_hls(_decoder_context,  _encoder->get_video_stream( 2 ), _encoder_context.get_stream_info(), scaled_frame_3, stream_index + 2);
////            }
//        }
//
//        av_frame_unref(input_frame);
//    }
//
//    return 0;
//}
//
//int encode_audio_hls(const std::unique_ptr< input_stream_context >& _decoder_context,
//                     const std::unique_ptr< output_stream >& _audio_stream, const std::unique_ptr< output_stream_info >& _stream_info, AVFrame *input_frame, int _stream_index) {
//    AVPacket *output_packet = av_packet_alloc();
//    if (!output_packet) {throw std::runtime_error("could not allocate memory for output packet");}
//
//    const auto& _decoder = _decoder_context->get_decoder();
//    auto _decoder_audio_stream = _decoder->get_audio_stream()->get_av_stream();
//
//    auto _encoder_audio_codec_context = _audio_stream->get_av_codec_context();
//    auto _encoder_audio_stream = _audio_stream->get_av_stream();
//
//    int response = avcodec_send_frame( _encoder_audio_codec_context, input_frame);
//
//    while (response >= 0) {
//        response = avcodec_receive_packet( _encoder_audio_codec_context, output_packet);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//
//        output_packet->stream_index = _stream_index;
//
//        av_packet_rescale_ts(output_packet, _decoder_audio_stream->time_base, _encoder_audio_stream->time_base);
//        response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet);
//        if (response != 0) {
//            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
//            return -1;
//        }
//    }
//    av_packet_unref(output_packet);
//    av_packet_free(&output_packet);
//    return 0;
//}
//
//int transcode_audio_hls(const std::unique_ptr< input_stream_context >& _decoder_context,
//                        const std::unique_ptr< hls_stream_context >& _encoder_context,
//                        AVPacket *input_packet, AVFrame *input_frame)
//{
//    const auto& _decoder = _decoder_context->get_decoder();
//    auto _decoder_audio_codec_context = _decoder->get_audio_stream()->get_av_codec_context();
//
//    const auto& _encoder = _encoder_context->get_encoder();
//
//    int response = avcodec_send_packet( _decoder_audio_codec_context, input_packet);
//    if (response < 0) {
//        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
//        return response;
//    }
//
//    while (response >= 0) {
//        response = avcodec_receive_frame(_decoder_audio_codec_context, input_frame);
//        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
//            break;
//        } else if (response < 0) {
//            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
//            return response;
//        }
//
//        const int _stream_index = _encoder->get_streams_count() ;
//        if (response >= 0) {
//            for( hls_stream_encoder::stream_index _idx = 0; _idx < _encoder->get_streams_count(); ++_idx )
//            {
//                encode_audio_hls(_decoder_context, _encoder->get_audio_stream( _idx ), _encoder_context->get_stream_info(), input_frame, _stream_index + _idx);
//            }
//
////            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_1(), _encoder_context.get_stream_info(), input_frame, _stream_index + 2);
////            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_2(), _encoder_context.get_stream_info(), input_frame, _stream_index + 3);
////            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_3(), _encoder_context.get_stream_info(), input_frame, _stream_index + 4);
//        }
//        av_frame_unref(input_frame);
//    }
//    return 0;
//}
///*
//int to_hls2( int argc, char *argv[] )
//{
//    try {
//        int64_t pts = 0;
////        avcodec_register_all();
//
//        const auto args = parse_arguments(argc, argv);
//        input_stream_context _input_stream_context_1(args.front());
//        input_stream_context _input_stream_context_2(args[1]);
//
//        const auto input_framerate = _input_stream_context_1.get_input_framerate();
//        const auto input_samplerate = _input_stream_context_2.get_input_samplerate();
//
//
//
//        scaling_options _scale_1 = { 1920, 1080, 1280, 720, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 2 * 1000 * 1000 };
//        scaling_options _scale_2 = { 1920, 1080, 840, 480, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 1 * 1000 * 1000 };
//        scaling_options _scale_3 = { 1920, 1080, 420, 240, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 500 * 1000 };
////        scaling_options _scale_4 = { 1920, 1080, 1920, 1080, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 10 * 1000 * 1000 };
//
//        const auto _scalings = { _scale_1, _scale_2, _scale_3 };
//        // _scale_4 ;
//
//
//        hls_stream_context _hls_stream_context( args[2], input_framerate, input_samplerate, _scalings );
//
//        _hls_stream_context.write_header();
//
//        frame video_input_frame;
//        frame audio_input_frame;
//        packet video_input_packet;
//        packet audio_input_packet;
//
//        const auto& _format_context_1 = _input_stream_context_1.get_stream_info()->get_format_context();
//        const auto& _format_context_2 = _input_stream_context_2.get_stream_info()->get_format_context();
//
//        std::vector< std::unique_ptr< frame > > _scaling_frames;
//        _scaling_frames.reserve( _scalings.size() );
//        for( int i = 0; i < _scalings.size(); ++i )
//        {
//            _scaling_frames.emplace_back( std::make_unique< frame >() );
//        }
////        frame scaled_frame_1;
////        frame scaled_frame_2;
////        frame scaled_frame_3;
//
//        bool no_video = false;
//        bool no_audio = false;
//        while( true )
//        {
//            if( av_read_frame( _format_context_1.get(), video_input_packet.get() ) >= 0 )
//            {
//                if( _format_context_1.get_stream_by_idx( video_input_packet.get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
//                {
////                    transcode_video(_input_stream_context_1, _output_stream_context_1, video_input_packet.get(), video_input_frame_1.get() );
////                    transcode_video(_input_stream_context_1, _output_stream_context_2, video_input_packet.get(), video_input_frame_2.get() );
//                    transcode_video_hls(_input_stream_context_1,
//                                        _hls_stream_context,
//                                    video_input_packet.get(),
//                                    video_input_frame.get(),
//                                    _scaling_frames );
//                }
//            }
//            else
//            {
//                no_video = true;
//            }
//            if( av_read_frame( _format_context_2.get(), audio_input_packet.get() ) >= 0 )
//            {
//                if (_format_context_2.get_stream_by_idx(audio_input_packet.get()->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
//                {
////                    transcode_audio(_input_stream_context_2, _output_stream_context_1, audio_input_packet.get(),  audio_input_frame_1.get());
////                    transcode_audio(_input_stream_context_2, _output_stream_context_2, audio_input_packet.get(),  audio_input_frame_2.get());
//                    transcode_audio_hls(_input_stream_context_2, _hls_stream_context, audio_input_packet.get(),  audio_input_frame.get());
//                }
//            }
//            else
//            {
//                no_audio = true;
//            }
//            if (no_video && no_audio)
//            {
//                break;
//            }
//        }
//
//
//        _hls_stream_context.write_trailer();
//
//    }
//    catch ( const std::runtime_error& ex )
//    {
//        std::cout << "Error: " << ex.what();
//        return -1;
//    }
//
//    return 0;
//}
//*/
//
//int to_hls3( int argc, char *argv[] )
//{
//    try {
//        const auto _args = parse_arguments(argc, argv);
//        const auto _video_src = _args.at( 0 );
//        const auto _audio_src = _args.at( 1 );
//        const auto _output_dir = _args.at( 2 );
//        auto _hls_encoder = std::make_unique< hls_encoder >( _video_src, _audio_src, _output_dir );
//
//        scaling_options _scale_1 = { 1920, 1080, 1280, 720, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 2 * 1000 * 1000 };
//        scaling_options _scale_2 = { 1920, 1080, 840, 480, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 1 * 1000 * 1000 };
//        scaling_options _scale_3 = { 1920, 1080, 420, 240, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 500 * 1000 };
//
//        _hls_encoder->add_scaling_option( _scale_1 );
//        _hls_encoder->add_scaling_option( _scale_2 );
//        _hls_encoder->add_scaling_option( _scale_3 );
//
//        _hls_encoder->initialize_input_output();
//
//
//        _hls_encoder->m_hls_stream_context->write_header();
//
//        frame video_input_frame;
//        frame audio_input_frame;
//        packet video_input_packet;
//        packet audio_input_packet;
//
//        const auto& _format_context_1 = _hls_encoder->m_video_src_input_context->get_stream_info()->get_format_context();
//        const auto& _format_context_2 = _hls_encoder->m_audio_src_input_context->get_stream_info()->get_format_context();
//
//        std::vector< std::unique_ptr< frame > > _scaling_frames;
//        _scaling_frames.reserve( _hls_encoder->m_scaling_options.size() );
//        for( int i = 0; i < _hls_encoder->m_scaling_options.size(); ++i )
//        {
//            _scaling_frames.emplace_back( std::make_unique< frame >() );
//        }
////        frame scaled_frame_1;
////        frame scaled_frame_2;
////        frame scaled_frame_3;
//
//        bool no_video = false;
//        bool no_audio = false;
//        while( true )
//        {
//            if( av_read_frame( _format_context_1.get(), video_input_packet.get() ) >= 0 )
//            {
//                if( _format_context_1.get_stream_by_idx( video_input_packet.get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
//                {
////                    transcode_video(_input_stream_context_1, _output_stream_context_1, video_input_packet.get(), video_input_frame_1.get() );
////                    transcode_video(_input_stream_context_1, _output_stream_context_2, video_input_packet.get(), video_input_frame_2.get() );
//                    transcode_video_hls(_hls_encoder->m_video_src_input_context,
//                                        _hls_encoder->m_hls_stream_context,
//                                        video_input_packet.get(),
//                                        video_input_frame.get(),
//                                        _scaling_frames );
//                }
//            }
//            else
//            {
//                no_video = true;
//            }
//            if( av_read_frame( _format_context_2.get(), audio_input_packet.get() ) >= 0 )
//            {
//                if (_format_context_2.get_stream_by_idx(audio_input_packet.get()->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
//                {
////                    transcode_audio(_input_stream_context_2, _output_stream_context_1, audio_input_packet.get(),  audio_input_frame_1.get());
////                    transcode_audio(_input_stream_context_2, _output_stream_context_2, audio_input_packet.get(),  audio_input_frame_2.get());
//                    transcode_audio_hls(_hls_encoder->m_audio_src_input_context, _hls_encoder->m_hls_stream_context, audio_input_packet.get(),  audio_input_frame.get());
//                }
//            }
//            else
//            {
//                no_audio = true;
//            }
//            if (no_video && no_audio)
//            {
//                break;
//            }
//        }
//
//
//        _hls_encoder->m_hls_stream_context->write_trailer();
//    }
//    catch ( const std::runtime_error& ex )
//    {
//        std::cout << "Error: " << ex.what();
//        return -1;
//    }
//
//    return 0;
//}

int main( int argc, char *argv[] )
{
    try {
        const auto _args = parse_arguments(argc, argv);
        const auto _video_src = _args.at( 0 );
        const auto _audio_src = _args.at( 1 );
        const auto _output_dir = _args.at( 2 );

        hls_output_filenames _output_filenames = {
                "media_stream_%v/segment_%03d.ts",
                "media_stream_%v/media.m3u8",
                "master.m3u8"
        };

        auto _hls_encoder = std::make_unique< hls_encoder >( _video_src, _audio_src, _output_dir, _output_filenames );

        constexpr auto AUDIO_BITRATE = 128000;

        // TODO: fill this with arguments to application
        scaling_options _scale_1 = {
                1920, 1080,                             // source
                1280, 720,                              // target
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, // pixel formats of source and target
                2 * 1000 * 1000 - AUDIO_BITRATE,        // bitrate, 128000 - audio bitrate
                60,                                     // GOP size
                1                                       // max_b_frames
        };
        scaling_options _scale_2 = {
                1920, 1080,
                840, 480,
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P,
                1 * 1000 * 1000 - AUDIO_BITRATE,
                60,
                1
        };
        scaling_options _scale_3 = {
                1920, 1080,
                420, 240,
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P,
                500 * 1000 - AUDIO_BITRATE,
                60,
                1
        };

        _hls_encoder->add_scaling_option( _scale_1 );
        _hls_encoder->add_scaling_option( _scale_2 );
        _hls_encoder->add_scaling_option( _scale_3 );

        if( _hls_encoder->initialize_input_output() )
        {
           if( _hls_encoder->process_transcoding() )
               std::cout << " Transcoding has succeeded!";
           else
               std::cout << " Transcoding has failed! ";
        }
        else
        {
            std::cout << " Initialization failed! ";
        }
    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Runtime error: " << ex.what();
        return -1;
    }
    catch ( const std::exception& ex )
    {
        std::cout << "Error: " << ex.what();
        return -1;
    }
    catch ( ... )
    {
        std::cout << " Something really bad happened ";
        return -1;
    }

    return 0;
}