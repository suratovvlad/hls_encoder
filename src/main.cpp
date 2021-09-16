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

struct frame
{
public:
    frame()
    {
        m_frame = av_frame_alloc();
        if( !m_frame )
        {
            throw std::runtime_error("failed to allocated memory for AVFrame");
        }
    }
    ~frame()
    {
        if( m_frame ) {
            av_frame_free( &m_frame );
            m_frame = nullptr;
        }
    }

    AVFrame* get() const
    {
        return m_frame;
    }

private:
    AVFrame* m_frame;
};

struct packet
{
public:
    packet()
    {
        m_packet = av_packet_alloc();
        if( !m_packet )
        {
            throw std::runtime_error("failed to allocated memory for AVPacket");
        }
    }
    ~packet()
    {
        if( m_packet ) {
            av_packet_free( &m_packet );
            m_packet = nullptr;
        }
    }

    AVPacket* get() const
    {
        return m_packet;
    }

private:
    AVPacket* m_packet;
};


int encode_video(const input_stream_context& _decoder_context, const output_stream_context& _encoder_context, AVFrame *input_frame) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    av_init_packet(output_packet);

    if (!output_packet) { throw std::runtime_error("could not allocate memory for output packet"); }

    const auto& _decoder = _decoder_context.get_decoder();
    const auto& _encoder = _encoder_context.get_encoder();

    auto _encoder_video_codec_context = _encoder->get_video_stream()->get_av_codec_context();
    auto _encoder_video_stream = _encoder->get_video_stream()->get_av_stream();

    auto _decoder_video_stream = _decoder->get_video_stream()->get_av_stream();

    int response = avcodec_send_frame( _encoder_video_codec_context, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(_encoder_video_codec_context, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }

        output_packet->stream_index = _decoder->get_video_stream()->get_stream_index();
        output_packet->duration = _encoder_video_stream->time_base.den / _encoder_video_stream->time_base.num / _decoder_video_stream->avg_frame_rate.num * _decoder_video_stream->avg_frame_rate.den;

        av_packet_rescale_ts(output_packet, _decoder_video_stream->time_base, _encoder_video_stream->time_base);
        response = av_interleaved_write_frame( _encoder_context.get_stream_info()->get_format_context().get(), output_packet );
        if (response != 0 && response != AVERROR(EINVAL)) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int encode_audio(const input_stream_context& _decoder_context, const output_stream_context& _encoder_context, AVFrame *input_frame) {
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {throw std::runtime_error("could not allocate memory for output packet");}

    const auto& _decoder = _decoder_context.get_decoder();
    const auto& _encoder = _encoder_context.get_encoder();
    auto _encoder_audio_codec_context = _encoder->get_audio_stream()->get_av_codec_context();
    auto _encoder_audio_stream = _encoder->get_audio_stream()->get_av_stream();
    auto _decoder_audio_stream = _decoder->get_audio_stream()->get_av_stream();

    int response = avcodec_send_frame( _encoder_audio_codec_context, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet( _encoder_audio_codec_context, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }

        output_packet->stream_index = _decoder->get_audio_stream()->get_stream_index();

        av_packet_rescale_ts(output_packet, _decoder_audio_stream->time_base, _encoder_audio_stream->time_base);
        response = av_interleaved_write_frame( _encoder_context.get_stream_info()->get_format_context().get(), output_packet);
        if (response != 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int transcode_audio(const input_stream_context& _decoder_context,
                    const output_stream_context& _encoder_context_1,
                    const output_stream_context& _encoder_context_2,
                    const output_stream_context& _encoder_context_3,
                    AVPacket *input_packet, AVFrame *input_frame)
{
    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_audio_codec_context = _decoder->get_audio_stream()->get_av_codec_context();

    int response = avcodec_send_packet( _decoder_audio_codec_context, input_packet);
    if (response < 0) {
        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(_decoder_audio_codec_context, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
            return response;
        }

        if (response >= 0) {
            encode_audio(_decoder_context, _encoder_context_1, input_frame);
            encode_audio(_decoder_context, _encoder_context_2, input_frame);
            encode_audio(_decoder_context, _encoder_context_3, input_frame);
        }
        av_frame_unref(input_frame);
    }
    return 0;
}

int transcode_video(const input_stream_context& _decoder_context,
                    const output_stream_context& _encoder_context_1,
                    const output_stream_context& _encoder_context_2,
                    const output_stream_context& _encoder_context_3,
                    AVPacket *input_packet,
                    AVFrame* input_frame,
                    AVFrame* scaled_frame_1,
                    AVFrame* scaled_frame_2,
                    AVFrame* scaled_frame_3
                    )
{
    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_video_codec_context = _decoder->get_video_stream()->get_av_codec_context();

    int response = avcodec_send_packet(_decoder_video_codec_context, input_packet);
    if (response < 0){
        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(_decoder_video_codec_context, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0 && response != AVERROR(EINVAL) ) {
            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
            return response;
        }


        if (response >= 0) {
            {
                _encoder_context_1.get_scaler()->scale(input_frame, scaled_frame_1);
                encode_video(_decoder_context, _encoder_context_1, scaled_frame_1);
            }
            {
                _encoder_context_2.get_scaler()->scale( input_frame, scaled_frame_2 );
                encode_video(_decoder_context, _encoder_context_2, scaled_frame_2);
            }
            {
                _encoder_context_3.get_scaler()->scale( input_frame, scaled_frame_3 );
                encode_video(_decoder_context, _encoder_context_3, scaled_frame_3);
            }
        }

        av_frame_unref(input_frame);
    }

    return 0;
}


int to_hls( int argc, char *argv[] )
{
    try {
        int64_t pts = 0;
//        avcodec_register_all();

        const auto args = parse_arguments( argc, argv );
        input_stream_context _input_stream_context_1( args.front() );
        input_stream_context _input_stream_context_2( args[1] );

        const auto input_framerate = _input_stream_context_1.get_input_framerate();
        const auto input_samplerate = _input_stream_context_2.get_input_samplerate();


        scaling_options _scale_1 = { 1920, 1080, 1280, 720, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };
        scaling_options _scale_2 = { 1920, 1080, 840, 480, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };
        scaling_options _scale_3 = { 1920, 1080, 420, 240, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P };

        output_stream_context _output_stream_context_1( args[2], input_framerate, input_samplerate, _scale_1 );
        output_stream_context _output_stream_context_2( args[2], input_framerate, input_samplerate, _scale_2 );
        output_stream_context _output_stream_context_3( args[2], input_framerate, input_samplerate, _scale_3 );
//        output_stream_info _output_stream_info( args[2] );
//        encoder _encoder( _output_stream_info, input_framerate, input_samplerate );


//        auto _output_format_context = _output_stream_context.get_stream_info()->get_format_context();
//        auto _av_output_format_context = _output_format_context.get();

//        if (_av_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
//            _av_output_format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
//
//        if (!(_av_output_format_context->oformat->flags & AVFMT_NOFILE)) {
//            if (avio_open(&_av_output_format_context->pb, args[2].c_str(), AVIO_FLAG_WRITE) < 0) {
//                throw std::runtime_error("could not open the output file");
//                return -1;
//            }
//        }

        _output_stream_context_1.write_header();
        _output_stream_context_2.write_header();
        _output_stream_context_3.write_header();

        frame video_input_frame;
        frame audio_input_frame;
        packet video_input_packet;
        packet audio_input_packet;

        const auto& _format_context_1 = _input_stream_context_1.get_stream_info()->get_format_context();
        const auto& _format_context_2 = _input_stream_context_2.get_stream_info()->get_format_context();

        frame scaled_frame_1;
        frame scaled_frame_2;
        frame scaled_frame_3;

        bool no_video = false;
        bool no_audio = false;
        while( true )
        {
            if( av_read_frame( _format_context_1.get(), video_input_packet.get() ) >= 0 )
            {
                if( _format_context_1.get_stream_by_idx( video_input_packet.get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
                {
//                    transcode_video(_input_stream_context_1, _output_stream_context_1, video_input_packet.get(), video_input_frame_1.get() );
//                    transcode_video(_input_stream_context_1, _output_stream_context_2, video_input_packet.get(), video_input_frame_2.get() );
                    transcode_video(_input_stream_context_1,
                                    _output_stream_context_1,
                                    _output_stream_context_2,
                                    _output_stream_context_3,
                                    video_input_packet.get(),
                                    video_input_frame.get(),
                                    scaled_frame_1.get(),
                                    scaled_frame_2.get(),
                                    scaled_frame_3.get() );
                }
            }
            else
            {
                no_video = true;
            }
            if( av_read_frame( _format_context_2.get(), audio_input_packet.get() ) >= 0 )
            {
                if (_format_context_2.get_stream_by_idx(audio_input_packet.get()->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {
//                    transcode_audio(_input_stream_context_2, _output_stream_context_1, audio_input_packet.get(),  audio_input_frame_1.get());
//                    transcode_audio(_input_stream_context_2, _output_stream_context_2, audio_input_packet.get(),  audio_input_frame_2.get());
                    transcode_audio(_input_stream_context_2,  _output_stream_context_1, _output_stream_context_2, _output_stream_context_3, audio_input_packet.get(),  audio_input_frame.get());
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

        _output_stream_context_1.write_trailer();
        _output_stream_context_2.write_trailer();
        _output_stream_context_3.write_trailer();

    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Error: " << ex.what();
        return -1;
    }

    return 0;
}

struct hls_stream_encoder: public coder
{
public:
//    hls_stream_encoder(  const std::unique_ptr<output_stream_info>& _output_stream,
//                         const AVRational& _input_framerate,
//                         const AVRational& _input_samplerate,
//                         const scaling_options& _scaling_options_1,
//                         const scaling_options& _scaling_options_2,
//                         const scaling_options& _scaling_options_3 )
//    {
//        m_video_streams.insert( { 0, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_1 ) } );
//        m_video_streams.insert( { 1, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_2 ) } );
//        m_video_streams.insert( { 2, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_3 ) } );
//
//        m_audio_streams.insert( { 0, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_1 ) } );
//        m_audio_streams.insert( { 1, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_2 ) } );
//        m_audio_streams.insert( { 2, std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_3 ) } );
//
//        m_streams_count = 3;
//    }

    hls_stream_encoder( const std::unique_ptr< output_stream_info> & _output_stream_info,
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
//        m_video_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_1 ) );
//        m_video_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_2 ) );
//        m_video_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options_3 ) );
//        m_audio_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_1 ) );
//        m_audio_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_2 ) );
//        m_audio_streams.emplace_back( std::make_unique< output_stream >( _output_stream_info->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options_3 ) );
//        m_streams_count = 3;
    }

//    const std::unique_ptr< output_stream >& get_videostream_1() const
//    {
//        return m_video_stream_1;
//    }
//    const std::unique_ptr< output_stream >& get_videostream_2() const
//    {
//        return m_video_stream_2;
//    }
//    const std::unique_ptr< output_stream >& get_videostream_3() const
//    {
//        return m_video_stream_3;
//    }
//
//    const std::unique_ptr< output_stream >& get_audiostream_1() const
//    {
//        return m_audio_stream_1;
//    }
//    const std::unique_ptr< output_stream >& get_audiostream_2() const
//    {
//        return m_audio_stream_2;
//    }
//    const std::unique_ptr< output_stream >& get_audiostream_3() const
//    {
//        return m_audio_stream_3;
//    }

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


//    std::unique_ptr< output_stream > m_video_stream_1;
//    std::unique_ptr< output_stream > m_video_stream_2;
//    std::unique_ptr< output_stream > m_video_stream_3;
//
//    std::unique_ptr< output_stream > m_audio_stream_1;
//    std::unique_ptr< output_stream > m_audio_stream_2;
//    std::unique_ptr< output_stream > m_audio_stream_3;
};


struct hls_stream_context
{
public:
    hls_stream_context( const std::string& output_path,
                        const AVRational& _input_framerate,
                        const AVRational& _input_samplerate,
                        const std::vector< scaling_options > _scaling_options )
            : m_scaling_options( _scaling_options )
            , m_output_path( output_path )
    {
        for( const auto& _scaling_option : m_scaling_options )
        {
            m_scalers.emplace_back( std::make_unique< video_scaler >( _scaling_option ) );
        }

//        m_filename_prefix = std::to_string( m_scaling_options.target_height );
        m_filename_pattern =  "v%v/fileSequence%d.ts";
        m_filename_playlist = "v%v/prog_index.m3u8";
        m_filename_master_playlist = "master.m3u8";

        std::filesystem::path path(m_output_path);
        m_absolute_path = path / m_filename_playlist;

//        std::vector< scaling_options > _scaling_options;
//        _scaling_options.push_back( _scaling_options_1 );
//        _scaling_options.push_back( _scaling_options_2 );
//        _scaling_options.push_back( _scaling_options_3 );

        m_stream_info = std::make_unique< output_stream_info >( m_absolute_path );
        m_encoder = std::make_unique< hls_stream_encoder >( m_stream_info, _input_framerate, _input_samplerate, _scaling_options );

        auto _av_output_format_context =  m_stream_info->get_format_context().get();

        if (_av_output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
            _av_output_format_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        if (!(_av_output_format_context->oformat->flags & AVFMT_NOFILE)) {
            if (avio_open(&_av_output_format_context->pb, m_absolute_path.c_str(), AVIO_FLAG_WRITE) < 0) {
                throw std::runtime_error("could not open the output file");
            }
        }
    }

    void write_header()
    {
        AVDictionary* headerOptions(0);
//        av_dict_set(&headerOptions, "segment_format", "mpegts", 0);
//        av_dict_set(&headerOptions, "segment_list_type", "m3u8", 0);
//        av_dict_set(&headerOptions, "segment_list", args[2].c_str(), 0);
//        av_dict_set_int(&headerOptions, "segment_list_size", 0, 0);
//        av_dict_set(&headerOptions, "segment_time_delta", "1.00", 0);
//        av_dict_set(&headerOptions, "segment_time", "10", 0);
//        av_dict_set_int(&headerOptions, "reference_stream", _input_stream_context_1.get_decoder()->get_video_stream()->get_stream_index(), 0);
//        av_dict_set(&headerOptions, "segment_list_flags", "cache+live", 0);
        av_dict_set(&headerOptions, "hls_segment_type", "mpegts", 0);
        av_dict_set(&headerOptions, "hls_playlist_type", "vod", 0);
        av_dict_set_int(&headerOptions, "hls_list_size", 0, 0);
        av_dict_set(&headerOptions, "segment_time_delta", "0.5", 0);
        av_dict_set(&headerOptions, "hls_flags", "independent_segments", 0);
        av_dict_set(&headerOptions, "hls_flags", "program_date_time", 0);
        av_dict_set(&headerOptions, "hls_flags", "iframes_only", 0);
        av_dict_set(&headerOptions, "hls_time", "6", 0);
        av_dict_set(&headerOptions, "hls_segment_filename", m_filename_pattern.c_str(), 0);
        av_dict_set(&headerOptions, "master_pl_name", m_filename_master_playlist.c_str(), 0);

        std::stringstream _streams_mapping;
        for( int i = 0; i < m_scaling_options.size(); ++i )
        {
            _streams_mapping << "v:" << i << ",a:" << i << " ";
        }

//        av_dict_set(&headerOptions, "var_stream_map", "v:0,a:0 v:1,a:1 v:2,a:2", 0);
        av_dict_set(&headerOptions, "var_stream_map", _streams_mapping.str().c_str(), 0);

        auto _av_output_format_context =  m_stream_info->get_format_context().get();

        /* init muxer, write output file header */
        auto ret = avformat_write_header( _av_output_format_context, &headerOptions );
        if (ret < 0) {
            throw std::runtime_error( "Error occurred when opening output file" );
        }
    }

    void write_trailer()
    {
        av_write_trailer( m_stream_info->get_format_context().get() );
    }

    const std::unique_ptr< hls_stream_encoder >& get_encoder() const
    {
        return m_encoder;
    }

    const std::unique_ptr< output_stream_info >& get_stream_info() const
    {
        return m_stream_info;
    }

//    const std::unique_ptr< video_scaler >& get_scaler_1() const
//    {
//        return m_scaler_1;
//    }
//    const std::unique_ptr< video_scaler >& get_scaler_2() const
//    {
//        return m_scaler_2;
//    }
//    const std::unique_ptr< video_scaler >& get_scaler_3() const
//    {
//        return m_scaler_3;
//    }

    using video_scaler_ptr = std::unique_ptr< video_scaler >;
    using video_scalers = std::vector< std::unique_ptr< video_scaler > >;

    const video_scaler_ptr& get_video_scaler_by_index( hls_stream_encoder::stream_index idx ) const
    {
        return m_scalers.at( idx );
    }

private:
    std::unique_ptr< output_stream_info > m_stream_info;
    std::unique_ptr< hls_stream_encoder > m_encoder;
//    scaling_options m_scaling_options_1;
//    scaling_options m_scaling_options_2;
//    scaling_options m_scaling_options_3;
    std::string m_filename_prefix;
    std::string m_filename_pattern;
    std::string m_filename_playlist;
    std::string m_filename_master_playlist;
    std::string m_output_path;
    std::string m_absolute_path;
//    std::unique_ptr< video_scaler > m_scaler_1;
//    std::unique_ptr< video_scaler > m_scaler_2;
//    std::unique_ptr< video_scaler > m_scaler_3;

    std::vector< scaling_options > m_scaling_options;
    video_scalers m_scalers;
};

int encode_video_hls(const input_stream_context& _decoder_context, const std::unique_ptr< output_stream>& _video_stream, const std::unique_ptr< output_stream_info >& _stream_info, AVFrame *input_frame, int _stream_index) {
    if (input_frame) input_frame->pict_type = AV_PICTURE_TYPE_NONE;

    AVPacket *output_packet = av_packet_alloc();
    av_init_packet(output_packet);

    if (!output_packet) { throw std::runtime_error("could not allocate memory for output packet"); }

    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_video_stream = _decoder->get_video_stream()->get_av_stream();

    auto _encoder_video_codec_context = _video_stream->get_av_codec_context();
    auto _encoder_video_stream = _video_stream->get_av_stream();


    int response = avcodec_send_frame( _encoder_video_codec_context, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet(_encoder_video_codec_context, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }

        output_packet->stream_index = _stream_index;
        output_packet->duration = _encoder_video_stream->time_base.den / _encoder_video_stream->time_base.num / _decoder_video_stream->avg_frame_rate.num * _decoder_video_stream->avg_frame_rate.den;

        av_packet_rescale_ts(output_packet, _decoder_video_stream->time_base, _encoder_video_stream->time_base);
        response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet );
        if (response != 0 && response != AVERROR(EINVAL)) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int transcode_video_hls(const input_stream_context& _decoder_context,
                    const hls_stream_context& _encoder_context,
                    AVPacket *input_packet,
                    AVFrame* input_frame,
                    const std::vector< std::unique_ptr< frame > >& _scaling_frames )
{
    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_video_codec_context = _decoder->get_video_stream()->get_av_codec_context();


    const auto& _encoder = _encoder_context.get_encoder();

    int response = avcodec_send_packet(_decoder_video_codec_context, input_packet);
    if (response < 0){
        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(_decoder_video_codec_context, input_frame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0 && response != AVERROR(EINVAL) ) {
            throw std::runtime_error("Error while receiving frame from decoder: [" + helpers::error2string(response) + "]");
            return response;
        }


        const int stream_index = _decoder->get_video_stream()->get_stream_index();
        if (response >= 0) {

//            std::vector< AVFrame* > _scaled_frames;
//            _scaled_frames.push_back( scaled_frame_1 );
//            _scaled_frames.push_back( scaled_frame_2 );
//            _scaled_frames.push_back( scaled_frame_3 );
            for( hls_stream_encoder::stream_index _idx = 0; _idx < _encoder->get_streams_count(); ++_idx )
            {
                auto _scaling_frame = _scaling_frames.at( _idx )->get();
                _encoder_context.get_video_scaler_by_index( _idx )->scale( input_frame, _scaling_frame );
                encode_video_hls(_decoder_context, _encoder->get_video_stream( _idx ), _encoder_context.get_stream_info(),_scaling_frame, stream_index + _idx );
            }

//            {
//                _encoder_context.get_video_scaler_by_index( 0 )->scale(input_frame, scaled_frame_1);
//                encode_video_hls(_decoder_context, _encoder->get_video_stream( 0 ), _encoder_context.get_stream_info(), scaled_frame_1, stream_index);
//            }
//            {
//                _encoder_context.get_video_scaler_by_index( 1 )->scale(input_frame, scaled_frame_2);
//                encode_video_hls(_decoder_context,  _encoder->get_video_stream( 1 ), _encoder_context.get_stream_info(), scaled_frame_2, stream_index + 1);
//            }
//            {
//                _encoder_context.get_video_scaler_by_index( 2 )->scale(input_frame, scaled_frame_3);
//                encode_video_hls(_decoder_context,  _encoder->get_video_stream( 2 ), _encoder_context.get_stream_info(), scaled_frame_3, stream_index + 2);
//            }
        }

        av_frame_unref(input_frame);
    }

    return 0;
}

int encode_audio_hls(const input_stream_context& _decoder_context, const std::unique_ptr< output_stream >& _audio_stream, const std::unique_ptr< output_stream_info >& _stream_info, AVFrame *input_frame, int _stream_index) {
    AVPacket *output_packet = av_packet_alloc();
    if (!output_packet) {throw std::runtime_error("could not allocate memory for output packet");}

    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_audio_stream = _decoder->get_audio_stream()->get_av_stream();

    auto _encoder_audio_codec_context = _audio_stream->get_av_codec_context();
    auto _encoder_audio_stream = _audio_stream->get_av_stream();

    int response = avcodec_send_frame( _encoder_audio_codec_context, input_frame);

    while (response >= 0) {
        response = avcodec_receive_packet( _encoder_audio_codec_context, output_packet);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        } else if (response < 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }

        output_packet->stream_index = _stream_index;

        av_packet_rescale_ts(output_packet, _decoder_audio_stream->time_base, _encoder_audio_stream->time_base);
        response = av_interleaved_write_frame( _stream_info->get_format_context().get(), output_packet);
        if (response != 0) {
            throw std::runtime_error("Error while receiving packet from encoder: [" + helpers::error2string(response) + "]");
            return -1;
        }
    }
    av_packet_unref(output_packet);
    av_packet_free(&output_packet);
    return 0;
}

int transcode_audio_hls(const input_stream_context& _decoder_context,
                    const hls_stream_context& _encoder_context,
                    AVPacket *input_packet, AVFrame *input_frame)
{
    const auto& _decoder = _decoder_context.get_decoder();
    auto _decoder_audio_codec_context = _decoder->get_audio_stream()->get_av_codec_context();

    const auto& _encoder = _encoder_context.get_encoder();

    int response = avcodec_send_packet( _decoder_audio_codec_context, input_packet);
    if (response < 0) {
        throw std::runtime_error("Error while sending packet to decoder: [" + helpers::error2string(response) + "]");
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(_decoder_audio_codec_context, input_frame);
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
                encode_audio_hls(_decoder_context, _encoder->get_audio_stream( _idx ), _encoder_context.get_stream_info(), input_frame, _stream_index + _idx);
            }

//            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_1(), _encoder_context.get_stream_info(), input_frame, _stream_index + 2);
//            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_2(), _encoder_context.get_stream_info(), input_frame, _stream_index + 3);
//            encode_audio_hls(_decoder_context, _encoder_context.get_encoder()->get_audiostream_3(), _encoder_context.get_stream_info(), input_frame, _stream_index + 4);
        }
        av_frame_unref(input_frame);
    }
    return 0;
}

int to_hls2( int argc, char *argv[] )
{
    try {
        int64_t pts = 0;
//        avcodec_register_all();

        const auto args = parse_arguments(argc, argv);
        input_stream_context _input_stream_context_1(args.front());
        input_stream_context _input_stream_context_2(args[1]);

        const auto input_framerate = _input_stream_context_1.get_input_framerate();
        const auto input_samplerate = _input_stream_context_2.get_input_samplerate();



        scaling_options _scale_1 = { 1920, 1080, 1280, 720, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 2 * 1000 * 1000 };
        scaling_options _scale_2 = { 1920, 1080, 840, 480, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 1 * 1000 * 1000 };
        scaling_options _scale_3 = { 1920, 1080, 420, 240, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 500 * 1000 };
        scaling_options _scale_4 = { 1920, 1080, 1920, 1080, AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, 10 * 1000 * 1000 };

        const auto _scalings = { _scale_1, _scale_2, _scale_3, _scale_4 };


        hls_stream_context _hls_stream_context( args[2], input_framerate, input_samplerate, _scalings );

        _hls_stream_context.write_header();

        frame video_input_frame;
        frame audio_input_frame;
        packet video_input_packet;
        packet audio_input_packet;

        const auto& _format_context_1 = _input_stream_context_1.get_stream_info()->get_format_context();
        const auto& _format_context_2 = _input_stream_context_2.get_stream_info()->get_format_context();

        std::vector< std::unique_ptr< frame > > _scaling_frames;
        _scaling_frames.reserve( _scalings.size() );
        for( int i = 0; i < _scalings.size(); ++i )
        {
            _scaling_frames.emplace_back( std::make_unique< frame >() );
        }
//        frame scaled_frame_1;
//        frame scaled_frame_2;
//        frame scaled_frame_3;

        bool no_video = false;
        bool no_audio = false;
        while( true )
        {
            if( av_read_frame( _format_context_1.get(), video_input_packet.get() ) >= 0 )
            {
                if( _format_context_1.get_stream_by_idx( video_input_packet.get()->stream_index )->codecpar->codec_type == AVMEDIA_TYPE_VIDEO )
                {
//                    transcode_video(_input_stream_context_1, _output_stream_context_1, video_input_packet.get(), video_input_frame_1.get() );
//                    transcode_video(_input_stream_context_1, _output_stream_context_2, video_input_packet.get(), video_input_frame_2.get() );
                    transcode_video_hls(_input_stream_context_1,
                                        _hls_stream_context,
                                    video_input_packet.get(),
                                    video_input_frame.get(),
                                    _scaling_frames );
                }
            }
            else
            {
                no_video = true;
            }
            if( av_read_frame( _format_context_2.get(), audio_input_packet.get() ) >= 0 )
            {
                if (_format_context_2.get_stream_by_idx(audio_input_packet.get()->stream_index)->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
                {
//                    transcode_audio(_input_stream_context_2, _output_stream_context_1, audio_input_packet.get(),  audio_input_frame_1.get());
//                    transcode_audio(_input_stream_context_2, _output_stream_context_2, audio_input_packet.get(),  audio_input_frame_2.get());
                    transcode_audio_hls(_input_stream_context_2, _hls_stream_context, audio_input_packet.get(),  audio_input_frame.get());
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


        _hls_stream_context.write_trailer();

    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Error: " << ex.what();
        return -1;
    }

    return 0;
}

int main( int argc, char *argv[] )
{
    return to_hls2( argc, argv );

    try {
    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Error: " << ex.what();
        return -1;
    }

    return 0;
}