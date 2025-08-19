#include "../audio/audio.hpp"

void DrawCursor() {
    ::rect r = {
        0,
        WINHEIGHT - MIDPOINT - SPACING * 3,
        MIDPOINT,
        WINHEIGHT - MIDPOINT - SPACING
    };

    float l = 1.0f;
    ::DrawBox(
        r,
        COLORGHOST,
        l,
        ::input::click{
            .lmb = []() { ::Seek( ::Playing.Duration * ( ::input::mouse.x / ( double )MIDPOINT ) ); },
            .rmb = []() { ::Seek( 0 ); },
            .scrl = []( int s ) { ::Seek( ::cursor - s ); }
        }
    );

    r.r = ( int )( ( MIDPOINT - SPACING ) * ( ::cursor / ::Playing.Duration ) );
    ::DrawBox( r, COLORCORAL, l );
}

struct Visual : ::UI {
    void Draw( ::DrawType dt ) {
        if ( dt > ::DrawType::Normal )
            ::DrawCursor();
    }
};
::Visual Visual;