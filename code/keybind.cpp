#include "common.hpp"
#include "audio/audio.hpp"

namespace Input {
    ::std::unordered_map< ::DWORD, ::std::function< bool() > > globalkey = {
        { VK_HOME, []() {
            if ( PRESSED( VK_HOME ) ) {
                if ( ( ::PauseAudio = !::PauseAudio ) )
                    ::SetThreadExecutionState( ES_CONTINUOUS );
                else
                    ::SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
            }

            return true;
        } },
        { VK_PRIOR, []() {
            if ( PRESSED( VK_PRIOR ) ) {
                if ( HELD( VK_CONTROL ) )
                    ::queue::next( 1 );
                else
                    ::Saved::Volumes[ ::Saved::Playing ] = ::std::max( 0.0, ::Saved::Volumes[ ::Saved::Playing ] + 0.05 );
            }

            return true;
        } },
        { VK_NEXT, []() {
            if ( PRESSED( VK_NEXT ) ) {
                if ( HELD( VK_CONTROL ) )
                    ::queue::next( -1 );
                else
                    ::Saved::Volumes[ ::Saved::Playing ] = ::std::max( 0.0, ::Saved::Volumes[ ::Saved::Playing ] - 0.05 );
            }

            return true;
        } },
        { VK_CAPITAL, []() {
            if ( PRESSED( VK_CAPITAL ) ) {
                if ( HELD( VK_CONTROL ) )
                    ::Execute( L"cmd.exe", 2 );
                else if ( HELD( VK_MENU ) )
                    ::SetDefaultDevice();
                else if ( HELD( VK_SHIFT ) )
                    ::Execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
                else
                    ::ShowWindow( ::consolehwnd, ::IsWindowVisible( ::consolehwnd ) ? SW_HIDE : SW_SHOW );
            }

            return true;
        } }
    };
}