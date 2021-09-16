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

struct hls_encoder {
public:
    hls_encoder( const std::string& video_src, const std::string& audio_src );


private:
    std::string m_video_src;
    std::string m_audio_src;
};


#endif //HLS_ENCODER_HLS_ENCODER_H
