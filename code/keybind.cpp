#include "common.hpp"
#include "audio/audio.hpp"

namespace input {
    ::std::unordered_map< int, ::std::function< bool( bool ) > > globalkey = {
        { VK_SNAPSHOT, []( bool down ) {
            if ( down ) {
                if ( ::input::state::ctrl ) {
                    if ( ( ::PauseDraw = !::PauseDraw ) )
                        ::InitiateDraw();
                    else ::Draw( ::DrawType::Redo );
                } else
                    if ( ( ::PauseAudio = !::PauseAudio ) )
                        ::SetThreadExecutionState( ES_CONTINUOUS );
                    else
                        ::SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
            }

            return true;
        } },
        { VK_INSERT, []( bool down ) {
            if ( down ) {
                if ( ::input::state::ctrl )
                    ::queue::next( 1 );
                else
                    ::SetVolume( 0.01 );
            }

            return true;
        } },
        { VK_END, []( bool down ) {
            if ( down ) {
                if ( ::input::state::ctrl )
                    ::queue::next( -1 );
                else
                    ::SetVolume( -0.01 );
            }

            return true;
        } },
        { VK_CAPITAL, []( bool down ) {
            if ( !down ) {
                if ( ::input::state::ctrl )
                    ::execute( L"cmd.exe", 2 );
                else if ( ::input::state::alt )
                    ::SetDefaultDevice();
                else
                    ::execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
            }

            return true;
        } },
        { VK_LMENU, []( bool down ) { ::input::state::alt = down; return false; } },
        { VK_RMENU, []( bool down ) { ::input::state::alt = down; return false; } },
        { VK_LSHIFT, []( bool down ) { ::input::state::shift = down; return false; } },
        { VK_RSHIFT, []( bool down ) { ::input::state::shift = down; return false; } },
        { VK_LCONTROL, []( bool down ) { ::input::state::ctrl = down; return false; } },
        { VK_RCONTROL, []( bool down ) { ::input::state::ctrl = down; return false; } },
    };
}