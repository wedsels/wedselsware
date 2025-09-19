#include "common.hpp"
#include "binary.hpp"
#include "audio/audio.hpp"

#define IDI_ICON1 101

#include <DbgHelp.h>
#include <csignal>

void Save() {
    ::Serializer s;
    s.write( ::Saved::Playing );
    s.write( ::Saved::Sorting );
    s.write( ::Saved::Playback );
    s.write( ::Saved::Queue );
    s.write( ::Saved::Volumes );
    s.write( ::Saved::Mixers );
}

void Load() {
    ::Deserializer d;
    d.read( ::Saved::Playing );
    d.read( ::Saved::Sorting );
    d.read( ::Saved::Playback );
    d.read( ::Saved::Queue );
    d.read( ::Saved::Volumes );
    d.read( ::Saved::Mixers );
}

::LONG WINAPI Crash( ::EXCEPTION_POINTERS* exceptionInfo ) {
    ::Save();

    ::HANDLE hFile = ::CreateFileW(
        ::String::WConcat(
            ::LineInfo::File.substr( ::LineInfo::File.find_last_of( '\\' ) + 1 ).c_str(),
            L" ",
            ::LineInfo::Func.c_str(),
            L" ",
            ::LineInfo::Line,
            L".dmp"
        ).c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if ( hFile == INVALID_HANDLE_VALUE )
        return EXCEPTION_EXECUTE_HANDLER;

    ::MINIDUMP_EXCEPTION_INFORMATION mdei;
    mdei.ThreadId = ::GetCurrentThreadId();
    mdei.ExceptionPointers = exceptionInfo;
    mdei.ClientPointers = FALSE;

    ::MiniDumpWriteDump( ::GetCurrentProcess(), ::GetCurrentProcessId(), hFile, ::MiniDumpNormal, &mdei, NULL, NULL );

    ::CloseHandle( hFile );

    return EXCEPTION_EXECUTE_HANDLER;
}

::BOOL CtrlHandler( ::DWORD c ) {
    if ( c == CTRL_C_EVENT || c == CTRL_BREAK_EVENT || c == CTRL_CLOSE_EVENT || c == CTRL_LOGOFF_EVENT || c == CTRL_SHUTDOWN_EVENT ) {
        ::Save();
        return TRUE;
    }
    return FALSE;
}

::LRESULT CALLBACK WndProc( ::HWND hwnd, ::UINT msg, ::WPARAM wParam, ::LPARAM lParam ) {
    if ( msg == WM_DESTROY || msg == SC_CLOSE || msg == WM_QUERYENDSESSION || msg == WM_ENDSESSION || msg == WM_CLOSE || msg == WM_QUIT ) {
        ::Save();
        return TRUE;
    }

    switch ( msg ) {
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_REDRAW:
                ::Draw( ( ::DrawType )wParam );
            return NULL;
        case WM_MOUSE:
                ::Mouse( wParam, lParam );
            return NULL;
        case WM_KEYBOARD:
                ::Keyboard( wParam, lParam );
            return NULL;
        case WM_QUEUENEXT:
                ::queue::next( 1 );
            return NULL;
        case WM_MIXER:
                ::SetMixers();
                ::Draw( ::DrawType::Redo );
            return NULL;
        case WM_ACTION:
                ( *( ::std::function< void() >* )( wParam ) )();
            return NULL;
        default:
            return ::DefWindowProcA( hwnd, msg, wParam, lParam );
    }
}

::HWND Window( ::HINSTANCE hInstance ) {
    ::WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Wedsels' Window";
    wc.hIcon = ( ::HICON )::LoadImageW( hInstance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 256, 256, LR_SHARED );

    ::RegisterClassW( &wc );

    ::HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
        wc.lpszClassName,
        wc.lpszClassName,
        WS_POPUP | WS_VISIBLE,
        WINLEFT, WINTOP, WINWIDTH, WINHEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    ::SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
    ::SetLayeredWindowAttributes( hwnd, 0, 255, LWA_ALPHA );
    ::SetWindowPos( hwnd, HWND_TOPMOST, WINLEFT, WINTOP, WINWIDTH, WINHEIGHT, SWP_NOACTIVATE );
    ::ShowWindow( hwnd, SW_SHOW );

    return hwnd;
}

int WINAPI wWinMain( ::HINSTANCE hInstance, ::HINSTANCE, ::PWSTR, int ) {
    ::SetUnhandledExceptionFilter( ::Crash );
    ::SetConsoleCtrlHandler( ::CtrlHandler, TRUE );
    ::std::atexit( ::Save );

    ::signal( SIGINT, []( int ) { ::Save(); } );
    ::signal( SIGTERM, []( int ) { ::Save(); } );
    ::signal( SIGSEGV, []( int ) { ::Save(); } );
    ::signal( SIGABRT, []( int ) { ::Save(); } );

    // ::AllocConsole();
    // ::FILE* fp;
    // ::freopen_s( &fp, "CONOUT$", "w", stdout );
    // ::freopen_s( &fp, "CONOUT$", "w", stderr );
    // ::freopen_s( &fp, "CONIN$", "r", stdin );

    ::Load();

    ::hwnd = ::Window( hInstance );

    // HER( ::InitializeDirectory( L"E:/Webs/" ) );
    // HER( ::InitializeDirectory( L"E:/Apps/" ) );
    HER( ::InitializeDirectory( L"F:/SoundStuff/Sounds/", ::ArchiveSong, []( ::std::wstring p ) { ::Remove( ::String::Hash( p ) ); } ) );

    HER( ::InitializeFont() );
    HER( ::InitGraphics() );
    HER( ::InitDevice() );
    HER( ::InitInput() );
    HER( ::InitDraw() );
    HER( ::InitializeMixer() );

    ::MSG msg = { 0 };
    while ( ::GetMessageW( &msg, NULL, 0, 0 ) ) {
        if ( msg.message == WM_QUIT )
            break;

        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );

        ::UpdateDirectories( msg );
    }

    ::Save();

    return 0;
}