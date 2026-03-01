#include "../audio/audio.hpp"

struct Queues : ::GridUI {
    ::Rect Bounds = { ( WINWIDTH + MIDPOINT ) / 2, 0, WINWIDTH, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Queue(); }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Library[ item ].Minicover; }

    void GridEnter( ::uint32_t item ) { ::SongInfo( item ); ::DisplayText.push_back( ::String::WConcat( L"Queue: ", ::Saved::Queue ) ); }
    void OutEnter() { ::DisplayText = { ::String::WConcat( L"Queue: ", ::Saved::Queue ) }; };

    ::size_t LastDisplay;
    bool GridBlockDraw() {
        bool b = LastDisplay == ::SongDisplay.size();
        LastDisplay = ::SongDisplay.size();
        return b;
    }

    int Clicked;
    bool Moved;

    void GridClear() { Clicked = -1; Moved = false; LastDisplay = 0; }
    void OutLeave() { GridClear(); }

    void GridMove( ::uint32_t item ) {
        if ( Clicked < 0 )
            return;

        ::media& s = ::Library[ item ];
        int nindex = Index();

        if( nindex > -1 && Clicked != nindex ) {
            ::uint32_t v = ::Queue()[ Clicked ];
            ::Queue().erase( ::Queue().begin() + Clicked );
            ::Queue().insert( ::Queue().begin() + nindex, v );

            Redraw();

            Clicked = nindex;
            Moved = true;
        }
    }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Library[ item ];

        if ( RELEASED( VK_LBUTTON ) ) {
            if ( !Moved && Clicked == Index() ) {
                if ( HELD( VK_SHIFT ) )
                    ::queue::set( s.ID, Clicked > -1 ? 0 : 1 );
                else if ( Clicked > -1 )
                    ::Queue().erase( ::Queue().begin() + Clicked );
            }

            GridClear();
        } else if ( PRESSED( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::queue::set( s.ID, 0 );
            else
                Clicked = Index();
        } else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( s.Path, 1 );
        else if ( PRESSED( VK_MBUTTON ) ) {
            if ( HELD( VK_CONTROL ) )
                ::queue::clear();
            else if ( HELD( VK_SHIFT ) )
                ::WriteCovers( s );
            else
                ::Execute( ::String::WConcat( L"\"C:\\Program Files\\Mozilla Firefox\\firefox.exe\" \"https://covers.musichoarders.xyz/?artist=", s.Artist, "&album=", s.Album, "&sources=applemusic,bugs,flo,itunes,kkbox,linemusic,musicbrainz,tidal", "\"" ), 2 );
        }
    }

    void XButton() {
        if ( PRESSED( VK_XBUTTON2 ) ) {
            if ( ::Queue().size() == 1 && ::Saved::Queues.size() - 1 == ::Saved::Queue )
                return;

            ::Saved::Queues.push_back( { ::Saved::Playing } );
            ::Saved::Queue++;
        } else if ( PRESSED( VK_XBUTTON1 ) ) {
            if ( ::Queue().size() == 1 && ::Saved::Queue == 0 )
                return;

            if ( ::Saved::Queue == 0 )
                ::Saved::Queues.insert( ::Saved::Queues.begin(), { ::Saved::Playing } );
            else
                ::Saved::Queue--;
        } else return;

        for ( int i = ::Saved::Queues.size(); i-- > 0; )
            if ( i != ::Saved::Queue && ::Saved::Queues[ i ].size() == 1 ) {
                if ( i < ::Saved::Queue )
                    ::Saved::Queue--;

                ::Saved::Queues.erase( ::Saved::Queues.begin() + i );
            }

        Redraw();
    }
} Queues;