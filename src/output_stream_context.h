//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_OUTPUT_STREAM_CONTEXT_H
#define HLS_ENCODER_OUTPUT_STREAM_CONTEXT_H

#include "libav_include.h"
#include "format_context.h"
#include "scaling_options.h"
#include "video_scaler.h"
#include "output_stream_info.h"


#include <filesystem>
#include <vector>


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

#endif //HLS_ENCODER_OUTPUT_STREAM_CONTEXT_H
