#include "../audio/audio.hpp"

::std::vector< ::uint32_t >& GetDisplay() {
    switch ( ::GridType ) {
        case ::GridTypes::Queue: return ::Saved::Queue;
        case ::GridTypes::Search: return ::Search;
        case ::GridTypes::Mixer: return ::MixersActive;
        case ::GridTypes::Apps: return ::Saved::Apps;
        case ::GridTypes::Webs: return ::Saved::Webs;
        default: return ::display;
    }
}

::uint32_t* GetCover( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer: return ::MixerEntries[ entry ].minicover;
        case ::GridTypes::Apps: return ::Saved::AppsPath[ entry ].img;
        case ::GridTypes::Webs: return ::Saved::WebsPath[ entry ].img;
        default: return ::Saved::Songs[ entry ].Minicover;
    }
}

::Input::Click GetClick( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer:
            return {
                .scrl = [ entry ]( int dir ) { ::SetMixerVolume( entry, dir * -( 0.01 + 0.009 * ( int )::Input::State[ VK_SHIFT ] ) ); }
            };
        case ::GridTypes::Apps: return { .lmb = [ entry ]() { ::Execute( ::Saved::AppsPath[ entry ].path ); } };
        case ::GridTypes::Webs: return { .lmb = [ entry ]() { ::Execute( ::Saved::WebsPath[ entry ].path ); } };
        default:
            return {
                .lmb = [ entry ]() {
                    ::uint32_t id = ::Saved::Songs[ entry ].ID;
                    int index = ::Index( ::Saved::Queue, id );

                    if ( ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) )
                        ::queue::set( id, ::Index( ::Saved::Queue, id ) > -1 ? 0 : 1 );
                    else if ( ::GridType == ::GridTypes::Queue && index > -1 )
                        ::Saved::Queue.erase( ::Saved::Queue.begin() + index );
                    else
                        ::queue::add( id, 1 );
                },
                .rmb = [ entry ]() { ::Execute( ::Saved::Songs[ entry ].Path, 1 ); },
                .mmb = []() { ::queue::clear(); },
                .xmb = []( int d ) { ::queue::next( d ); },
                .scrl = []( int s ) { ::DisplayOffset += s; }
            };
    }
}

void DefaultDisplay( ::uint32_t entry ) {
    ::DisplayInformation[ 0 ] = [ entry ]() { return ::Saved::Songs[ entry ].Title; };
    ::DisplayInformation[ 1 ] = [ entry ]() { return ::Saved::Songs[ entry ].Artist; };
    ::DisplayInformation[ 2 ] = [ entry ]() { return ::Saved::Songs[ entry ].Album; };
    ::DisplayInformation[ 3 ] = [ entry ]() { return ::Saved::Songs[ entry ].Encoding; };
    ::DisplayInformation[ 4 ] = [ entry ]() { return ::String::WConcat( ::Saved::Songs[ entry ].Duration, L"s" ); };
    ::DisplayInformation[ 5 ] = [ entry ]() { return ::String::WConcat( ::Saved::Songs[ entry ].Size, L"mb" ); };
    ::DisplayInformation[ 6 ] = [ entry ]() { return ::String::WConcat( ::Saved::Songs[ entry ].Bitrate, L"kbps" ); };
    ::DisplayInformation[ 7 ] = [ entry ]() { return ::String::WConcat( ::Saved::Songs[ entry ].Samplerate, L"Hz" ); };
    ::DisplayInformation[ 8 ] = [ entry ]() { return ::String::WConcat( ::Saved::Volumes[ entry ], L"%" ); };
}

void GetDisplay( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::MixerEntries[ entry ].name; };
                ::DisplayInformation[ 1 ] = [ entry ]() { return ::String::WConcat( ::Saved::Mixers[ entry ], L"%" ); };
            break;
        case ::GridTypes::Apps:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::Saved::AppsPath[ entry ].path; };
            break;
        case ::GridTypes::Webs:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::Saved::WebsPath[ entry ].path; };
            break;
        default:
                ::DefaultDisplay( entry );
            break;
    }
}

void DrawSlide() {
    static constexpr int grid = COLUMNS * ROWS;

    ::std::vector< ::uint32_t >& Display = ::GetDisplay();

    float l = 1.0f;
    ::Rect r = {
        0,
        ROWS * ( MINICOVER + SPACING ),
        ::MIDPOINT,
        ROWS * ( MINICOVER + SPACING ) + SPACING * 2
    };

    ::DrawBox(
        r,
        COLORGHOST,
        ::Input::Click{
            .lmb = []() { ::DisplayOffset = ::std::max( ( int )::Saved::Songs.size() - grid, grid ) * ( ::Input::mouse.x / ( double )MIDPOINT ); },
            .rmb = []() { ::DisplayOffset = 0;  },
            .scrl = []( int s ) { ::DisplayOffset += s; }
        }
    );

    r.l = ( int )( ( ::MIDPOINT - SPACING * 2 ) * ( ::DisplayOffset / ( ::std::max( ( int )Display.size() - grid, grid ) + 1.0f ) ) );
    r.r = r.l + SPACING * 2;
    ::DrawBox( r, COLORCORAL );
}

struct Grid : ::UI {
    int LastOffset;
    ::GridTypes LastGrid = ::GridTypes::Count;
    ::Rect LastHover;

    void Clear() {
        LastOffset = 0;
        LastGrid = ::GridTypes::Count;
    }

    void Draw() {
        if ( LastOffset == ::DisplayOffset && LastGrid == ::GridType && LastHover == ::Input::hover )
            return;

        ::std::vector< ::uint32_t >& display = ::GetDisplay();

        ::DisplayOffset = ::std::clamp( ::DisplayOffset, 0, ::std::max( 0, ( int )display.size() - COLUMNS * ROWS ) );

        ::DrawSlide();

        for ( int i = 0; i < COLUMNS * ROWS; i++ ) {
            int loc = i + ::DisplayOffset;
            ::Rect rect { ( MINICOVER + SPACING ) * ( i % COLUMNS ), ( MINICOVER + SPACING ) * ( i / COLUMNS ), MINICOVER };

            if ( loc >= display.size() )
                ::DrawBox( rect, COLORALPHA );
            else
                ::DrawImage( rect, ::GetCover( display[ loc ] ), ::GetClick( display[ loc ] ) );

            if ( rect == ::Input::hover )
                ::GetDisplay( display[ loc ] );
        }

        LastOffset = ::DisplayOffset;
        LastGrid = ::GridType;
        LastHover = ::Input::hover;
    }
} Grid;