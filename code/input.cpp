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
                    if ( o != ::Input::hover && ::Input::State[ VK_LBUTTON ] )
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

::LRESULT CALLBACK InputProc( int nCode, ::WPARAM wParam, ::LPARAM lParam ) {
    if ( nCode == HC_ACTION && ~nCode < 0 ) {
        if ( wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN || wParam == WM_KEYUP || wParam == WM_SYSKEYUP ) {
            ::DWORD key = ( ( ::KBDLLHOOKSTRUCT* )lParam )->vkCode;
            bool down = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;

            ::Input::State[ key ] = down;
            ::Input::State[ VK_MENU ] = ::Input::State[ VK_LMENU ] || ::Input::State[ VK_RMENU ];
            ::Input::State[ VK_SHIFT ] = ::Input::State[ VK_LSHIFT ] || ::Input::State[ VK_RSHIFT ];
            ::Input::State[ VK_CONTROL ] = ::Input::State[ VK_LCONTROL ] || ::Input::State[ VK_RCONTROL ];

            if ( auto it = ::Input::globalkey.find( key ); it != ::Input::globalkey.end() )
                if ( it->second( down ) ) {
                    ::Redraw( ::DrawType::Redo );
                    return -1;
                }

            BLOCKCALL( key, WM_KEYBOARD, ( ( ::KBDLLHOOKSTRUCT* )lParam )->vkCode );
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
                    ::Input::State[ VK_LBUTTON ] = true;
                    BLOCKCALL( lmb, WM_MOUSE, 0 );
                break;
            case WM_LBUTTONUP:
                    ::Input::State[ VK_LBUTTON ] = false;
                    BLOCKCALL( lmb, WM_MOUSE, 0 );
                break;
            case WM_RBUTTONDOWN:
                    ::Input::State[ VK_RBUTTON ] = true;
                    BLOCKCALL( rmb, WM_MOUSE, 0 );
                break;
            case WM_RBUTTONUP:
                    ::Input::State[ VK_RBUTTON ] = false;
                    BLOCKCALL( rmb, WM_MOUSE, 0 );
                break;
            case WM_MBUTTONDOWN:
                    ::Input::State[ VK_MBUTTON ] = true;
                    BLOCKCALL( mmb, WM_MOUSE, 0 );
                break;
            case WM_MBUTTONUP:
                    ::Input::State[ VK_MBUTTON ] = false;
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