#include "common.hpp"
#include "ui/ui.hpp"

#define ESAFECALL( type ) do { if ( ::Input::clicks[ over ].type ) ::Input::clicks[ over ].type(); } while ( 0 )
#define ISAFECALL( type, i ) do { if ( ::Input::clicks[ over ].type ) ::Input::clicks[ over ].type( i ); } while ( 0 )
#define BLOCKCALL( type, msg, lparam ) do { if ( !::Input::passthrough && ::Input::clicks[ over ].type ) { ::Message( msg, wParam, lparam ); return -1; } } while ( 0 )

::Rect over = {};

void Mouse( int c, ::LPARAM l ) {
    switch( c ) {
        case WM_MOUSEMOVE: {
                ::Rect o = ::Input::hover;
                ::Input::hover = *( ::Rect* )l;

                if ( !::Input::hover.empty() ) {
                    ESAFECALL( hvr );
                    if ( o != ::Input::hover && ::Input::State[ VK_LBUTTON ].load( ::std::memory_order_relaxed ) )
                        ESAFECALL( lmb );
                }
        }   break;
        case WM_LBUTTONDOWN:
                ESAFECALL( lmb );
            break;
        case WM_RBUTTONDOWN:
                ESAFECALL( rmb );
            break;
        case WM_MBUTTONDOWN:
                ESAFECALL( mmb );
            break;
        case WM_XBUTTONDOWN:
                ISAFECALL( xmb, l );
            break;
        case WM_MOUSEWHEEL:
                ISAFECALL( scrl, l );
            break;
        default:
            return;
    }

    ::memset( ::DisplayInformation, 0, sizeof( ::DisplayInformation ) );
    ::Draw( ::DrawType::Redo );
}

void Keyboard( int c, ::LPARAM l ) {
    bool down = c == WM_KEYDOWN || c == WM_SYSKEYDOWN;
    bool up = c == WM_KEYUP || c == WM_SYSKEYUP;

    if ( !up )
        ISAFECALL( key, l );

    ::Draw( ::DrawType::Redo );
}

void SetState( ::DWORD key, bool down ) {
    ::Input::State[ key ].store( down, ::std::memory_order_relaxed );
    ::Input::State[ VK_MENU ].store( ::Input::State[ VK_LMENU ] || ::Input::State[ VK_RMENU ], ::std::memory_order_relaxed );
    ::Input::State[ VK_SHIFT ].store( ::Input::State[ VK_LSHIFT ] || ::Input::State[ VK_RSHIFT ], ::std::memory_order_relaxed );
    ::Input::State[ VK_CONTROL ].store( ::Input::State[ VK_LCONTROL ] || ::Input::State[ VK_RCONTROL ], ::std::memory_order_relaxed );
}

::LRESULT CALLBACK InputProc( int nCode, ::WPARAM wParam, ::LPARAM lParam ) {
    if ( nCode == HC_ACTION ) {
        if ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP ) {
            ::DWORD key = ( ( ::KBDLLHOOKSTRUCT* )lParam )->vkCode;
            bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

            if ( ::Input::globalkey.contains( key ) ) {
                if ( ::Input::globalkey[ key ]( down ) ) {
                    ::Redraw( ::DrawType::Redo );
                    return -1;
                }
            } else if ( !down && ::Input::State[ key ].load( ::std::memory_order_relaxed ) || !::Input::hover.empty() ) {
                ::SetState( key, down );
                BLOCKCALL( key, WM_KEYBOARD, ( ( ::KBDLLHOOKSTRUCT* )lParam )->vkCode );
            } else ::SetState( key, down );
        } else switch ( wParam ) {
            case WM_MOUSEMOVE: {
                    ::Input::mouse = ( ( ::MSLLHOOKSTRUCT* )lParam )->pt;
                    ::Input::mouse.x -= WINLEFT;
                    ::Input::mouse.y -= WINTOP;

                    over = {};

                    ::RECT cr;
                    if ( !::Input::passthrough && !::PauseDraw && ( !::GetClipCursor( &cr ) || cr.right > WINLEFT ) )
                        if ( ::Input::mouse.x > 0 && ::Input::mouse.x < ::MIDPOINT && ::Input::mouse.y > 0 && ::Input::mouse.y < WINHEIGHT )
                            for ( auto& i : ::Input::clicks )
                                if ( i.first.within( ::Input::mouse ) ) {
                                    over = i.first;
                                    break;
                                }

                    if ( ::Input::hover != over )
                        ::Message( WM_MOUSE, wParam, reinterpret_cast< ::WPARAM >( &over ) );
            }   break;
            case WM_LBUTTONDOWN:
                    ::Input::State[ VK_LBUTTON ].store( !::Input::hover.empty(), ::std::memory_order_relaxed );
                    if ( ::Input::State[ VK_LBUTTON ].load( ::std::memory_order_relaxed ) )
                        BLOCKCALL( lmb, WM_MOUSE, 0 );
                break;
            case WM_LBUTTONUP:
                    ::Input::State[ VK_LBUTTON ].store( false, ::std::memory_order_relaxed );
                    BLOCKCALL( lmb, WM_MOUSE, 0 );
                break;
            case WM_RBUTTONDOWN:
                    ::Input::State[ VK_RBUTTON ].store( !::Input::hover.empty(), ::std::memory_order_relaxed );
                    if ( ::Input::State[ VK_RBUTTON ].load( ::std::memory_order_relaxed ) )
                        BLOCKCALL( rmb, WM_MOUSE, 0 );
                break;
            case WM_RBUTTONUP:
                    ::Input::State[ VK_RBUTTON ].store( false, ::std::memory_order_relaxed );
                    BLOCKCALL( rmb, WM_MOUSE, 0 );
                break;
            case WM_MBUTTONDOWN:
                    ::Input::State[ VK_MBUTTON ].store( !::Input::hover.empty(), ::std::memory_order_relaxed );
                    if ( ::Input::State[ VK_MBUTTON ].load( ::std::memory_order_relaxed ) )
                        BLOCKCALL( mmb, WM_MOUSE, 0 );
                break;
            case WM_MBUTTONUP:
                    ::Input::State[ VK_MBUTTON ].store( false, ::std::memory_order_relaxed );
                    BLOCKCALL( mmb, WM_MOUSE, 0 );
                break;
            case WM_XBUTTONDOWN:
                    BLOCKCALL( xmb, WM_MOUSE, GET_XBUTTON_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) * 2 - 3 );
                break;
            case WM_XBUTTONUP:
                    BLOCKCALL( xmb, WM_MOUSE, GET_XBUTTON_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) * 2 - 3 );
                break;
            case WM_MOUSEWHEEL:
                    BLOCKCALL( scrl, WM_MOUSE, GET_WHEEL_DELTA_WPARAM( ( ( ::MSLLHOOKSTRUCT* )lParam )->mouseData ) > 0 ? -1 : 1 );
                break;
        }
    }

    return ::CallNextHookEx( NULL, nCode, wParam, lParam );
}

::HRESULT InitInput() {
    THREAD(
        ::HHOOK hKey = ::SetWindowsHookExA( WH_KEYBOARD_LL, ::InputProc, NULL, 0 );
        ::HHOOK hMouse = ::SetWindowsHookExA( WH_MOUSE_LL, ::InputProc, NULL, 0 );

        ::MSG msg = { 0 };
        while ( ::GetMessageW( &msg, NULL, 0, 0 ) && msg.message != WM_QUIT );
    );

    return S_OK;
}