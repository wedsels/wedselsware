#include "../audio/audio.hpp"

int lasttile = -1;

::std::vector< ::uint32_t >& GetDisplay() {
    switch ( ::GridType ) {
        case ::GridTypes::Queue: return ::Saved::Queue;
        case ::GridTypes::Search: return ::Search;
        case ::GridTypes::Mixer: return ::MixersActive;
        case ::GridTypes::Apps: return ::Apps;
        case ::GridTypes::Webs: return ::Webs;
        default: return ::display;
    }
}

::uint8_t GetChannels( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer: return 4;
        case ::GridTypes::Apps: return 4;
        case ::GridTypes::Webs: return 4;
        default: return 3;
    }
}

::uint8_t* GetCover( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer: return ::MixerEntries[ entry ].minicover;
        case ::GridTypes::Apps: return ::AppsPath[ entry ].img;
        case ::GridTypes::Webs: return ::WebsPath[ entry ].img;
        default: return ::songs[ entry ].minicover;
    }
}

::Input::Click GetClick( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer:
            return {
                .scrl = [ entry ]( int dir ) { ::SetMixerVolume( entry, dir * -0.01 ); }
            };
        case ::GridTypes::Apps: return { .lmb = [ entry ]() { ::Execute( ::AppsPath[ entry ].path ); } };
        case ::GridTypes::Webs: return { .lmb = [ entry ]() { ::Execute( ::WebsPath[ entry ].path ); } };
        default:
            return {
                .lmb = [ entry ]() {
                    ::uint32_t id = ::songs[ entry ].id;
                    int index = ::Index( ::Saved::Queue, id );

                    if ( ::Input::State::shift )
                        ::queue::set( id, ::Index( ::Saved::Queue, id ) > -1 ? 0 : 1 );
                    else if ( ::GridType == ::GridTypes::Queue && index > -1 )
                        ::Saved::Queue.erase( ::Saved::Queue.begin() + index );
                    else
                        ::queue::add( id, 1 );
                },
                .rmb = [ entry ]() { ::Execute( ::songs[ entry ].path, 1 ); },
                .mmb = []() { ::queue::clear(); },
                .xmb = []( int d ) { ::queue::next( d ); },
                .scrl = []( int s ) { ::DisplayOffset += s; }
            };
    }
}

void GetDisplay( ::uint32_t entry ) {
    switch ( ::GridType ) {
        case ::GridTypes::Mixer:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::MixerEntries[ entry ].name; };
                ::DisplayInformation[ 1 ] = [ entry ]() { return ::String::WConcat( ::Saved::Mixers[ entry ], L"%" ); };
            break;
        case ::GridTypes::Apps:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::AppsPath[ entry ].path; };
            break;
        case ::GridTypes::Webs:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::WebsPath[ entry ].path; };
            break;
        default:
                ::DisplayInformation[ 0 ] = [ entry ]() { return ::songs[ entry ].title; };
                ::DisplayInformation[ 1 ] = [ entry ]() { return ::songs[ entry ].artist; };
                ::DisplayInformation[ 2 ] = [ entry ]() { return ::songs[ entry ].encoding; };
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
        MIDPOINT,
        ROWS * ( MINICOVER + SPACING ) + SPACING * 2
    };

    ::DrawBox(
        r,
        COLORGHOST,
        l,
        ::Input::Click{
            .lmb = []() { ::DisplayOffset = ::std::max( ( int )::songs.size() - grid, grid ) * ( ::Input::mouse.x / ( double )MIDPOINT ); },
            .rmb = []() { ::DisplayOffset = 0;  },
            .scrl = []( int s ) { ::DisplayOffset += s; }
        }
    );

    r.l = ( int )( ( MIDPOINT - SPACING * 2 ) * ( ::DisplayOffset / ( ::std::max( ( int )Display.size() - grid, grid ) + 1.0f ) ) );
    r.r = r.l + SPACING * 2;
    l = 1.0f;
    ::DrawBox( r, COLORCORAL, l );
}

struct Grid : ::UI {
    void Draw( ::DrawType dt ) {
        ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

        ::std::vector< ::uint32_t >& display = ::GetDisplay();

        ::DisplayOffset = ::std::clamp( ::DisplayOffset, 0, ::std::max( 0, ( int )display.size() - COLUMNS * ROWS ) );

        if ( dt > ::DrawType::Normal || ::DisplayOffset != ::lasttile ) {
            ::DrawSlide();

            for ( int i = 0; i < COLUMNS * ROWS; i++ ) {
                int loc = i + ::DisplayOffset;
                ::Rect rect { ( MINICOVER + SPACING ) * ( i % COLUMNS ), ( MINICOVER + SPACING ) * ( i / COLUMNS ), MINICOVER };

                if ( loc >= display.size() ) {
                    float l = 1.0f;
                    ::DrawBox( rect, COLORALPHA, l );
                } else
                    ::DrawImage(
                        rect,
                        ::GetCover( display[ loc ] ),
                        1.0f,
                        ::GetClick( display[ loc ] ),
                        ::GetChannels( display[ loc ] )
                    );

                if ( rect == ::Input::hover )
                    ::GetDisplay( display[ loc ] );
            }
        }
        ::lasttile = ::DisplayOffset;
    }
};
::Grid Grid;