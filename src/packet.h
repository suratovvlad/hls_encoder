//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_PACKET_H
#define HLS_ENCODER_PACKET_H

#include "libav_include.h"

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

#endif //HLS_ENCODER_PACKET_H
