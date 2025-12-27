#include "ui.hpp"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>

#include <shellapi.h>

::uint8_t* To8Bit( ::uint32_t* src, int width, int height ) {
    ::uint8_t* ret = ::new ::uint8_t[ width * height * ::STBIR_RGBA ];

    for ( int i = 0; i < width * height; i++ ) {
        ::uint32_t c = src[ i ];

        ret[ i * 4 + 0 ] = ( c >> 16 ) & 0xFF;
        ret[ i * 4 + 1 ] = ( c >> 8 ) & 0xFF;
        ret[ i * 4 + 2 ] = c & 0xFF;
        ret[ i * 4 + 3 ] = ( c >> 24 ) & 0xFF;
    }

    return ret;
}

::uint32_t* To32Bit( ::uint8_t* data, int w, int h ) {
    ::uint32_t* ret = ::new ::uint32_t[ w * h ];

    for ( int y = 0; y < h; y++ )
        for ( int x = 0; x < w; x++ ) {
            int i = ( y * w + x ) * 4;
            ret[ y * w + x ] = ( data[ i + 3 ] << 24 ) | ( data[ i + 0 ] << 16 ) | ( data[ i + 1 ] << 8 ) | data[ i + 2 ];
        }

    return ret;
}

::uint32_t* ResizeImage( ::uint32_t* data, int w, int h, int scale ) {
    if ( !data ) return nullptr;

    ::uint8_t* in = ::To8Bit( data, w, h );
    ::uint8_t* img = ::new ::uint8_t[ scale * scale * ::STBIR_RGBA ];

    if ( !::stbir_resize_uint8_linear( in, w, h, 0, img, scale, scale, 0, ::STBIR_RGBA ) ) {
        ::delete[] in;
        in = nullptr;
        ::delete[] img;
        img = nullptr;

        return nullptr;
    }

    ::uint32_t* ret = ::To32Bit( img, scale, scale );

    ::delete[] in;
    in = nullptr;
    ::delete[] img;
    img = nullptr;

    return ret;
}

::uint32_t* ProcessImage( ::uint8_t* data, int w, int h, int scale ) {
    if ( !data ) return nullptr;

    ::uint32_t* in = ::To32Bit( data, w, h );
    ::uint32_t* img = ::ResizeImage( in, w, h, scale );

    ::delete[] in;
    in = nullptr;

    ::stbi_image_free( data );

    return img;
}

::uint32_t* ArchiveImage( const char* path, int scale ) {
    int w, h, c;
    ::uint8_t* data = ::stbi_load( path, &w, &h, &c, ::STBIR_RGBA );

    return ::ProcessImage( data, w, h, scale );
}

::uint32_t* ArchiveImage( ::uint8_t* imgdata, int size, int scale ) {
    int w, h;
    if ( !imgdata ) return nullptr;
    ::uint8_t* data = ::stbi_load_from_memory( imgdata, size, &w, &h, nullptr, ::STBIR_RGBA );

    return ::ProcessImage( data, w, h, scale );
}

::uint32_t* ArchiveHICON( ::LPCWSTR path, int size ) {
    ::HICON icon;

    ::std::wstring p = path;
    for ( auto& i : p )
        if ( i == '/' )
            i = '\\';

    if ( !p.contains( L".exe" ) )
        return nullptr;

    ::SHFILEINFOW shFileInfo = { 0 };
    if ( ::SHGetFileInfoW( p.c_str(), 0, &shFileInfo, sizeof( shFileInfo ), SHGFI_ICON | SHGFI_LARGEICON ) )
        icon = shFileInfo.hIcon;
    else return nullptr;

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
    ::uint8_t* img = ::new ::uint8_t[ dataSize ];
    ::memcpy( img, bits, dataSize );

    for ( int i = 0; i < size * size; ++i )
        ::std::swap( img[ i * 4 + 0 ], img[ i * 4 + 2 ] );

    ::SelectObject( hdc, old );
    ::DeleteObject( hBitmap );
    ::DeleteDC( hdc );

    ::DestroyIcon( icon );

    ::uint32_t* ret = ::To32Bit( img, size, size );

    ::delete[] img;
    img = nullptr;

    return ret;
}