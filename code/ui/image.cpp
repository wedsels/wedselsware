#include "ui.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>

::uint8_t* ResizeImage( ::uint8_t* data, int w, int h, int scale ) {
    if ( !data ) return nullptr;

    ::uint8_t* img = ::new ::uint8_t[ scale * scale * ::STBIR_RGB ];

    if ( !::stbir_resize_uint8_linear( data, w, h, 0, img, scale, scale, 0, ::STBIR_RGB ) ) {
        ::delete[] img;
        img = nullptr;
    }

    return img;
}

::uint8_t* ProcessImage( ::uint8_t* data, int w, int h, int scale ) {
    if ( !data ) return nullptr;

    ::uint8_t* img = ::ResizeImage( data, w, h, scale );

    ::stbi_image_free( data );

    return img;
}

::uint8_t* ArchiveImage( const char* path, int scale ) {
    int w, h, c;
    ::uint8_t* data = ::stbi_load( path, &w, &h, &c, 0 );

    return ::ProcessImage( data, w, h, scale );
}

::uint8_t* ArchiveImage( ::uint8_t* imgdata, int size, int scale ) {
    int w, h;
    if ( !imgdata ) return nullptr;
    ::uint8_t* data = ::stbi_load_from_memory( imgdata, size, &w, &h, nullptr, ::STBIR_RGB );

    return ::ProcessImage( data, w, h, scale );
}

::uint8_t* ArchiveHICON( ::HICON& icon, int size ) {
    ::HDC hdc = ::CreateCompatibleDC( NULL );

    ::BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof( ::BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    ::HBITMAP hBitmap = ::CreateDIBSection( hdc, &bmi, DIB_RGB_COLORS, &bits, NULL, 0 );
    if ( !hBitmap || !bits ) {
        ::DeleteDC( hdc );
        return nullptr;
    }

    ::HGDIOBJ old = ::SelectObject( hdc, hBitmap );

    ::RECT rc = { 0, 0, size, size };
    ::FillRect( hdc, &rc, ( ::HBRUSH )( COLOR_WINDOW + 1 ) );

    ::DrawIconEx( hdc, 0, 0, icon, size, size, 0, NULL, DI_NORMAL );

    ::size_t dataSize = size * size * 4;
    ::uint8_t* ret = ::new ::uint8_t[ dataSize ];
    ::memcpy( ret, bits, dataSize );

    for ( int i = 0; i < size * size; ++i ) {
        ::std::swap( ret[ i * 4 + 0 ], ret[ i * 4 + 2 ] );
    }

    ::SelectObject( hdc, old );
    ::DeleteObject( hBitmap );
    ::DeleteDC( hdc );

    ::DestroyIcon( icon );

    return ret;
}