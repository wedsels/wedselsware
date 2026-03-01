#include "common.hpp"
#include "ui/ui.hpp"

#define RETURN do { ::SetHovers( true ); return -1; } while ( 0 )

#define PARENTKEY( type ) do { if ( ::Toggled[ VK_L##type ] || ::Toggled[ VK_R##type ] ) ::Toggled[ VK_##type ] = true; else ::Toggled[ VK_##type ] = false; } while ( 0 )
#define BLOCKKEY( type ) do { bool block = false; for ( auto& i : ::FocusUI ) if ( i->Active() && i->BlockKey() ) { block = true; i->Key(); } if ( block ) RETURN; } while ( 0 )

#define BLOCKCLICKDOWN( call, type ) do { bool block = false; ::Toggled[ type ] = true; for ( auto& i : ::FocusUI ) if ( i->Active() && i->BlockClick() ) { block = true; i->call(); } if ( block ) RETURN; else ::Toggled[ type ] = false; } while ( 0 )
#define BLOCKCLICKUP( call, type ) do { if ( ::Toggled[ type ] ) { ::Toggled[ type ] = false; for ( auto& i : ::FocusUI ) i->call(); RETURN; } } while ( 0 )

#define BLOCKONE( call, type ) do { bool block = false; ::Toggled[ type ] = true; for ( auto& i : ::FocusUI ) if ( i->Active() && i->BlockClick() ) { block = true; i->call(); } ::Toggled[ type ] = false; if ( block ) RETURN; } while ( 0 )

#define EXTERNAL( var, call ) do { auto v = var; ::std::function< void() > f = [ &v ]() { v->call(); }; FUNCTION( f, false ); } while ( 0 )

::std::unordered_set< ::UI* > FocusUI;

::INPUT inputs[ 2 ];
void Stroke( ::WORD key ) {
    key = ::MapVirtualKeyW( key, MAPVK_VK_TO_VSC );
    inputs[ 0 ].ki.wScan = key;
    inputs[ 1 ].ki.wScan = key;

    ::SendInput( 2, inputs, sizeof( ::INPUT ) );
}

void SetHovers( bool clear = false ) {
    for ( auto it = ::FocusUI.begin(); it != ::FocusUI.end(); )
        if ( clear || !( *it )->Active() || !( *it )->Rect().within( ::Input::mouse ) ) {
            if ( !clear ) ( *it )->Leave();
            it = ::FocusUI.erase( it );
        } else ( *it++ )->Move();

    for ( auto& i : ::UI::UIs )
        if ( ::UI::Slide == 0 && i->Active() && !::FocusUI.contains( i ) && i->Rect().within( ::Input::mouse ) ) {
            i->Enter();
            FocusUI.insert( i );
        }
}

#define PROFILE( body ) do { ::std::chrono::time_point s = ::std::chrono::high_resolution_clock::now(); body; ::std::cout<<"Executed: "<<::std::chrono::duration_cast< ::std::chrono::microseconds >( ::std::chrono::high_resolution_clock::now() - s ).count()<<'\n'; } while ( 0 )

::LRESULT CALLBACK InputProc( int nCode, ::WPARAM wParam, ::LPARAM lParam ) {
    if ( nCode == HC_ACTION ) {
        ::std::memcpy( ::OldToggled, ::Toggled, UINT8_MAX );

        if ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP ) {
            ::DWORD key = ( ( ::KBDLLHOOKSTRUCT* )lParam )->vkCode;
            bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

            ::Toggled[ key ] = down;
            PARENTKEY( SHIFT );
            PARENTKEY( CONTROL );
            PARENTKEY( MENU );

            if ( ::Input::globalkey.contains( key ) ) {
                if ( ::Input::globalkey[ key ]() )
                    RETURN;
            } else BLOCKKEY( key );
        } else switch ( wParam ) {
            case WM_MOUSEMOVE: {
                    ::Input::mouse = ( ( ::MSLLHOOKSTRUCT* )lParam )->pt;
                    ::Input::mouse.x -= WINLEFT;
                    ::Input::mouse.y -= WINTOP;

                    if ( ::Input::mouse.x >= 0 && ::Input::mouse.y <= 0 && !::UI::Expand ) {
                        FUNCTIONVOID( ::SetTop, false );
                        FUNCTIONVOID( ::UI::Redraw, false );
                        ::UI::Expand = true;
                    } else if ( ( ::Input::mouse.x < 0 || ::Input::mouse.y > ::UI::MaxSlide + 50 ) && ::UI::Expand )
                        ::UI::Expand = false;

                    ::SetHovers();
            }   break;
            case WM_LBUTTONDOWN:
                    BLOCKCLICKDOWN( Click, VK_LBUTTON );
                break;
            case WM_LBUTTONUP:
                    BLOCKCLICKUP( Click, VK_LBUTTON );
                break;
            case WM_RBUTTONDOWN:
                    BLOCKCLICKDOWN( Click, VK_RBUTTON );
                break;
            case WM_RBUTTONUP:
                    BLOCKCLICKUP( Click, VK_RBUTTON );
                break;
            case WM_MBUTTONDOWN:
                    BLOCKCLICKDOWN( Click, VK_MBUTTON );
                break;
            case WM_MBUTTONUP:
                    BLOCKCLICKUP( Click, VK_MBUTTON );
                break;
            case WM_XBUTTONDOWN: {
                    ::uint64_t x = GET_XBUTTON_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
                    BLOCKCLICKDOWN( XButton, x );
                } break;
            case WM_XBUTTONUP: {
                    ::uint64_t x = GET_XBUTTON_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2;
                    BLOCKCLICKUP( XButton, x );
                } break;
            case WM_MOUSEWHEEL: {
                    ::uint64_t s = GET_WHEEL_DELTA_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) < 0 ? VK_SCROLL1 : VK_SCROLL2;
                    BLOCKONE( Scroll, s );
                } break;
        }
    }

    return ::CallNextHookEx( NULL, nCode, wParam, lParam );
}

::HRESULT InitInput() {
    inputs[ 0 ].type = INPUT_KEYBOARD;
    inputs[ 0 ].ki.wVk = 0;
    inputs[ 0 ].ki.dwFlags = KEYEVENTF_SCANCODE;
    inputs[ 0 ].ki.dwExtraInfo = NULL;
    inputs[ 0 ].ki.time = NULL;

    inputs[ 1 ].type = INPUT_KEYBOARD;
    inputs[ 1 ].ki.wVk = 0;
    inputs[ 1 ].ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    inputs[ 1 ].ki.dwExtraInfo = NULL;
    inputs[ 1 ].ki.time = NULL;

    THREAD(
        ::SetWindowsHookExW( WH_KEYBOARD_LL, ::InputProc, NULL, 0 );
        ::SetWindowsHookExW( WH_MOUSE_LL, ::InputProc, NULL, 0 );

        ::MSG msg = { 0 };
        while ( ::GetMessageW( &msg, NULL, 0, 0 ) ) if ( MQUIT( msg.message ) ) break;
    );

    return S_OK;
}