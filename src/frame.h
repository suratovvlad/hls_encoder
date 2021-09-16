//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_FRAME_H
#define HLS_ENCODER_FRAME_H

#include "libav_include.h"

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

#endif //HLS_ENCODER_FRAME_H
