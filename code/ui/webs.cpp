#include "../audio/audio.hpp"

struct Webs : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Webs; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Saved::Webs; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Saved::WebsPath[ item ].img; }

    void GridEnter( ::uint32_t item ) { ::DisplayText = { ::Saved::WebsPath[ item ].path }; }

    void GridClick( ::uint32_t item ) {
        if ( PRESSED( VK_LBUTTON ) )
            ::Execute( ::Saved::WebsPath[ item ].path );
        else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( ::Saved::WebsPath[ item ].path, 1 );
    }
} Webs;