#include "../audio/audio.hpp"

struct Queues : ::GridUI {
    ::Rect Bounds = { ( WINWIDTH + MIDPOINT ) / 2, 0, WINWIDTH, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::Queue(); }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::Saved::Songs[ item ].Minicover; }

    void GridEnter( ::uint32_t item ) { ::SongInfo( item ); ::DisplayText.push_back( ::String::WConcat( L"Queue: ", ::Saved::Queue ) ); }
    void OutEnter() { ::DisplayText = { ::String::WConcat( L"Queue: ", ::Saved::Queue ) }; };

    int Index;
    bool Moved;

    void GridClear() { Index = -1; Moved = false; }
    void OutLeave() { GridClear(); }

    void GridMove( ::uint32_t item ) {
        if ( Index < 0 )
            return;

        ::media& s = ::Saved::Songs[ item ];
        int nindex = ::Index( ::Queue(), s.ID );

        if( nindex > -1 && Index != nindex ) {
            ::uint32_t v = ::Queue()[ Index ];
            ::Queue().erase( ::Queue().begin() + Index );
            ::Queue().insert( ::Queue().begin() + nindex, v );

            Redraw();
        
            Index = nindex;
            Moved = true;
        }
    }

    void GridClick( ::uint32_t item ) {
        ::media& s = ::Saved::Songs[ item ];

        if ( RELEASED( VK_LBUTTON ) ) {
            if ( !Moved && Index == ::Index( ::Queue(), s.ID ) ) {
                if ( HELD( VK_SHIFT ) )
                    ::queue::set( s.ID, Index > -1 ? 0 : 1 );
                else if ( Index > -1 && ::Queue()[ Index ] != ::Saved::Playing )
                    ::Queue().erase( ::Queue().begin() + Index );

                Redraw();
            }

            GridClear();
        } else if ( PRESSED( VK_LBUTTON ) ) {
            if ( HELD( VK_SHIFT ) )
                ::queue::set( s.ID, ::Index( ::Queue(), s.ID ) > -1 ? 0 : 1 );
            else
                Index = ::Index( ::Queue(), s.ID );
        } else if ( PRESSED( VK_RBUTTON ) )
            ::Execute( s.Path, 1 );
        else if ( PRESSED( VK_MBUTTON ) )
            ::queue::clear();
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