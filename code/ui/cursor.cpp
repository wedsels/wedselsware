#include "../audio/audio.hpp"

struct Cursor : ::UI {
    void Draw() {
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
    }
};
::Cursor Cursor;