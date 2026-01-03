#include "audio.hpp"

#include <fftw3.h>

::uint16_t Bands[ WINHEIGHT ];

void Seek( int time ) {
    int framecount = ::Device.sampleRate / 100;

    ::cursor = ::std::clamp( time, 0, ( int )::Playing.Duration );
    ::av_seek_frame( ::Playing.Format, ::Playing.Stream, ( int )( ::cursor / ::Playing.Timebase ), AVSEEK_FLAG_BACKWARD );
    ::avcodec_flush_buffers( ::Playing.Codec );
    ::swr_close( ::Playing.SWR );
    ::swr_init( ::Playing.SWR );
    ::swr_inject_silence( ::Playing.SWR, framecount * 2 );
}

struct Visualizer : ::UI {
    ::uint16_t OldBands[ WINHEIGHT ];

    void Clear() { ::std::memset( OldBands, 0, sizeof( OldBands ) ); }

    void Draw() {
        if ( !::Bands[ 0 ] && !OldBands[ 0 ] )
            return;

        for ( int i = 0; i < WINHEIGHT; i++ ) {
            int x = SPACING + ::MIDPOINT + ( WINHEIGHT - 1 - i ) * WINWIDTH;
            int b = ::Bands[ i ];

            if ( OldBands[ i ] > b )
                ::std::memset( ::Canvas + x + b, 0, ( OldBands[ i ] - b ) * sizeof( ::uint32_t ) );

            ::std::memcpy(
                ::Canvas + x,
                ::Playing.Cover + ::MIDPOINT * ( ::MIDPOINT - 1 - ( i + ::Frame ) % ::MIDPOINT ),
                b * sizeof( ::uint32_t )
            );

            OldBands[ i ] = b;
        }
    }
} Visualizer;

void Waveform( float* data, ::uint32_t frames ) {
    static const double width = WINWIDTH - ::MIDPOINT - SPACING;
    static const double alpha = 0.33;
    static double smooth[ WINHEIGHT ];

    double samples = ( double )frames / WINHEIGHT;

    for ( int i = 0; i < WINHEIGHT; i++ ) {
        int start = i * samples;
        int end = ( i + 1 ) * samples;

        double peak = 0.0;
        for ( int j = start; j < end && j < frames; ++j )
            peak = ::std::max( peak, 0.5 * ::std::abs( data[ j * 2 ] + data[ j * 2 + 1 ] ) );

        smooth[ i ] = alpha * peak + ( 1 - alpha ) * smooth[ i ];

        ::Bands[ i ] = ::std::clamp( width * 0.33 * smooth[ i ], 1.0, width );
    }
}

void Decode( ::ma_device* device, ::uint8_t* output, ::ma_uint32 framecount ) {
    if ( ::PauseAudio || !::Playing.SWR ) {
        if ( ::Bands[ 0 ] )
            ::memset( ::Bands, 0, sizeof( ::Bands ) );
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

    ::cursor += framecount / ( double )device->sampleRate;

    if ( !::PauseDraw )
        ::Waveform( ( float* )output, framecount );
    else if ( ::Bands[ 0 ] )
        ::memset( ::Bands, 0, sizeof( ::Bands ) );

    if ( ( int )::cursor > ::Playing.Duration )
        ::Message( WM_QUEUENEXT, 0, 0 );
}