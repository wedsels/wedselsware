#include "ui.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <fstream>

::stbtt_fontinfo FontInfo;
::std::vector< unsigned char > FontBuffer;

int TextWidth( ::std::wstring& text ) {
    int width = 0;
    for ( wchar_t& i : text )
        width += i == ' ' ? FONTSPACE : ::ArchiveFont( i ).width;
    return width;
}

::Font ArchiveFont( wchar_t c ) {
    if ( ::Font::Letters.find( c ) == ::Font::Letters.end() ) {
        float scale = ::stbtt_ScaleForPixelHeight( &FontInfo, FONTHEIGHT );

        int width, height, xoff, yoff;
        unsigned char* bitmap = ::stbtt_GetCodepointBitmap( &::FontInfo, 0, scale, c, &width, &height, nullptr, nullptr );

        int x0, y0, x1, y1;
        ::stbtt_GetCodepointBitmapBox( &::FontInfo, c, scale, scale, &x0, &y0, &x1, &y1 );

        ::uint32_t* pixels = ::new ::uint32_t[ width * height ];

        for ( int i = 0; i < width * height; ++i )
            pixels[ i ] = ::SetAlpha( COLORPALE, bitmap[ i ] );

        ::stbtt_FreeBitmap( bitmap, nullptr );

        ::Font::Letters[ c ] = {
            .map = pixels,
            .width = width,
            .height = height,
            .yoff = y0
        };
    }

    return ::Font::Letters[ c ];
}

::HRESULT InitializeFont() {
    ::std::ifstream file( "onryou.ttf", ::std::ios::binary | ::std::ios::ate );
    if ( !file )
        return E_FAIL;

    ::std::streamsize size = file.tellg();
    file.seekg( 0, ::std::ios::beg );

    FontBuffer.resize( size );
    if ( !file.read( ( char* )FontBuffer.data(), size ) )
        return E_FAIL;

    if ( !::stbtt_InitFont( &FontInfo, FontBuffer.data(), 0 ) )
        return E_FAIL;

    return S_OK;
}