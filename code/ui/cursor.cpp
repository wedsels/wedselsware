#include "../audio/audio.hpp"

void DrawCursor() {
    ::Rect r = {
        0,
        WINHEIGHT - ::MIDPOINT - SPACING * 3,
        ::MIDPOINT,
        WINHEIGHT - ::MIDPOINT - SPACING
    };

    float l = 1.0f;
    ::DrawBox(
        r,
        COLORGHOST,
        l,
        ::Input::Click{
            .lmb = []() { ::Seek( ::Playing.Duration * ( ::Input::mouse.x / ( double )MIDPOINT ) ); },
            .rmb = []() { ::Seek( 0 ); },
            .scrl = []( int s ) { ::Seek( ::cursor - s ); }
        }
    );

    r.r = ( int )( ( ::MIDPOINT - SPACING ) * ::std::min( 1.0, ( ::cursor / ::Playing.Duration ) ) );
    ::DrawBox( r, COLORCORAL, l );
}

struct Visual : ::UI {
    void Draw( ::DrawType dt ) {
        if ( dt > ::DrawType::Normal )
            ::DrawCursor();
    }
};
::Visual Visual;