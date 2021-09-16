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

struct format_context final
{
public:
    format_context()
    {
        m_format_context = avformat_alloc_context();
        if( !m_format_context )
        {
            throw std::runtime_error( "Cannot allocate format context" );
        }
    }

    AVFormatContext* get() const
    {
        return m_format_context;
    }

    unsigned number_of_streams() const
    {
        return m_format_context->nb_streams;
    };

    AVStream* get_stream_by_idx( unsigned idx ) const
    {
        const auto& count = m_format_context->nb_streams;
        if( idx >= count )
        {
            throw std::runtime_error( "Wrong stream index: [" + std::to_string( idx ) + "] of [" + std::to_string( count ) + "]" );
        }
        return m_format_context->streams[ idx ];
    }

    AVFormatContext** get_pointer()
    {
        return &m_format_context;
    }

    ~format_context()
    {
        if( m_format_context ) {
            avformat_free_context( m_format_context );
            m_format_context = nullptr;
        }
    }

private:
    AVFormatContext* m_format_context;
};

struct input_stream_info
{
public:
    input_stream_info( const std::string& filename )
    {
        auto ret = avformat_open_input( m_format_context.get_pointer(), filename.c_str(), nullptr, nullptr );
        if( ret != 0 )
        {
            throw std::runtime_error( "Failed to open input file: [" + filename + "]. Error: [" + helpers::error2string( ret ) + "] " );
        }

        ret = avformat_find_stream_info( m_format_context.get(), nullptr );
        if( ret < 0 )
        {
            throw std::runtime_error( "Failed to get stream info: [" + filename + "]. Error: [" + helpers::error2string( ret ) + "] " );
        }

        av_dump_format( m_format_context.get(), 0, filename.c_str(), 0 );
    }

    const format_context& get_format_context() const
    {
        return m_format_context;
    }

    ~input_stream_info()
    {
        avformat_close_input( m_format_context.get_pointer() );
    }
private:
    format_context m_format_context;
};

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

struct coder
{
public:

    coder() = default;
    virtual ~coder() = default;

    const std::unique_ptr< stream >& get_video_stream() const
    {
        return m_video_stream;
    }

    const std::unique_ptr< stream >& get_audio_stream() const
    {
        return m_audio_stream;
    }

protected:
    std::unique_ptr< stream > m_video_stream;
    std::unique_ptr< stream > m_audio_stream;
};

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

struct input_stream_context
{
public:
    input_stream_context( const std::string& filename )
    {
        m_stream_info = std::make_unique< input_stream_info >( filename );
        m_decoder = std::make_unique< decoder >( m_stream_info );
    }

    AVRational get_input_framerate()
    {
        return av_guess_frame_rate( m_stream_info->get_format_context().get(),
                                 m_decoder->get_video_stream()->get_av_stream(),
                                  nullptr );
    }

    AVRational get_input_samplerate()
    {
        return AVRational{ 1, m_decoder->get_audio_stream()->get_av_codec_context()->sample_rate };
    }

    const std::unique_ptr< decoder >& get_decoder() const
    {
        return m_decoder;
    }

    const std::unique_ptr< input_stream_info >& get_stream_info() const
    {
        return m_stream_info;
    }


private:
    std::unique_ptr< input_stream_info > m_stream_info;
    std::unique_ptr< decoder > m_decoder;
};


struct output_stream_info
{
public:
    output_stream_info( const std::string& filename )
    {
        auto ret = avformat_alloc_output_context2( m_format_context.get_pointer(), nullptr, NULL, filename.c_str() );
        if( ret != 0 )
        {
            throw std::runtime_error( "Failed to open output file: [" + filename + "]. Error: [" + helpers::error2string( ret ) + "] " );
        }
//
//        ret = avformat_find_stream_info( m_format_context.get(), nullptr );
//        if( ret < 0 )
//        {
//            throw std::runtime_error( "Failed to get stream info: [" + filename + "]. Error: [" + helpers::error2string( ret ) + "] " );
//        }
//
//        av_dump_format( m_format_context.get(), 0, filename.c_str(), 0 );
    }

    const format_context& get_format_context() const
    {
        return m_format_context;
    }

    ~output_stream_info()
    {
//        avformat_close_input( m_format_context.get_pointer() );
    }
private:
    format_context m_format_context;
};


struct scaling_options
{
    int source_width = 0;
    int source_height = 0;
    int target_width = 0;
    int target_height = 0;
    AVPixelFormat source_pixel_format = AV_PIX_FMT_YUV420P;
    AVPixelFormat target_pixel_format = AV_PIX_FMT_YUV420P;
    int64_t bit_rate = 0;

