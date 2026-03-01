#include "audio/audio.hpp"

#define GLOBAL( key, ret, body ) { key, []() { if ( PRESSED( key ) ) { THREAD( body ); } return ret; } }

::relaxed_atomic< bool > Kunai;

namespace Input {
    ::std::unordered_map< ::DWORD, ::std::function< bool() > > globalkey = {
        GLOBAL( VK_HOME, true,
            ::Message( WM_SLEEP, ::PauseAudio = !::PauseAudio );
        ),
        GLOBAL( VK_PRIOR, true,
            if ( HELD( VK_CONTROL ) )
                ::queue::next( 1 );
            else
                ::Saved::Volumes[ ::Saved::Playing ] = ::std::max( 0.0, ::Saved::Volumes[ ::Saved::Playing ] + 0.05 );
        ),
        GLOBAL( VK_NEXT, true,
            if ( HELD( VK_CONTROL ) )
                ::queue::next( -1 );
            else
                ::Saved::Volumes[ ::Saved::Playing ] = ::std::max( 0.0, ::Saved::Volumes[ ::Saved::Playing ] - 0.05 );
        ),
        GLOBAL( VK_CAPITAL, ::GetKeyState( VK_CAPITAL ) == 0,
            if ( HELD( VK_CONTROL ) )
                ::Execute( L"C:\\Program Files\\Mozilla Firefox\\firefox.exe" );
            else if ( HELD( VK_SHIFT ) )
                ::Execute( L"cmd.exe", 2 );
            else
                ::ShowWindow( ::consolehwnd, ::IsWindowVisible( ::consolehwnd ) ? SW_HIDE : SW_SHOW );
        ),
        GLOBAL( VK_END, true,
            if ( ( ::Kunai = !::Kunai ) )
                ::Kunai.v.notify_one();
        )
    };
}

::HRESULT InitKeys() {
    THREAD(
        const auto period = ::std::chrono::duration_cast< ::std::chrono::steady_clock::duration >( ::std::chrono::duration< double >( 4.0 ) );

        while ( true ) {
            ::Kunai.v.wait( false );

            ::Stroke( 'F' );
            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 30 ) );
            ::Stroke( 'F' );

            ::std::this_thread::sleep_until( ::std::chrono::steady_clock::now() + period );
        }
    );

    return S_OK;
}