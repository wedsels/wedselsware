#include "../audio/audio.hpp"

struct Visualizer : ::UI {
    ::Rect Bounds = { 0, MaxSlide, WINWIDTH, WINHEIGHT };
    ::Rect& Rect() { return Bounds; }

    bool BlockClick() { return false; }
    bool BlockKey() { return false; }

    ::uint32_t LastOff;
    ::uint16_t OldWave[ WINWIDTH ];

    void Clear() {
        LastOff = 0;
        ::std::memset( OldWave, 0, sizeof( OldWave ) );
    }

    ::uint32_t Off() { return ::Frame / 6; }

    bool BlockDraw() { return ( !OldWave[ 0 ] && !::Wave[ 0 ] || Off() == LastOff ) && !::std::memcmp( OldWave, OldWave, sizeof( OldWave ) ); }
    void Draw() {
        LastOff = Off();

        ::Rect b = Bounds;
        if ( Slide > 0 ) {
            b.t -= Slide;
            b.b -= Slide;
        }

        static const int dimensions = WINHEIGHT * WINWIDTH - 1;
        static const int mask = MIDPOINT * MIDPOINT - 1;
        static const int width = WINWIDTH / 2;

        for ( int x = 0; x < width; x++ ) {
            int wave = ::Wave[ x ];
            int y = 0;

            int row = b.t * WINWIDTH;

            for ( ; y < wave; y++ ) {
                if ( row > dimensions )
                    break;

                ::uint32_t cover = ::Playing.Cover[ ( width + x + ( y + LastOff ) * MIDPOINT ) & mask ];
                if ( !( cover & 0xFF000000 ) )
                    cover = COLORGHOST;

                ::Canvas[ x + row ] = cover;
                ::Canvas[ WINWIDTH - 1 - x + row ] = cover;

                row += WINWIDTH;
            }

            for ( ; y < OldWave[ x ]; y++ ) {
                if ( row > dimensions )
                    break;

                ::Canvas[ x + row ] = COLORALPHA;
                ::Canvas[ WINWIDTH - 1 - x + row ] = COLORALPHA;

                row += WINWIDTH;
            }

            OldWave[ x ] = wave;
        }
    }
} Visualizer;