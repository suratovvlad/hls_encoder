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

struct hls_encoder
{
public:
    hls_encoder( const std::string& video_src, const std::string& audio_src, const std::string& output_dir, const hls_output_filenames& filenames );

    bool initialize_input_output();

    void add_scaling_option( const scaling_options& _scaling_option );

    bool process_transcoding();

private:

    int encode_video( const std::unique_ptr< output_stream>& _video_stream, const std::unique_ptr< frame >& input_frame, int _stream_index );

    int transcode_video( const std::unique_ptr< packet >& input_packet, const std::unique_ptr< frame >& input_frame,
                         const std::vector< std::unique_ptr< frame > >& _scaling_frames );

    int encode_audio( const std::unique_ptr< output_stream >& _audio_stream, const std::unique_ptr< frame >& input_frame, int _stream_index );

    int transcode_audio( const std::unique_ptr< packet >& input_packet, const std::unique_ptr< frame >& input_frame );

    std::string m_video_src;
    std::string m_audio_src;
    std::string m_output_dir;
    std::vector< scaling_options > m_scaling_options;
    hls_output_filenames m_output_filenames;

    std::unique_ptr< input_stream_context > m_video_src_input_context;
    std::unique_ptr< input_stream_context > m_audio_src_input_context;
    std::unique_ptr< hls_stream_context > m_hls_stream_context;
    bool m_initialized = false;
};


#endif //HLS_ENCODER_HLS_ENCODER_H
