#include "audio.hpp"

#include <array>

constexpr int BANDS = WINHEIGHT;

::uint16_t Bands[ ::BANDS ];

void Seek( int time ) {
    ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

    int framecount = ::Device.sampleRate / 100;

    time = ::std::max( time, 0 );
    ::cursor = ( framecount / ( double )::Device.sampleRate ) * time * 100.0;
    ::av_seek_frame( ::Playing.Format, ::Playing.Stream, ( int )( time / ::Playing.Timebase ), AVSEEK_FLAG_BACKWARD );
    ::avcodec_flush_buffers( ::Playing.Codec );
    ::swr_close( ::Playing.SWR );
    ::swr_init( ::Playing.SWR );
    ::swr_inject_silence( ::Playing.SWR, framecount * 2 );
}

void Waveform( double* data, ::uint32_t frames ) {
    static const double width = WINWIDTH - ::MIDPOINT;
    static const double alpha = 0.2;
    static double smooth[ ::BANDS ];

    double output[ ::BANDS ];
    double samples = ( double )frames / ::BANDS;

    double v = ::Saved::Volumes[ ::Saved::Playing ];
    int off = ::cursor * 100.0;

    for ( int i = 0; i < ::BANDS; ++i ) {
        int start = ( int )( i * samples );
        int end = ( int )( ( i + 1 ) * samples );

        double sum = 0.0;
        for ( int j = start; j < end && j < frames; ++j )
            sum += data[ j ] * data[ j ];

        output[ i ] = ::std::sqrt( sum / ( end - start ) );
        smooth[ i ] = alpha * output[ i ] + ( 1.0 - alpha ) * smooth[ i ];

        int w = ::std::clamp( width * ( smooth[ i ] * 1e5 ) * v, 1.0, width );

        int change = ::Bands[ i ] - w;
        if ( change > 0 ) {
            int x = ::MIDPOINT + ( ::BANDS - 1 - i ) * WINWIDTH + w;
            ::std::memset( ::Canvas + x, COLORALPHA, change * sizeof( ::uint32_t ) );
        } else ::std::memcpy(
            ::Canvas + ::MIDPOINT + ( ::BANDS - 1 - i ) * WINWIDTH,
            ::Canvas + WINWIDTH * ( WINHEIGHT - ::MIDPOINT ) + WINWIDTH * ( ::MIDPOINT - 1 - ( i + off ) % ::MIDPOINT ),
            w * sizeof( ::uint32_t )
        );

        ::Bands[ i ] = w;
    }
}

void ClearBars() {
    if ( ::Bands[ 0 ] > 0 )
        for ( int i = 0; i < ::BANDS; i++ )
            if ( !::PauseDraw )
                for ( int x = 0; x < ::Bands[ i ]; x++ )
                    ::SetPixel( ::MIDPOINT + x, ( ::BANDS - 1 - i ), COLORALPHA );
    
    ::memset( ::Bands, 0, sizeof( ::Bands ) );
}

void Decode( ::ma_device* device, ::uint8_t* output, ::ma_uint32 framecount ) {
    ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

    if ( ::PauseAudio || !::Playing.SWR ) {
        ::ClearBars();
        return;
    }

    ::uint8_t* out = output;
    ::ma_uint32 frames = framecount;

    int to = FFMIN( frames, ::av_rescale_rnd( ::swr_get_delay( ::Playing.SWR, ::Playing.Codec->sample_rate ), device->sampleRate, ::Playing.Codec->sample_rate, ::AV_ROUND_UP ) );
    if ( to > 0 ) {
        int wrote = ::swr_convert( ::Playing.SWR, &out, to, nullptr, 0 );
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

        out += conv * device->playback.channels * sizeof( float );
        frames -= conv;

        ::av_packet_unref( ::Playing.Packet );
        ::av_frame_unref( ::Playing.Frame );

        if ( conv <= 0 )
            break;
    }

    static int lastsecond;
    ::cursor += framecount / ( double )device->sampleRate;

    if ( ::PauseDraw )
        ::ClearBars();
    else {
        ::Waveform( ( double* )output, framecount );

        lastsecond = ( int )::cursor;
    }

    if ( ( int )::cursor > ::Playing.Duration )
        ::Message( WM_QUEUENEXT, 0, 0 );
}