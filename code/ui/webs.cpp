#include "../audio/audio.hpp"

struct Webs : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Webs; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Webs; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::WebsPath[ item ].IMG; }

    void GridEnter( ::uint32_t item ) { ::DisplayText = { ::WebsPath[ item ].Path }; }

    void GridClick( ::uint32_t item ) {
        if ( PRESSED( VK_LBUTTON ) )
            ::Execute( ::WebsPath[ item ].Path );
        else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( ::WebsPath[ item ].Path, 1 );
    }
} WebsDisplay;