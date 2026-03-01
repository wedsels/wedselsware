#include "../audio/audio.hpp"

struct Songs : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Songs; }

    void GridClear() { SetOffset( ::Index( ::SongDisplay, ::Saved::Playing ) ); }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::SongDisplay; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Library[ item ].Minicover; }

    void GridEnter( ::uint32_t item ) { ::SongInfo( item ); }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Library[ item ];

        if ( HELD( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::queue::set( s.ID, 1 );
            else
                ::queue::add( s.ID, 1 );
        } else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( s.Path, 1 );
        else if ( PRESSED( VK_MBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::WriteCovers( s );
            else
                ::Execute( ::String::WConcat( L"\"C:\\Program Files\\Mozilla Firefox\\firefox.exe\" \"https://covers.musichoarders.xyz/?artist=", s.Artist, "&album=", s.Album, "&sources=applemusic,bugs,flo,itunes,kkbox,linemusic,musicbrainz,tidal", "\"" ), 2 );
        }
    }

    void GridMove( ::uint32_t item ) {
        if ( HELD( VK_LBUTTON ) )
            GridClick( item );
    }
} Songs;