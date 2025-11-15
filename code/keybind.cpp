#include "common.hpp"
#include "audio/audio.hpp"

namespace Input {
    ::std::unordered_map< int, ::std::function< bool( bool ) > > globalkey = {
        { VK_HOME, []( bool down ) {
            if ( down ) {
                if ( ::Input::State::ctrl )
                    if ( ( ::PauseDraw = !::PauseDraw ) )
                        ::InitiateDraw();
                    else ::Draw( ::DrawType::Redo );
                else
                    if ( ( ::PauseAudio = !::PauseAudio ) )
                        ::SetThreadExecutionState( ES_CONTINUOUS );
                    else
                        ::SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
            }

            return true;
        } },
        { VK_PRIOR, []( bool down ) {
            if ( down ) {
                if ( ::Input::State::ctrl )
                    ::queue::next( 1 );
                else
                    ::SetVolume( 0.05 );
            }

            return true;
        } },
        { VK_NEXT, []( bool down ) {
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
                else if ( ::Input::State::shift ) {
                    ::RECT rect = { WINLEFT + ::MIDPOINT, WINTOP, WINLEFT + ::MIDPOINT + WINWIDTH - ::MIDPOINT, WINTOP + WINHEIGHT };
                    ::HWND h = ::GetForegroundWindow();
                    ::AdjustWindowRectEx( &rect, ::GetWindowLongW( h, GWL_STYLE ), FALSE, ::GetWindowLongW( h, GWL_EXSTYLE ) );
                    ::MoveWindow( h, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE );
                }
                else
                    ::Execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
            }

            return true;
        } },
        { VK_END, []( bool down ) { if ( down ) ::Input::passthrough = !::Input::passthrough; return true; } },
        { VK_LMENU, []( bool down ) { ::Input::State::alt = down; return false; } },
        { VK_RMENU, []( bool down ) { ::Input::State::alt = down; return false; } },
        { VK_LSHIFT, []( bool down ) { ::Input::State::shift = down; return false; } },
        { VK_RSHIFT, []( bool down ) { ::Input::State::shift = down; return false; } },
        { VK_LCONTROL, []( bool down ) { ::Input::State::ctrl = down; return false; } },
        { VK_RCONTROL, []( bool down ) { ::Input::State::ctrl = down; return false; } }
    };
}