#include "../audio/audio.hpp"

struct Apps : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Apps; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Saved::Apps; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Saved::AppsPath[ item ].img; }

    void GridEnter( ::uint32_t item ) { ::DisplayText = { ::Saved::AppsPath[ item ].path }; }

    void GridClick( ::uint32_t item ) {
        if ( PRESSED( VK_LBUTTON ) )
            ::Execute( ::Saved::AppsPath[ item ].path );
        else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( ::Saved::AppsPath[ item ].path, 1 );
    }
} Apps;