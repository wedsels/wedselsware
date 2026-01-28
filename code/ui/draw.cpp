#include "../audio/audio.hpp"

void SetPixel( int x, int y, ::uint32_t color ) {
    int i = y * WINWIDTH + x;

    if ( i > WINWIDTH * WINHEIGHT )
        return;

    ::Canvas[ i ] = color;
}

::uint32_t TimeColor() {
    static const int cycle = 100;
    double phase = ::std::fmod( ( double )( ::Frame % cycle ) / cycle, 1.0 );
    double r = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase );
    double g = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase + 2.1 );
    double b = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase + 4.2 );

    ::uint8_t R = ( ::uint8_t )( r * 255.0 );
    ::uint8_t G = ( ::uint8_t )( g * 255.0 );
    ::uint8_t B = ( ::uint8_t )( b * 255.0 );

    return ( B << 24 ) | ( G << 16 ) | ( R << 8 ) | 255;
}

void Invert( ::uint32_t& color ) {
    color = ( color ^ 0x00FFFFFFu ) | 0xFF000000u;
}

void BorderRect( ::Rect& r ) {
    if ( r.l < 0 )
        r.l = 0;
    if ( r.t < 0 )
        r.t = 0;
    if ( r.b > WINHEIGHT )
        r.b = WINHEIGHT;
    if ( r.r > WINWIDTH )
        r.r = WINWIDTH;
}

void DrawBorder( ::Rect& r ) {
    ::BorderRect( r );

    int width = r.r - r.l;
    int height = r.b - r.t;

    for ( int x = 0; x < width - 2; x++ ) {
        ::Invert( ::Canvas[ 1 + r.t * WINWIDTH + r.l + x ] );
        ::Invert( ::Canvas[ 1 + ( r.b - 1 ) * WINWIDTH + r.l + x ] );
    }

    for ( int y = 0; y < height; y++ ) {
        ::Invert( ::Canvas[ ( r.t + y ) * WINWIDTH + r.l ] );
        ::Invert( ::Canvas[ ( r.t + y ) * WINWIDTH + r.r - 1 ] );
    }
}

void DrawBox( ::Rect& r, ::uint32_t b ) {
    ::BorderRect( r );

    int width = r.r - r.l;
    int height = r.b - r.t;

    for ( int y = 0; y < height; y++ )
        ::std::fill_n(
            ::Canvas + ( r.t + y ) * WINWIDTH + r.l,
            width,
            b
        );
}

void DrawImage( ::Rect& r, ::uint32_t* img ) {
    ::BorderRect( r );

    int width = r.r - r.l;
    int height = r.b - r.t;

    if ( !img || ::EmptyImage( img, width ) )
        ::DrawBox( r, COLORGHOST );
    else for ( int y = 0; y < height; y++ )
        ::std::memcpy(
            ::Canvas + ( r.t + y ) * WINWIDTH + r.l,
            img + y * width,
            width * sizeof( ::uint32_t )
        );
}

void DrawString( int ox, int oy, int width, ::std::wstring& s ) {
    const int tw = ::TextWidth( s );
    int offx = 0;

    if ( tw <= width )
        offx += width / 2;
    else
        offx += FONTSPACE * ::Frame / 30.0;

    const int wlimit = width + tw / 2;

    for ( wchar_t& i : s ) {
        const ::Font& f = ::ArchiveFont( i );

        const int sy = oy + f.yoff + FONTHEIGHT;

        for ( int y = 0; y < f.height; ++y ) {
            const int cy = y * f.width;

            int dy = sy + y;
            if ( dy >= WINHEIGHT )
                break;
            dy *= WINWIDTH;

            for ( int x = 0; x < f.width; ++x ) {
                ::uint32_t c = f.map[ cy + x ];
                if ( ( c >> 24 ) == 0 )
                    continue;

                int dx = width + offx + x;
                if ( dx >= wlimit )
                    dx -= wlimit;

                if ( dx < 0 || dx >= width )
                    continue;

                ::Canvas[ ox + dx + dy ] = c;
            }
        }

        offx += f.width;

        if ( i == ' ' )
            offx += FONTSPACE;
    }
}