//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_CODER_H
#define HLS_ENCODER_CODER_H

#include "libav_include.h"
#include "stream.h"
#include <memory>

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

#endif //HLS_ENCODER_CODER_H