    scaling_options() =default;
    scaling_options( const scaling_options& ) =default;
    scaling_options( scaling_options&& )= default;

    scaling_options& operator=( const scaling_options& ) =default;
    scaling_options& operator=( scaling_options&& )= default;
    ~scaling_options() = default;
};

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

struct encoder : public coder
{
public:
    encoder( const std::unique_ptr<output_stream_info>& _output_stream, const AVRational& _input_framerate, const AVRational& _input_samplerate, const scaling_options& _scaling_options )
    {
        m_video_stream = std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::video, _input_framerate, _scaling_options );
        m_audio_stream = std::make_unique< output_stream >( _output_stream->get_format_context(), stream::stream_type::audio, _input_samplerate, _scaling_options );
    }
};



struct video_scaler
{
    video_scaler( const scaling_options& _scaling_options )
            : m_scaling_options( _scaling_options )
    {
        m_scaler = sws_getContext(
                m_scaling_options.source_width,
                m_scaling_options.source_height,
                m_scaling_options.source_pixel_format,
                m_scaling_options.target_width,
                m_scaling_options.target_height,
                m_scaling_options.target_pixel_format,
                SWS_BICUBIC,
                NULL,
                NULL,
                NULL
        );
    }

    ~video_scaler()
    {
        sws_freeContext(m_scaler);
    }

    SwsContext* get()
    {
        return m_scaler;
    }

    void scale( const AVFrame* input_frame, AVFrame* scaled_frame ) const
    {
        scaled_frame->format = m_scaling_options.target_pixel_format;
        scaled_frame->width  = m_scaling_options.target_width;
        scaled_frame->height = m_scaling_options.target_height;

        av_image_alloc(
                scaled_frame->data,
                scaled_frame->linesize,
                scaled_frame->width,
                scaled_frame->height,
                m_scaling_options.target_pixel_format,
                16);

        sws_scale( m_scaler,
                   (const uint8_t * const*) input_frame->data, input_frame->linesize,
                   0, m_scaling_options.source_height,
                   scaled_frame->data, scaled_frame->linesize);

        scaled_frame->quality = input_frame->quality;
        scaled_frame->pts = input_frame->pts;
    }

private:
    scaling_options m_scaling_options;
    SwsContext* m_scaler;
};


struct output_stream_context
{
public:
    output_stream_context( const std::string& output_path, const AVRational& _input_framerate, const AVRational& _input_samplerate, const scaling_options& _scaling_options )
        : m_scaling_options( _scaling_options )
        , m_output_path( output_path )
    {
        m_scaler = std::make_unique< video_scaler >( m_scaling_options );
        m_filename_prefix = std::to_string( m_scaling_options.target_height );
        m_filename_pattern = m_filename_prefix + "p_%03d.ts";
        m_filename_playlist = m_filename_prefix + "p.m3u8";

        std::filesystem::path path(m_output_path);
        m_absolute_path = path / m_filename_playlist;


        m_stream_info = std::make_unique< output_stream_info >( m_absolute_path );
        m_encoder = std::make_unique< encoder >( m_stream_info, _input_framerate, _input_samplerate, _scaling_options );

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
        av_dict_set(&headerOptions, "segment_time_delta", "1.0", 0);
        av_dict_set(&headerOptions, "hls_flags", "append_list", 0);
        av_dict_set(&headerOptions, "hls_time", "10", 0);
        av_dict_set(&headerOptions, "hls_segment_filename", m_filename_pattern.c_str(), 0);

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

    const std::unique_ptr< encoder >& get_encoder() const
    {
        return m_encoder;
    }

    const std::unique_ptr< output_stream_info >& get_stream_info() const
    {
        return m_stream_info;
    }

    const std::unique_ptr< video_scaler >& get_scaler() const
    {
        return m_scaler;
    }

private:
    std::unique_ptr< output_stream_info > m_stream_info;
    std::unique_ptr< encoder > m_encoder;
    scaling_options m_scaling_options;
    std::string m_filename_prefix;
    std::string m_filename_pattern;
    std::string m_filename_playlist;
    std::string m_output_path;
    std::string m_absolute_path;
    std::unique_ptr< video_scaler > m_scaler;
};


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

public:

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
                    auto _scaling_frame = _scaling_frames.at( _idx )->get();
                    m_hls_stream_context->get_video_scaler_by_index( _idx )->scale( input_frame->get(), _scaling_frame );
                    encode_video( _encoder->get_video_stream( _idx ), _scaling_frames.at( _idx ), stream_index + _idx );
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
