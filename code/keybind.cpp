#include "common.hpp"
#include "audio/audio.hpp"

namespace Input {
    ::std::unordered_map< ::DWORD, ::std::function< bool( bool ) > > globalkey = {
        { VK_HOME, []( bool down ) {
            if ( down ) {
                if ( ::Input::State[ VK_CONTROL ].load( ::std::memory_order_relaxed ) )
                    ::PauseDraw = !::PauseDraw;
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
                if ( ::Input::State[ VK_CONTROL ].load( ::std::memory_order_relaxed ) )
                    ::queue::next( 1 );
                else
                    ::SetVolume( 0.05 );
            }

            return true;
        } },
        { VK_NEXT, []( bool down ) {
            if ( down ) {
                if ( ::Input::State[ VK_CONTROL ].load( ::std::memory_order_relaxed ) )
                    ::queue::next( -1 );
                else
                    ::SetVolume( -0.05 );
            }

            return true;
        } },
        { VK_CAPITAL, []( bool down ) {
            if ( !down ) {
                if ( ::Input::State[ VK_CONTROL ].load( ::std::memory_order_relaxed ) )
                    ::Execute( L"cmd.exe", 2 );
                else if ( ::Input::State[ VK_MENU ].load( ::std::memory_order_relaxed ) )
                    ::SetDefaultDevice();
                else if ( ::Input::State[ VK_SHIFT ].load( ::std::memory_order_relaxed ) )
                    ::Seek( 0 );
                else
                    ::Execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
            }

            return true;
        } },
        { VK_END, []( bool down ) { if ( down ) ::Input::passthrough = !::Input::passthrough; return true; } }
    };
}