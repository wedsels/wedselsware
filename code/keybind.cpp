#include "common.hpp"
#include "audio/audio.hpp"

namespace Input {
    ::std::unordered_map< int, ::std::function< bool( bool ) > > globalkey = {
        { VK_SNAPSHOT, []( bool down ) {
            if ( down ) {
                if ( ::Input::State::ctrl ) {
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
                if ( ::Input::State::ctrl )
                    ::queue::next( 1 );
                else
                    ::SetVolume( 0.05 );
            }

            return true;
        } },
        { VK_END, []( bool down ) {
            if ( down ) {
                if ( ::Input::State::ctrl )
                    ::queue::next( -1 );
                else
                    ::SetVolume( -0.05 );
            }

            return true;
        } },
        { VK_CAPITAL, []( bool down ) {
            if ( !down ) {
                if ( ::Input::State::ctrl )
                    ::Execute( L"cmd.exe", 2 );
                else if ( ::Input::State::alt )
                    ::SetDefaultDevice();
                else
                    ::Execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
            }

            return true;
        } },
        { VK_LMENU, []( bool down ) { ::Input::State::alt = down; return false; } },
        { VK_RMENU, []( bool down ) { ::Input::State::alt = down; return false; } },
        { VK_LSHIFT, []( bool down ) { ::Input::State::shift = down; return false; } },
        { VK_RSHIFT, []( bool down ) { ::Input::State::shift = down; return false; } },
        { VK_LCONTROL, []( bool down ) { ::Input::State::ctrl = down; return false; } },
        { VK_RCONTROL, []( bool down ) { ::Input::State::ctrl = down; return false; } }
    };
}