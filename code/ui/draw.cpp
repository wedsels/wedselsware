#include "ui.hpp"

void CheckClick( ::rect r, float* light, ::std::optional< ::input::click > c ) {
    if ( c ) {
        ::input::click::create( r, *c );
        if ( light && ::input::hover == r )
            *light *= 0.5f;
    } else ::input::clicks.erase( r );
}

void DrawBox( ::rect t, ::uint32_t b, float& light, ::std::optional< ::input::click > c ) {
    int width = t.r - t.l;
    int height = t.b - t.t;

    ::CheckClick( t, &light, c );

    for ( int y = 0; y < height; y++ )
        for ( int x = 0; x < width; x++ ) {
            int cx = t.l + x;
            int cy = t.t + y;

            if ( cx < 0 || cx >= WINWIDTH || cy < 0 || cy >= WINHEIGHT )
                continue;

            ::SetPixel( cx, cy, ::Multiply( b, light ) );
        }
}

void DrawImage( ::rect r, ::uint8_t* img, float light, ::std::optional< ::input::click > c, ::uint8_t channels ) {
    int size = r.r - r.l;

    ::CheckClick( r, &light, c );

    for ( int y = 0; y < size; y++ )
        for ( int x = 0; x < size; x++ ) {
            int cx = r.l + x;
            int cy = r.t + y;

            ::uint8_t colors[] = { 255, 255, 255, 255 };

            if ( img ) {
                int index = ( y * size + x ) * channels;

                for ( int i = 0; i < channels; i++ )
                    colors[ i ] = ( ::uint8_t )( img[ index + i ] * ( i < 3 ? 1.0f : light ) );

                ::SetPixel( cx, cy, ::BGRA( colors[ 0 ], colors[ 1 ], colors[ 2 ], colors[ 3 ], light ) );
            } else ::SetPixel( cx, cy, ::Multiply( COLORGHOST, light ) );
        }
}

void DrawString( int ox, int oy, int width, ::std::wstring& s, ::std::optional< ::input::click > c ) {
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

void Render( ::DrawType dt ) {
    for ( ::UI* i : ::UI::Registry() )
        i->Draw( dt );
    
    ::InitiateDraw();
}

void Draw( ::DrawType dt ) {
    if ( ::PauseDraw )
        return;

    switch ( dt ) {
        case ::DrawType::None:
            break;
        case ::DrawType::Normal:
                ::Render( dt );
            break;
        case ::DrawType::Redo:
                ::Render( dt );
            break;
    }
}

::HRESULT InitDraw() {
    for ( ::UI* i : ::UI::Registry() )
        i->Initialize();

    return S_OK;
}