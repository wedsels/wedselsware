#include "../audio/audio.hpp"

int lastportrait = -1;

::std::unordered_map< ::GridTypes, ::std::wstring > GTNames = {
    { ::GridTypes::Queue, L"Queue" },
    { ::GridTypes::Songs, L"Songs" },
    { ::GridTypes::Search, L"Search" },
    { ::GridTypes::Mixer, L"Mixer" }
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
            ::rect rect { 0, WINHEIGHT - MIDPOINT, MIDPOINT };

            ::input::click c {
                .lmb = []() { ::EnumNext( ::GridType ); ::DisplayOffset = ::Index( ::display, ::Saved::Playing ); },
                .rmb = []() { ::EnumNext( ::Saved::Playback ); },
                .mmb = []() { ::EnumNext( ::Saved::Sorting ); ::Sort(); },
                .xmb = []( int d ) { ::queue::next( d ); },
                .scrl = []( int s ) { ::SetVolume( s * -0.001 ); }
            };

            bool search = ::GridType == ::GridTypes::Search;

            if ( search )
                c.key = []( int key ) {
                    ::string::key( ::Searching, key );
                    for ( auto& i : ::display ) {
                        int index = ::Index( ::Search, i );
                        bool find =
                            ::string::supper( ::songs[ i ].title ).find( ::Searching ) != ::std::wstring::npos
                            || ::string::supper( ::songs[ i ].artist ).find( ::Searching ) != ::std::wstring::npos;

                        if ( index < 0 && find )
                            ::Search.push_back( i );
                        else if ( index > -1 && !find )
                            ::Search.erase( ::Search.begin() + index );

                        ::Sort();
                    }
                };

            ::DrawImage(
                rect,
                ::Playing.Cover,
                ( ::DisplayInformation[ 0 ] && ::input::hover != rect ) ? 0.5f : 1.0f,
                c
            );

            if ( rect == ::input::hover ) {
                ::DisplayInformation[ 0 ] = []() { return ::songs[ ::Saved::Playing ].title; };
                ::DisplayInformation[ 1 ] = []() { return ::songs[ ::Saved::Playing ].artist; };
                ::DisplayInformation[ 2 ] = []() { return ::songs[ ::Saved::Playing ].encoding; };
                ::DisplayInformation[ 3 ] = []() { return ::string::wconcat( ( int )::cursor, L"s / ", ::Playing.Duration, L"s" ); };
                ::DisplayInformation[ 4 ] = []() { return ::string::wconcat( ::Playing.Size, L"mb" ); };
                ::DisplayInformation[ 5 ] = []() { return ::string::wconcat( ::Playing.Bitrate, L"kbps" ); };
                ::DisplayInformation[ 6 ] = []() { return ::string::wconcat( ::Playing.Samplerate, L"Hz" ); };
                ::DisplayInformation[ 7 ] = []() { return ::string::wconcat( ::Saved::Volumes[ ::Saved::Playing ], L"%" ); };
                ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 3 ] = []() { return ::string::wconcat( L"Grid: ", ::GTNames[ ::GridType ] ); };
                ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 2 ] = []() { return ::string::wconcat( L"Sorting: ", ::STNames[ ::Saved::Sorting ] ); };
                ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) - 1 ] = []() { return ::string::wconcat( L"Playback: ", ::PBNames[ ::Saved::Playback ] ); };

                if ( search )
                    ::DisplayInformation[ ARRAYSIZE( DisplayInformation ) / 2 ] = []() { return ::string::wconcat( L"Searching: ", ::Searching ); };
            }

            if ( ::DisplayInformation[ 0 ] )
                for ( int i = 0; i < ARRAYSIZE( ::DisplayInformation ); i++ )
                    if ( ::DisplayInformation[ i ] ) {
                        ::std::wstring info = ::DisplayInformation[ i ]();
                        ::DrawString( 0, WINHEIGHT - MIDPOINT + ( FONTHEIGHT + SPACING ) * i, MIDPOINT, info );
                    }
        }
        ::lastportrait = ::Saved::Playing;
    }
};
::Cover Cover;