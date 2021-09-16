//
// Created by degree on 17.09.2021.
//

#ifndef HLS_ENCODER_VIDEO_SCALER_H
#define HLS_ENCODER_VIDEO_SCALER_H


struct video_scaler
{
    video_scaler( const scaling_options& _scaling_options )
            : m_scaling_options( _scaling_options )
    {
        m_scaler = sws_getContext(
                m_scaling_options.source_width,
                m_scaling_options.source_height,
                m_scaling_options.source_pixel_format,
                m_scaling_options.target_width,
                m_scaling_options.target_height,
                m_scaling_options.target_pixel_format,
                SWS_BICUBIC,
                NULL,
                NULL,
                NULL
        );
    }

    ~video_scaler()
    {
        sws_freeContext(m_scaler);
    }

    SwsContext* get()
    {
        return m_scaler;
    }

    void scale( const std::unique_ptr< frame >& input_frame, const std::unique_ptr< frame >& scaled_frame ) const
    {
        scaled_frame->get()->format = m_scaling_options.target_pixel_format;
        scaled_frame->get()->width  = m_scaling_options.target_width;
        scaled_frame->get()->height = m_scaling_options.target_height;

        av_image_alloc(
                scaled_frame->get()->data,
                scaled_frame->get()->linesize,
                scaled_frame->get()->width,
                scaled_frame->get()->height,
                m_scaling_options.target_pixel_format,
                16);

        sws_scale( m_scaler,
                   (const uint8_t * const*) input_frame->get()->data, input_frame->get()->linesize,
                   0, m_scaling_options.source_height,
                   scaled_frame->get()->data, scaled_frame->get()->linesize);

        scaled_frame->get()->quality = input_frame->get()->quality;
        scaled_frame->get()->pts = input_frame->get()->pts;
    }

private:
    scaling_options m_scaling_options;
    SwsContext* m_scaler;
};

#endif //HLS_ENCODER_VIDEO_SCALER_H
