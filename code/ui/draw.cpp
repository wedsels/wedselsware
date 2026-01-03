#include "../audio/audio.hpp"

void SetPixel( int x, int y, ::uint32_t color ) {
    if ( y * WINWIDTH + x > WINWIDTH * WINHEIGHT )
        return;

    ::Canvas[ y * WINWIDTH + x ] = color;
}

::uint32_t TimeColor() {
    static const int cycle = 100;
    double phase = ::std::fmod( ( double )( ::Frame % cycle ) / cycle, 1.0 );
    double r = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase );
    double g = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase + 2.1 );
    double b = 0.5 + 0.5 * ::std::sin( 2.0 * M_PI * phase + 4.2 );

    ::uint8_t R = ( ::uint8_t )( r * 255 );
    ::uint8_t G = ( ::uint8_t )( g * 255 );
    ::uint8_t B = ( ::uint8_t )( b * 255 );

    return ( B << 24 ) | ( G << 16 ) | ( R << 8 ) | 255;
}

void DrawBorder( ::Rect r, ::uint32_t b ) {
    if ( r.b > WINHEIGHT )
        r.b = WINHEIGHT;
    if ( r.r > WINWIDTH )
        r.r = WINWIDTH;

    int width = r.r - r.l;
    int height = r.b - r.t;

    ::std::fill_n( ::Canvas + r.t * WINWIDTH + r.l, width, b );
    ::std::fill_n( ::Canvas + ( r.b - 1 ) * WINWIDTH + r.l, width, b );

    for ( int y = 0; y < height; y++ ) {
        ::Canvas[ ( r.t + y ) * WINWIDTH + r.l ] = b;
        ::Canvas[ ( r.t + y ) * WINWIDTH + r.r - 1 ] = b;
    }
}

void CheckClick( ::Rect r, ::std::optional< ::Input::Click > c ) {
    if ( c )
        ::Input::Click::create( r, *c );
    else
        ::Input::clicks.erase( r );

    if ( ::Input::hover == r )
        ::DrawBorder( r, ::TimeColor() );
}

void DrawBox( ::Rect r, ::uint32_t b, ::std::optional< ::Input::Click > c ) {
    if ( r.b > WINHEIGHT )
        r.b = WINHEIGHT;
    if ( r.r > WINWIDTH )
        r.r = WINWIDTH;

    int width = r.r - r.l;
    int height = r.b - r.t;

    for ( int y = 0; y < height; y++ )
        ::std::fill_n(
            ::Canvas + ( r.t + y ) * WINWIDTH + r.l,
            width,
            b
        );

    ::CheckClick( r, c );
}

void DrawImage( ::Rect r, ::uint32_t* img, ::std::optional< ::Input::Click > c ) {
    if ( r.b > WINHEIGHT )
        r.b = WINHEIGHT;
    if ( r.r > WINWIDTH )
        r.r = WINWIDTH;

    int width = r.r - r.l;
    int height = r.b - r.t;

    if ( ::EmptyImage( img, width ) )
        ::DrawBox( r, COLORGHOST );
    else for ( int y = 0; y < height; y++ )
        ::std::memcpy(
            ::Canvas + ( r.t + y ) * WINWIDTH + r.l,
            img + y * width,
            width * sizeof( ::uint32_t )
        );

    ::CheckClick( r, c );
}

void DrawString( int ox, int oy, int width, ::std::wstring& s, ::std::optional< ::Input::Click > c ) {
    int tw = ::TextWidth( s );
    if ( tw <= width )
        ox += width / 2;
    else
        ox += FONTSPACE * ::Frame / 10.0;

    for ( wchar_t& i : s ) {
        ::Font f = ::ArchiveFont( i );

        for ( int y = 0; y < f.height; ++y )
            for ( int x = 0; x < f.width; ++x ) {
                int dx = ( width + ox + x ) % ( width + tw / 2 );
                if ( dx < 0 || dx >= width )
                    continue;

                ::uint32_t c = f.map[ y * f.width + x ];
                if ( ( c >> 24 ) > 0 )
                    ::SetPixel( dx, oy + y + f.yoff + FONTHEIGHT, c );
            }

        ox += f.width;

        if ( i == ' ' )
            ox += FONTSPACE;
    }
}

::HRESULT InitDraw() {
    for ( ::UI* i : ::UI::Registry() )
        i->Initialize();

    return S_OK;
}