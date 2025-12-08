#include "audio.hpp"

#include <array>

constexpr int BANDS = WINHEIGHT / 2;

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
    double samplesPerBar = ( double )frames / ::BANDS;
    ::std::vector< double > barAmplitudes( ::BANDS, 0.0 );

    for ( int bar = 0; bar < ::BANDS; ++bar ) {
        int startSampleF = ::std::floor( bar * samplesPerBar );
        int endSampleF = ::std::ceil( ( bar + 1 ) * samplesPerBar );

        double sumAmplitude = 0.0;
        int count = 0;

        for ( int i = startSampleF; i < endSampleF && i < frames; ++i ) {
            sumAmplitude += ::std::abs( data[ i ] );
            ++count;
        }

        if ( count > 0 )
            barAmplitudes[ bar ] = sumAmplitude / count;
    }

    static double smoothedAmplitudes[ ::BANDS ];
    constexpr double smoothingFactor = 0.25;

    for ( int i = 0; i < ::BANDS; ++i ) {
        smoothedAmplitudes[ i ] =
            smoothingFactor * barAmplitudes[ i ] + ( 1.0 - smoothingFactor ) * smoothedAmplitudes[ i ];
        barAmplitudes[ i ] = smoothedAmplitudes[ i ];
    }

    static double globalMaxAmplitude = 0.00000001;
    constexpr double decayRate = 0.9;

    double maxAmplitude = *::std::max_element( barAmplitudes.begin(), barAmplitudes.end() );
    globalMaxAmplitude = ::std::max( globalMaxAmplitude * decayRate, maxAmplitude );

    if ( globalMaxAmplitude > 0 )
        for ( auto& amp : barAmplitudes )
            amp /= globalMaxAmplitude;

    for ( int i = 0; i < ::BANDS; ++i ) {
        int w = ::std::clamp( WINWIDTH / 2.0 * ( barAmplitudes[ i ] * 0.5 ) * ::Saved::Volumes[ ::Saved::Playing ], 1.0, ( WINWIDTH - ::MIDPOINT ) / 2.0 );

        if ( w == ::Bands[ i ] )
            continue;

        int change = ::Bands[ i ] - w;
        if ( change > 0 )
            for ( int x = w; x < w + change; x++ )
                ::SetPixel( ::MIDPOINT + x * 2, ( ::BANDS - 1 - i ) * 2, COLORALPHA );
        else {
            int xoff = ::cursor * 50.0;
            int yoff = ::cursor * 25.0;
            for ( int x = 0; x < w; x++ )
                ::SetPixel( ::MIDPOINT + x * 2, ( ::BANDS - 1 - i ) * 2, ::ImagePixelColor( ::Playing.Cover, x + xoff, ::BANDS - 1 - i + yoff, ::MIDPOINT ) );
        }

        ::Bands[ i ] = w;
    }
}

void ClearBars() {
    if ( ::Bands[ 0 ] > 0 ) {
        for ( int i = 0; i < ::BANDS; i++ ) {
            if ( !::PauseDraw )
                for ( int x = 0; x < ::Bands[ i ]; x++ )
                    ::SetPixel( ::MIDPOINT + x * 2, ( ::BANDS - 1 - i ) * 2, COLORALPHA );
            ::Bands[ i ] = 0;
        }

        if ( !::PauseDraw ) {
            ::DrawCursor();
            ::InitiateDraw();
        }
    }
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

        if ( lastsecond != ( int )::cursor )
            ::Redraw( ::DrawType::Redo );
        else {
            ::DrawCursor();
            ::InitiateDraw();
        }
        lastsecond = ( int )::cursor;
    }

    if ( ( int )::cursor > ::Playing.Duration )
        ::Message( WM_QUEUENEXT, 0, 0 );
}