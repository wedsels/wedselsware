#include "../audio/audio.hpp"

struct Cursor : ::UI {
    double LastCursor;
    ::Rect LastHover;

    void Clear() { LastCursor = 0.0; }

    void Draw() {
        if ( LastCursor == ::cursor && LastHover == ::Input::hover )
            return;

        ::Rect r = {
            0,
            WINHEIGHT - ::MIDPOINT - SPACING * 3,
            ::MIDPOINT,
            WINHEIGHT - ::MIDPOINT - SPACING
        };

        ::DrawBox(
            r,
            COLORGHOST,
            ::Input::Click{
                .lmb = []() { ::Seek( ::Playing.Duration * ( ::Input::mouse.x / ( double )MIDPOINT ) ); },
                .rmb = []() { ::Seek( 0 ); },
                .scrl = []( int s ) { ::Seek( ::cursor - s ); }
            }
        );

        r.r = ( int )( ( ::MIDPOINT - SPACING ) * ::std::min( 1.0, ( ::cursor / ::Playing.Duration ) ) );
        ::DrawBox( r, COLORCORAL );

        LastCursor = ::cursor;
        LastHover = ::Input::hover;
    }
} Cursor;