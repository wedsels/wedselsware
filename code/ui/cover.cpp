#include "../audio/audio.hpp"

int lastportrait = -1;

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
    virtual ::uint8_t GetPriority() const { return 0; }

    void Draw( ::DrawType dt ) {
        ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

        if ( dt > ::DrawType::Normal || ::Saved::Playing != ::lastportrait ) {
            ::Rect rect { 0, WINHEIGHT - ::MIDPOINT, ::MIDPOINT };

            ::Input::Click c {
                .lmb = []() { ::EnumNext( ::GridType, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); ::DisplayOffset = ::Index( ::display, ::Saved::Playing ); },
                .rmb = []() { ::EnumNext( ::Saved::Playback, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); },
                .mmb = []() { ::EnumNext( ::Saved::Sorting, ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) ); ::Sort(); },
                .xmb = []( int d ) { ::queue::next( d ); },
                .scrl = []( int s ) { ::SetVolume( s * -0.001 ); }
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
                                        if ( !::Saved::Songs[ ::display[ i ] ].Minicover[ MINICOVER * MINICOVER ] )
                                            ::Search.push_back( ::display[ i ] );
                                } else {
                                    for ( int i = ::Search.size() - 1; i >= 0; i-- )
                                        if ( ::wcsstr( ::Saved::Songs[ ::Search[ i ] ].Path, ::Searching.c_str() ) )
                                            ::Search.erase( ::Search.begin() + i );
                                }
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

            ::DrawImage(
                rect,
                ::Playing.Cover,
                ( ::DisplayInformation[ 0 ] && ::Input::hover != rect ) ? 0.5f : 1.0f,
                c
            );

            if ( rect == ::Input::hover ) {
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
        }
        ::lastportrait = ::Saved::Playing;
    }
};
::Cover Cover;