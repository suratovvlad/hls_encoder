//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_OUTPUT_STREAM_INFO_H
#define HLS_ENCODER_OUTPUT_STREAM_INFO_H

#include "libav_include.h"
#include "format_context.h"

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
    }

    const format_context& get_format_context() const
    {
        return m_format_context;
    }

    ~output_stream_info() = default;
private:
    format_context m_format_context;
};

#endif //HLS_ENCODER_OUTPUT_STREAM_INFO_H
