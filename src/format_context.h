//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_FORMAT_CONTEXT_H
#define HLS_ENCODER_FORMAT_CONTEXT_H

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

#endif //HLS_ENCODER_FORMAT_CONTEXT_H
