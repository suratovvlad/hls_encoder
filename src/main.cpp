//
// Created by degree on 11.09.2021.
//

#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>

#include "hls_encoder.h"

std::vector< std::string > parse_arguments( int argc, char *argv[] )
{
    auto args = std::vector< std::string >{};
    args.reserve( argc );

    if( argc < 2 )
    {
        throw std::runtime_error( "Provide video input file name with first argument" );
    }

    if( argc < 3 )
    {
        throw std::runtime_error( "Provide audio input file name with second argument" );
    }

    if( argc < 4 )
    {
        throw std::runtime_error( "Provide video output file name with third argument" );
    }

    for( int i = 1; i < argc; ++i )
    {
        args.push_back( argv[i] );
    }

    return args;
}

int main( int argc, char *argv[] )
{
    try {
        const auto _args = parse_arguments(argc, argv);
        const auto _video_src = _args.at( 0 );
        const auto _audio_src = _args.at( 1 );
        const auto _output_dir = _args.at( 2 );

        hls_output_filenames _output_filenames = {
                "media_stream_%v/segment_%03d.ts",
                "media_stream_%v/media.m3u8",
                "master.m3u8"
        };

        auto _hls_encoder = std::make_unique< hls_encoder >( _video_src, _audio_src, _output_dir, _output_filenames );

        constexpr auto AUDIO_BITRATE = 128000;

        // TODO: fill this with arguments to application
        scaling_options _scale_1 = {
                1920, 1080,                             // source
                1280, 720,                              // target
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P, // pixel formats of source and target
                2 * 1000 * 1000 - AUDIO_BITRATE,        // bitrate, 128000 - audio bitrate
                60,                                     // GOP size
                1                                       // max_b_frames
        };
        scaling_options _scale_2 = {
                1920, 1080,
                840, 480,
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P,
                1 * 1000 * 1000 - AUDIO_BITRATE,
                60,
                1
        };
        scaling_options _scale_3 = {
                1920, 1080,
                420, 240,
                AV_PIX_FMT_YUV420P, AV_PIX_FMT_YUV420P,
                500 * 1000 - AUDIO_BITRATE,
                60,
                1
        };

        _hls_encoder->add_scaling_option( _scale_1 );
        _hls_encoder->add_scaling_option( _scale_2 );
        _hls_encoder->add_scaling_option( _scale_3 );

        if( _hls_encoder->initialize_input_output() )
        {
           if( _hls_encoder->process_transcoding() )
               std::cout << " Transcoding has succeeded!";
           else
               std::cout << " Transcoding has failed! ";
        }
        else
        {
            std::cout << " Initialization failed! ";
        }
    }
    catch ( const std::runtime_error& ex )
    {
        std::cout << "Runtime error: " << ex.what();
        return -1;
    }
    catch ( const std::exception& ex )
    {
        std::cout << "Error: " << ex.what();
        return -1;
    }
    catch ( ... )
    {
        std::cout << " Something really bad happened ";
        return -1;
    }

    return 0;
}