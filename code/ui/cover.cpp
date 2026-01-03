#include "../audio/audio.hpp"

::std::unordered_map< ::GridTypes, ::std::wstring > GTNames = {
    { ::GridTypes::Queue, L"Queue" },
    { ::GridTypes::Songs, L"Songs" },
    { ::GridTypes::Search, L"Search" },
    { ::GridTypes::Mixer, L"Mixer" },
    { ::GridTypes::Apps, L"Apps" },
    { ::GridTypes::Webs, L"Webs" }
};

::std::unordered_map< ::Playback, ::std::wstring > PBNames = {
    { ::Playback::Queue, L"Queue" },
    { ::Playback::Linear, L"Linear" },
    { ::Playback::Repeat, L"Repeat" },
    { ::Playback::Shuffle, L"Shuffle" }
};

::std::unordered_map< ::SortTypes, ::std::wstring > STNames = {
    { ::SortTypes::Time, L"Time" },
    { ::SortTypes::Title, L"Title" },
    { ::SortTypes::Artist, L"Artist" }
};

struct Cover : ::UI {
    ::Rect LastHover;
    ::uint32_t LastCover;
    ::std::wstring LastSearch;
    int LastCursor = -1;
    ::GridTypes LastGrid = ::GridTypes::Count;
    ::SortTypes LastSorting = ::SortTypes::Count;
    ::Playback LastPlayback = ::Playback::Count;
    double LastVolume;

    void Clear() {
        LastHover = {};
        LastCover = 0;
        LastSearch.clear();
        LastCursor = -1;
        LastGrid = ::GridTypes::Count;
        LastSorting = ::SortTypes::Count;
        LastPlayback = ::Playback::Count;
        LastVolume = 0.0;
    }

    virtual ::uint8_t GetPriority() const { return 0; }

    void Draw() {
        if ( LastHover == ::Input::hover
            && LastCover == ::Saved::Playing
            && LastSearch == ::Searching
            && ( LastCursor < 0 || LastCursor == ( int )::cursor )
            && LastGrid == ::GridType
            && LastSorting == ::Saved::Sorting
            && LastPlayback == ::Saved::Playback
            && LastVolume == ::Saved::Volumes[ ::Saved::Playing ]
        ) return;

        ::Rect rect { 0, WINHEIGHT - ::MIDPOINT, ::MIDPOINT };

        ::Input::Click c {
            .lmb = []() { ::EnumNext( ::GridType, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); ::DisplayOffset = ::Index( ::display, ::Saved::Playing ); },
            .rmb = []() { ::EnumNext( ::Saved::Playback, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); },
            .mmb = []() { ::EnumNext( ::Saved::Sorting, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); ::Sort(); },
            .xmb = []( int d ) { ::queue::next( d ); },
            .scrl = []( int s ) { ::SetVolume( s * -( 0.001 + 0.009 * ( int )::Input::State[ VK_SHIFT ] ) ); }
        };

        bool search = ::GridType == ::GridTypes::Search;

        if ( search )
            c.key = []( ::DWORD key ) {
                if ( ::Searching.empty() && ::Search.empty() )
                    ::Search = ::display;

                switch ( ::String::Key( ::Searching, key ) ) {
                    case 0:
                        return;
                    case 1:
                            if ( ::Searching == L"NULL" ) {
                                ::Search.clear();
                                for ( int i = 0; i < ::display.size(); i++ )
                                    if ( ::EmptyImage( ::Saved::Songs[ ::display[ i ] ].Minicover, ARRAYSIZE( ::Saved::Songs[ ::display[ i ] ].Minicover ) ) )
                                        ::Search.push_back( ::display[ i ] );
                            } else
                                for ( int i = ::Search.size() - 1; i >= 0; i-- )
                                    if ( !::wcsstr( ::Saved::Songs[ ::Search[ i ] ].Path, ::Searching.c_str() ) )
                                        ::Search.erase( ::Search.begin() + i );
                        break;
                    case 2:
                            ::Search.clear();
                            for ( int i = 0; i < ::display.size(); i++ )
                                if ( ::wcsstr( ::Saved::Songs[ ::display[ i ] ].Path, ::Searching.c_str() ) )
                                    ::Search.push_back( ::display[ i ] );
                        break;
                }

                ::Sort();
            };

        ::DrawImage( rect, ::Playing.Cover, c );

        bool active = rect == ::Input::hover;

        if ( active ) {
            ::DefaultDisplay( ::Saved::Playing );
            ::DisplayInformation[ 4 ] = []() { return ::String::WConcat( ( int )::cursor, L" / ", ::Saved::Songs[ ::Saved::Playing ].Duration, L"s" ); };
            ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 3 ] = []() { return ::String::WConcat( L"Grid: ", ::GTNames[ ::GridType ] ); };
            ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 2 ] = []() { return ::String::WConcat( L"Sorting: ", ::STNames[ ::Saved::Sorting ] ); };
            ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 1 ] = []() { return ::String::WConcat( L"Playback: ", ::PBNames[ ::Saved::Playback ] ); };

            if ( search )
                ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) / 2 ] = []() { return ::String::WConcat( L"Searching: ", ::Searching ); };
        }

        if ( ::DisplayInformation[ 0 ] )
            for ( int i = 0; i < ARRAYSIZE( ::DisplayInformation ); i++ )
                if ( ::DisplayInformation[ i ] ) {
                    ::std::wstring info = ::DisplayInformation[ i ]();
                    ::DrawString( 0, WINHEIGHT - ::MIDPOINT + ( FONTHEIGHT + SPACING ) * i, ::MIDPOINT, info );
                }

        LastHover = ::Input::hover;
        LastCover = ::Saved::Playing;
        LastSearch = ::Searching;
        LastCursor = active ? ::cursor : -1;
        LastGrid = ::GridType;
        LastSorting = ::Saved::Sorting;
        LastPlayback = ::Saved::Playback;
        LastVolume = ::Saved::Volumes[ ::Saved::Playing ];
    }
} Cover;