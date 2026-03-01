#include "../audio/audio.hpp"

struct Search : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Search; }

    ::std::vector< ::uint32_t > Found;
    ::std::wstring Finding;

    ::std::vector< ::uint32_t >& GetDisplay() { return Found; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Library[ item ].Minicover; }

    void GridEnter( ::uint32_t item ) { ::SongInfo( item ); ::DisplayText.push_back( ::String::WConcat( L"Searching: ", Finding ) ); }
    void OutEnter() { ::DisplayText = { ::String::WConcat( L"Searching: ", Finding ) }; };

    bool BlockKey() { return true; }

    void Keypress() {
        for ( int i = 0; i < UINT8_MAX; ++i )
            if ( PRESSED( i ) )
                switch ( ::String::Key( Finding, i ) ) {
                    case 0:
                        break;
                    case 1:
                            if ( Finding == L"NULL" ) {
                                Found.clear();
                                for ( int i = 0; i < ::SongDisplay.size(); i++ )
                                    if ( !::Library[ ::SongDisplay[ i ] ].Minicover )
                                        Found.push_back( ::SongDisplay[ i ] );
                            } else if ( !Found.empty() ) {
                                for ( int i = Found.size() - 1; i >= 0; i-- )
                                    if ( !::Library[ Found[ i ] ].Path.contains( Finding ) )
                                        Found.erase( Found.begin() + i );
                            } else
                                for ( int i = 0; i < ::SongDisplay.size(); i++ )
                                    if ( ::Library[ ::SongDisplay[ i ] ].Path.contains( Finding ) )
                                        Found.push_back( ::SongDisplay[ i ] );
                        break;
                    case 2:
                            Found.clear();
                            if ( !Finding.empty() )
                                for ( int i = 0; i < ::SongDisplay.size(); i++ )
                                    if ( ::Library[ ::SongDisplay[ i ] ].Path.contains( Finding ) )
                                        Found.push_back( ::SongDisplay[ i ] );
                        break;
                }
    }

    void GridKey( ::uint32_t item ) { return Keypress(); }
    void OutKey() { return Keypress(); }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Library[ item ];

        if ( HELD( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::SetSong( s.ID );
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
} Search;