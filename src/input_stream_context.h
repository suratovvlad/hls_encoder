//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_INPUT_STREAM_CONTEXT_H
#define HLS_ENCODER_INPUT_STREAM_CONTEXT_H

#include "libav_include.h"
#include "input_stream_info.h"
#include "decoder.h"

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

#endif //HLS_ENCODER_INPUT_STREAM_CONTEXT_H
