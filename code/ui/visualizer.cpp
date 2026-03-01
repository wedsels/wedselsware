#include "../audio/audio.hpp"

struct Visualizer : ::UI {
    ::Rect Bounds = { 0, MaxSlide, WINWIDTH, WINHEIGHT };
    ::Rect& Rect() { return Bounds; }

    bool BlockClick() { return false; }
    bool BlockKey() { return false; }

    ::uint32_t LastOff;
    ::uint16_t OldWave[ WINWIDTH / 2 ];

    void Clear() {
        LastOff = 0;
        ::std::memset( OldWave, 0, sizeof( OldWave ) );
    }

    ::uint32_t Off() { return ::Frame / 6; }

    bool BlockDraw() { return ( !OldWave[ 0 ] && !::Wave[ 0 ] || Off() == LastOff ) && !::std::memcmp( OldWave, Wave, sizeof( OldWave ) ); }
    void Draw() {
        LastOff = Off();

        ::Rect b = Bounds;
        if ( Slide > 0 ) {
            b.t -= Slide;
            b.b -= Slide;
        }

        const int maxy = WINHEIGHT - b.t;

        static const int width = WINWIDTH / 2;
        static const int mask = MIDPOINT * MIDPOINT - 1;

        for ( int x = 0; x < width; x++ ) {
            int wave = ::std::min( ( int )::Wave[ width - x - 1 ], maxy );
            int y = 0;

            int r = WINWIDTH - 1 - x;
            int cx = ( width + x + LastOff * MIDPOINT ) & mask;

            ::uint32_t* row = ::Canvas + b.t * WINWIDTH;

            for ( ; y < wave; y++ ) {
                ::uint32_t cover = COLORGHOST;
                if ( ::PlayingCover && ::PlayingCover[ cx ] & 0xFF000000 )
                    cover = ::PlayingCover[ cx ];
                cx = ( cx + MIDPOINT ) & mask;

                row[ x ] = cover;
                row[ r ] = cover;

                row += WINWIDTH;
            }

            for ( ; y < OldWave[ x ]; y++ ) {
                row[ x ] = COLORALPHA;
                row[ r ] = COLORALPHA;

                row += WINWIDTH;
            }

            OldWave[ x ] = wave;
        }
    }
} Visualizer;