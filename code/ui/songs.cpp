#include "../audio/audio.hpp"

struct Songs : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Songs; }

    void GridClear() { SetOffset( ::Index( ::SongDisplay, ::Saved::Playing ) ); }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::SongDisplay; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Saved::Songs[ item ].Minicover; }

    void GridEnter( ::uint32_t item ) { ::SongInfo( item ); }

    // bool GridBlockDraw() { ::std::cout<<GetDisplay().size()<<'\n'; return true; }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Saved::Songs[ item ];

        if ( HELD( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::queue::set( s.ID, ::Index( ::Queue(), s.ID ) > -1 ? 0 : 1 );
            else if ( ::Index( ::Queue(), s.ID ) < 0 )
                ::queue::add( s.ID, 1 );
        } else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( s.Path, 1 );
    }

    void GridMove( ::uint32_t item ) {
        if ( HELD( VK_LBUTTON ) )
            GridClick( item );
    }
} Songs;