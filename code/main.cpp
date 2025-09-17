#include "common.hpp"
#include "audio/audio.hpp"

#define IDI_ICON1 101

#include <DbgHelp.h>
#include <csignal>

void Save() {
    ::std::ofstream o( "wedselsdata", ::std::ios::binary );
    if ( !o ) return;

    o.write( reinterpret_cast< char* >( &::Saved::Playing ), sizeof( ::Saved::Playing ) );
    o.write( reinterpret_cast< char* >( &::Saved::Sorting ), sizeof( ::Saved::Sorting ) );
    o.write( reinterpret_cast< char* >( &::Saved::Playback ), sizeof( ::Saved::Playback ) );

    ::uint32_t queueSize = ::Saved::Queue.size();
    o.write( reinterpret_cast< char* >( &queueSize ), sizeof( queueSize ) );
    o.write( reinterpret_cast< char* >( ::Saved::Queue.data() ), queueSize * sizeof( ::uint32_t ) );

    ::uint32_t mapSize = ::Saved::Volumes.size();
    o.write( reinterpret_cast< char* >( &mapSize ), sizeof( mapSize ) );
    for ( const auto& [ key, value ] : ::Saved::Volumes ) {
        o.write( reinterpret_cast< const char* >( &key ), sizeof( key ) );
        o.write( reinterpret_cast< const char* >( &value ), sizeof( value ) );
    }

    ::uint32_t mixerSize = ::Saved::Mixers.size();
    o.write( reinterpret_cast< char* >( &mixerSize ), sizeof( mixerSize ) );
    for ( const auto& [ key, value ] : ::Saved::Mixers ) {
        o.write( reinterpret_cast< const char* >( &key ), sizeof( key ) );
        o.write( reinterpret_cast< const char* >( &value ), sizeof( value ) );
    }

    o.close();
}

void Load() {
    ::std::ifstream i( "wedselsdata", ::std::ios::binary );
    if ( !i ) return;

    i.read( reinterpret_cast< char* >( &::Saved::Playing ), sizeof( ::Saved::Playing ) );
    i.read( reinterpret_cast< char* >( &::Saved::Sorting ), sizeof( ::Saved::Sorting ) );
    i.read( reinterpret_cast< char* >( &::Saved::Playback ), sizeof( ::Saved::Playback ) );

    ::uint32_t queueSize;
    i.read( reinterpret_cast< char* >( &queueSize ), sizeof( queueSize ) );
    ::Saved::Queue.resize( queueSize );
    i.read( reinterpret_cast< char* >( ::Saved::Queue.data() ), queueSize * sizeof( ::uint32_t ) );

    ::uint32_t mapSize;
    i.read( reinterpret_cast< char* >( &mapSize ), sizeof( mapSize ) );
    ::Saved::Volumes.clear();
    for ( ::uint32_t u = 0; u < mapSize; ++u ) {
        ::uint32_t key;
        double value;
        i.read( reinterpret_cast< char* >( &key ), sizeof( key ) );
        i.read( reinterpret_cast< char* >( &value ), sizeof( value ) );
        ::Saved::Volumes[ key ] = value;
    }

    ::uint32_t mixerSize;
    i.read( reinterpret_cast< char* >( &mixerSize ), sizeof( mixerSize ) );
    ::Saved::Mixers.clear();
    for ( ::uint32_t u = 0; u < mixerSize; ++u ) {
        ::uint32_t key;
        double value;
        i.read( reinterpret_cast< char* >( &key ), sizeof( key ) );
        i.read( reinterpret_cast< char* >( &value ), sizeof( value ) );
        ::Saved::Mixers[ key ] = value;
    }

    i.close();
}

::LONG WINAPI Crash( ::EXCEPTION_POINTERS* exceptionInfo ) {
    ::Save();

    ::SYSTEMTIME time;
    ::GetSystemTime( &time );
    char dumpFileName[ 256 ];
    ::sprintf_s( dumpFileName, "crash_dump_%04d%02d%02d_%02d%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond );

    ::HANDLE hFile = ::CreateFileA( dumpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
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

    // if ( true ) {
    //     ::AllocConsole();
    //     ::FILE* fp;
    //     ::freopen_s( &fp, "CONOUT$", "w", stdout );
    //     ::freopen_s( &fp, "CONOUT$", "w", stderr );
    //     ::freopen_s( &fp, "CONIN$", "r", stdin );
    // }

    ::Load();

    ::hwnd = ::Window( hInstance );

    HER( ::InitializeDirectory() );
    HER( ::InitializeFont() );
    HER( ::InitGraphics() );
    HER( ::InitDevice() );
    HER( ::InitInput() );
    HER( ::InitDraw() );
    HER( ::InitializeMixer() );

    ::std::filesystem::directory_iterator eit, dit( L"F:/SoundStuff/Sounds/" );

    ::MSG msg = { 0 };
    while ( ::GetMessageW( &msg, NULL, 0, 0 ) ) {
        if ( msg.message == WM_QUIT )
            break;

        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );

        while ( dit != eit && !::PeekMessageW( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
            if ( dit->is_regular_file() && !dit->is_symlink() )
                ::ArchiveSong( dit->path().wstring() );

            ++dit;
        }
    }

    ::Save();

    return 0;
}