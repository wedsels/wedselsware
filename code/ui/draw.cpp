#include "ui.hpp"

void CheckClick( ::Rect r, float* light, ::std::optional< ::Input::Click > c ) {
    if ( c ) {
        ::Input::Click::create( r, *c );
        if ( light && ::Input::hover == r )
            *light *= 0.5f;
    } else ::Input::clicks.erase( r );
}

void DrawBox( ::Rect t, ::uint32_t b, float& light, ::std::optional< ::Input::Click > c ) {
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

void DrawImage( ::Rect r, ::uint8_t* img, float light, ::std::optional< ::Input::Click > c, ::uint8_t channels ) {
    int size = r.r - r.l;

    ::CheckClick( r, &light, c );

    for ( int y = 0; y < size; y++ )
        for ( int x = 0; x < size; x++ )
            ::SetPixel( r.l + x, r.t + y, ::ImagePixelColor( img, x, y, size, channels, light ) );
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