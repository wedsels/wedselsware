#include "audio.hpp"

#define CURSOR do { if ( ::cursor += 1.0 / device->sampleRate >= ::Playing.Duration ) break; } while ( 0 )

void Seek( int time ) {
    if ( ::Playing.AudioStream < 0 || !::Playing.Format || !::Playing.Codec )
        return;

    ::std::lock_guard lock( ::SeekMutex );

    ::cursor = ::std::clamp( time, 0, ( int )::Playing.Duration );
    ::int64_t stamp = ::std::max( ( ::int64_t )1, ::av_rescale_q( ::cursor * 1e6, AV_TIME_BASE_Q, ::Playing.Timebase ) );

    ::avformat_seek_file( ::Playing.Format, ::Playing.AudioStream, INT64_MIN, stamp, stamp, AVSEEK_FLAG_BACKWARD );
    ::avcodec_flush_buffers( ::Playing.Codec );

    ::int64_t pts = -1;
    while ( pts < stamp ) {
        if ( ::av_read_frame( ::Playing.Format, ::Playing.Packet ) < 0 )
            break;

        ::avcodec_send_packet( ::Playing.Codec, ::Playing.Packet );

        while ( ::avcodec_receive_frame( ::Playing.Codec, ::Playing.Frame ) == 0 ) {
            if ( ::Playing.Frame->pts != AV_NOPTS_VALUE )
                pts = Playing.Frame->pts;
            else
                pts += 1;

            if ( pts >= stamp )
                break;

            ::av_frame_unref( ::Playing.Frame );
        }

        ::av_packet_unref( ::Playing.Packet );
    }
}

void Waveform( float* data, ::uint32_t frames ) {
    static const double alpha = 0.33;
    static const int size = WINWIDTH / 2;
    static double smooth[ size ];

    double samples = ( double )frames / size;

    for ( int i = 0; i < size; i++ ) {
        int start = i * samples;
        int end = ::std::clamp( ( int )( ( i + 1 ) * samples ), start + 1, ( int )frames );

        double peak = 0.0;
        for ( int j = start; j < end && j < frames; ++j ) {
            peak = ::std::max( peak, ( double )::std::abs( data[ j * 2 ] ) );
            peak = ::std::max( peak, ( double )::std::abs( data[ j * 2 + 1 ] ) );
        }

        smooth[ i ] = alpha * peak + ( 1 - alpha ) * smooth[ i ];
        ::Wave[ i ] = ::std::clamp( ( int )( WINHEIGHT * smooth[ i ] ), 1, WINHEIGHT );
    }
}

void Decode( ::ma_device* device, ::uint8_t* output, ::ma_uint32 framecount ) {
    if ( ::PauseAudio || !::Playing.SWR ) {
        if ( ::Wave[ 0 ] )
            ::memset( ::Wave, 0, sizeof( ::Wave ) );
        return;
    }

    ::uint8_t* out = output;
    ::ma_uint32 frames = framecount;

    {
        ::std::lock_guard lock( ::SeekMutex );

        while ( frames > 0 ) {
            int wrote = ::swr_convert( ::Playing.SWR, &out, FFMIN( frames, ::swr_get_out_samples( ::Playing.SWR, frames ) ), nullptr, 0 );
            if ( wrote <= 0 )
                break;

            out += wrote * device->playback.channels * sizeof( float );
            frames -= wrote;
        } while ( frames > 0 && ::av_read_frame( ::Playing.Format, ::Playing.Packet ) == 0 ) {
            if ( ::avcodec_send_packet( ::Playing.Codec, ::Playing.Packet ) < 0 || ::avcodec_receive_frame( ::Playing.Codec, ::Playing.Frame ) < 0 ) {
                ::av_packet_unref( ::Playing.Packet );
                ::av_frame_unref( ::Playing.Frame );

                continue;
            }

            int conv = 0;

            if ( ::Playing.Frame->extended_data && ::Playing.Frame->nb_samples > 0 )
                conv = ::swr_convert( ::Playing.SWR, &out, frames, ::Playing.Frame->extended_data, ::Playing.Frame->nb_samples );

            ::av_packet_unref( ::Playing.Packet );
            ::av_frame_unref( ::Playing.Frame );

            if ( conv <= 0 )
                break;

            out += conv * device->playback.channels * sizeof( float );
            frames -= conv;
        }

        ::av_packet_unref( ::Playing.Packet );
    }

    ::cursor = ::cursor + framecount / ( double )device->sampleRate;
    float* fo = ( float* )output;

    double v = ::Saved::Volumes[ ::Saved::Playing ];
    for ( int i = 0; i < framecount; i++ ) {
        float& l = fo[ i * 2 ];
        float& r = fo[ i * 2 + 1 ];

        r *= v;
        l *= v;

        if ( !::std::isfinite( r ) || r > 1.0f || r < -1.0f )
            r = 0.0f;

        if ( !::std::isfinite( l ) || l > 1.0f || l < -1.0f )
            l = 0.0f;
    }

    ::Waveform( fo, framecount );

    if ( ::cursor >= ::Playing.Duration )
        ::queue::next( ::Saved::Playback == ::Playback::Repeat ? 0 : 1 );
}