#include "../audio/audio.hpp"

struct Apps : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Apps; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Apps; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::AppsPath[ item ].IMG; }

    void GridEnter( ::uint32_t item ) { ::DisplayText = { ::AppsPath[ item ].Path }; }

    void GridClick( ::uint32_t item ) {
        if ( PRESSED( VK_LBUTTON ) )
            ::Execute( ::AppsPath[ item ].Path );
        else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( ::AppsPath[ item ].Path, 1 );
    }
} AppsDisplay;