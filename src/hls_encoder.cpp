//
// Created by degree on 11.09.2021.
//

#include "hls_encoder.h"

hls_encoder::hls_encoder( const std::string &video_src, const std::string &audio_src, const std::string& output_dir, const hls_output_filenames& filenames )
    : m_video_src( video_src )
    , m_audio_src( audio_src )
    , m_output_dir( output_dir )
    , m_output_filenames( filenames )
{
}

bool hls_encoder::initialize_input_output()
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
        m_hls_stream_context = std::make_unique< hls_stream_context >( m_output_dir, m_output_filenames, input_framerate, input_samplerate, m_scaling_options );

        m_initialized = true;
    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Error: " << ex.what();
        return false;
    }
    return true;
}

void hls_encoder::add_scaling_option(const scaling_options &_scaling_option)
{
    m_scaling_options.push_back( _scaling_option );
}

bool hls_encoder::process_transcoding()
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

int hls_encoder::encode_video(const std::unique_ptr<output_stream> &_video_stream,
                              const std::unique_ptr<frame> &input_frame, int _stream_index)
{

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

int hls_encoder::transcode_video(const std::unique_ptr<packet> &input_packet, const std::unique_ptr<frame> &input_frame,
                                 const std::vector<std::unique_ptr<frame>> &_scaling_frames)
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

int hls_encoder::encode_audio(const std::unique_ptr<output_stream> &_audio_stream,
                              const std::unique_ptr<frame> &input_frame, int _stream_index)
{

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

int hls_encoder::transcode_audio(const std::unique_ptr<packet> &input_packet, const std::unique_ptr<frame> &input_frame)
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

