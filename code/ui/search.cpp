#include "../audio/audio.hpp"

struct Search : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Search; }

    ::std::vector< ::uint32_t > Found;
    ::std::wstring Finding;

    ::std::vector< ::uint32_t >& GetDisplay() { return Found; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Saved::Songs[ item ].Minicover; }

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
                                    if ( ::EmptyImage( ::Saved::Songs[ ::SongDisplay[ i ] ].Minicover, ARRAYSIZE( ::Saved::Songs[ ::SongDisplay[ i ] ].Minicover ) ) )
                                        Found.push_back( ::SongDisplay[ i ] );
                            } else if ( !Found.empty() ) {
                                for ( int i = Found.size() - 1; i >= 0; i-- )
                                    if ( !::wcsstr( ::Saved::Songs[ Found[ i ] ].Path, Finding.c_str() ) )
                                        Found.erase( Found.begin() + i );
                            } else
                                for ( int i = 0; i < ::SongDisplay.size(); i++ )
                                    if ( ::wcsstr( ::Saved::Songs[ ::SongDisplay[ i ] ].Path, Finding.c_str() ) )
                                        Found.push_back( ::SongDisplay[ i ] );
                        break;
                    case 2:
                            Found.clear();
                            if ( !Finding.empty() )
                                for ( int i = 0; i < ::SongDisplay.size(); i++ )
                                    if ( ::wcsstr( ::Saved::Songs[ ::SongDisplay[ i ] ].Path, Finding.c_str() ) )
                                        Found.push_back( ::SongDisplay[ i ] );
                        break;
                }
    }

    void GridKey( ::uint32_t item ) { return Keypress(); }
    void OutKey() { return Keypress(); }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Saved::Songs[ item ];

        if ( HELD( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::queue::set( s.ID, ::Index( ::Queue(), s.ID ) > -1 ? 0 : 1 );
            else
                ::queue::add( s.ID, 1 );
        } else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( s.Path, 1 );
    }

    void GridMove( ::uint32_t item ) {
        if ( HELD( VK_LBUTTON ) )
            GridClick( item );
    }
} Search;