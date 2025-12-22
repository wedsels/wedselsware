#include "ui.hpp"

void SetPixel( int x, int y, ::uint32_t color ) {
    if ( y * WINWIDTH + x > WINWIDTH * WINHEIGHT )
        return;

    ::Canvas[ y * WINWIDTH + x ] = color;
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
        ::DrawBorder( r, COLORCOLD );
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
        ox += ( width - tw ) / 2;

    for ( wchar_t& i : s ) {
        ::Font f = ::ArchiveFont( i );

        for ( int y = 0; y < f.height; ++y )
            for ( int x = 0; x < f.width; ++x ) {
                if ( ox + x < 0 || ox + x >= width )
                    break;

                ::uint32_t c = f.map[ y * f.width + x ];
                if ( ( c >> 24 ) > 0 )
                    ::SetPixel( ox + x, oy + y + f.yoff + FONTHEIGHT, c );
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