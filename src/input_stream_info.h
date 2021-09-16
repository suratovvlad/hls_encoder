//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_INPUT_STREAM_INFO_H
#define HLS_ENCODER_INPUT_STREAM_INFO_H

#include "libav_include.h"
#include "format_context.h"

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


#endif //HLS_ENCODER_INPUT_STREAM_INFO_H
