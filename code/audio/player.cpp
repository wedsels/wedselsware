#include "audio.hpp"

#include <fftw3.h>

#include <array>

#define BANDS ( INT )( WINHEIGHT / 2 )

int lastsecond = 0;
double lastcursor = 0.0;

::uint16_t Bands[ BANDS ];

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

double fsin( double x ) {
    static constexpr int luts = 8192;
    static constexpr double tpi = 2.0 * M_PI;
    static constexpr double ipti = 1.0 / tpi;

    static const ::std::array< double, luts > lut = [] {
        ::std::array< double, luts > arr{};
        for ( int i = 0; i < luts; ++i )
            arr[ i ] = ::std::sin( tpi * i / luts );
        return arr;
    }();

    x = ::std::fmod( x, tpi );
    if ( x < 0.0 )
        x += tpi;

    return lut[ ( int )( x * ipti * luts ) ];
}

void Spectrum( double* data, ::uint32_t frames ) {
    static ::fftw_complex* fout = nullptr;
    static ::fftw_plan p = nullptr;
    static double* fin = nullptr;

    if ( !fin ) {
        fout = ( ::fftw_complex* )::fftw_malloc( sizeof( ::fftw_complex ) * frames );
        fin = ( double* )::fftw_malloc( sizeof( double ) * frames );
        p = ::fftw_plan_dft_r2c_1d( frames, fin, fout, FFTW_ESTIMATE );
    }

    for ( int i = 0; i < ( int )frames; i++ )
        fin[ i ] = data[ i ] * ( 0.5 * ( 1 - ::cos( 2 * M_PI * i / ( frames - 1 ) ) ) );

    ::fftw_execute( p );

    int bins = frames / 2 + 1;

    static const double minfq = 20.0;
    static const double maxfq = 35000.0;
    static const double bandwidth = ( maxfq - minfq ) / BANDS;

    double mmag = ::std::numeric_limits< double >::epsilon();
    for ( int i = 0; i < bins; i++ )
        mmag = ::std::max( mmag, ::sqrt( fout[ i ][ 0 ] * fout[ i ][ 0 ] + fout[ i ][ 1 ] * fout[ i ][ 1 ] ) );

    for ( int i = 0; i < bins; i++ ) {
        fout[ i ][ 0 ] /= mmag;
        fout[ i ][ 1 ] /= mmag;
    }

    for ( int i = 0; i < BANDS; i++ ) {
        double low = minfq + ( bandwidth * i );
        double high = minfq + ( bandwidth * ( i + 1 ) );

        int start = ( int )( ( low / ( ::Device.sampleRate / 2.0 ) ) * bins );
        int end = ( int )( ( high / ( ::Device.sampleRate / 2.0 ) ) * bins );

        start = ::std::max( start, 0 );
        end = ::std::min( end, bins - 1 );

        double sum = 0.0;
        int count = 0;
        for ( int j = start; j <= end; j++ ) {
            double magnitude = ::sqrt( fout[ j ][ 0 ] * fout[ j ][ 0 ] + fout[ j ][ 1 ] * fout[ j ][ 1 ] );

            double fq = j * ::Device.sampleRate / ( 0.05 * bins );
            double weight = 1.0 + fq / maxfq;
            sum += magnitude * weight;
            count++;
        }

        double avg = count > 0 ? sum / count : 0.0;
        avg *= ::Saved::Volumes[ ::Saved::Playing ];

        int w = ( int )::std::clamp( WINWIDTH / 2.0 * ::log10( 1.0 + avg ), 1.0, ( WINWIDTH - MIDPOINT ) / 2.0 );

        if ( w == ::Bands[ i ] )
            continue;

        int change = ::Bands[ i ] - w;
        if ( change > 0 )
            for ( int x = w; x < w + change; x++ )
                ::SetPixel( MIDPOINT + x * 2, i * 2, COLORALPHA );
        else
            for ( int x = 0; x < w; x++ )
                ::SetPixel(
                    MIDPOINT + x * 2,
                    i * 2,
                    ::BGRA(
                        127 * ( ::fsin( 0.02 * x + ::cursor ) + 1 ),
                        127 * ( ::fsin( 0.02 * i + ::cursor + 2.0 ) + 1 ),
                        127 * ( ::fsin( 0.02 * ( x + i ) + ::cursor + 4.0 ) + 1 )
                    )
                );

        ::Bands[ i ] = w;
    }
}

void ClearBars() {
    if ( ::Bands[ 0 ] > 0 ) {
        for ( int i = 0; i < BANDS; i++ ) {
            if ( !::PauseDraw )
                for ( int x = 0; x < ::Bands[ i ]; x++ )
                    ::SetPixel( MIDPOINT + x * 2, i * 2, COLORALPHA );
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

    ::ma_uint32 frames = framecount;
    ::cursor += frames / ( double )device->sampleRate;

    if ( ::swr_get_delay( ::Playing.SWR, ::Playing.Codec->sample_rate ) > framecount )
        framecount -= ::swr_convert( ::Playing.SWR, &output, framecount, nullptr, NULL );
    while ( framecount > 0 ) {
        switch ( ::av_read_frame( ::Playing.Format, ::Playing.Packet ) ) {
            case 0:
                break;
            case AVERROR_EOF:
                    ::Message( WM_QUEUENEXT, 0, 0 );
                return;
            default:
                return;
        }

        if ( ::avcodec_send_packet( ::Playing.Codec, ::Playing.Packet ) < 0 || ::avcodec_receive_frame( ::Playing.Codec, ::Playing.Frame ) < 0 )
            continue;

        int out = ::swr_convert( ::Playing.SWR, &output, framecount, ::Playing.Frame->extended_data, ::Playing.Frame->nb_samples );

        ::av_packet_unref( ::Playing.Packet );
        ::av_frame_unref( ::Playing.Frame );

        if ( out > 0 )
            framecount -= out;
        else break;
    }

    if ( ::PauseDraw )
        ::ClearBars();
    else {
        ::Spectrum( ( double* )output, frames );

        if ( ::lastsecond != ( int )::cursor )
            ::Redraw( ::DrawType::Redo );
        else {
            ::DrawCursor();
            ::InitiateDraw();
        }
        ::lastsecond = ( int )::cursor;
    }
}