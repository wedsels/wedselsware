#include "common.hpp"
#include "binary.hpp"
#include "audio/audio.hpp"

#define IDI_ICON1 101

#include <shellapi.h>
#include <DbgHelp.h>
#include <csignal>

inline bool Loaded = false;

void Save() {
    if ( !::Loaded )
        return;
    ::Loaded = false;

    ::Serializer s;

    SERIALIZE( Apps );
    SERIALIZE( AppsPath );
    SERIALIZE( Webs );
    SERIALIZE( WebsPath );

    SERIALIZE( Playing );
    SERIALIZE( Sorting );
    SERIALIZE( Playback );
    SERIALIZE( Queue );
    SERIALIZE( Queues );
    SERIALIZE( Volumes );
    SERIALIZE( Mixers );
    SERIALIZE( Songs );
}

void Load() {
    if ( ::Loaded )
        return;

    ::Deserializer d;

    DESERIALIZE( Apps );
    DESERIALIZE( AppsPath );
    DESERIALIZE( Webs );
    DESERIALIZE( WebsPath );

    DESERIALIZE( Playing );
    DESERIALIZE( Sorting );
    DESERIALIZE( Playback );
    DESERIALIZE( Queue );
    DESERIALIZE( Queues );
    DESERIALIZE( Volumes );
    DESERIALIZE( Mixers );
    DESERIALIZE( Songs );

    for ( auto& i : ::Saved::Songs | ::std::views::reverse )
        if ( ::std::filesystem::exists( i.second.Path ) )
            ::SongDisplay.push_back( i.first );
        else
            ::Remove( i.first );
    ::Sort();

    if ( ::Saved::Queues.empty() || ::Saved::Queue >= ::Saved::Queues.size() ) {
        ::Saved::Queue = 0;
        ::Saved::Queues = { { ::Saved::Playing } };
    }

    ::Loaded = true;
}

::LONG WINAPI Crash( ::EXCEPTION_POINTERS* exceptionInfo ) {
    ::Save();

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

    ::UpdateDirectories();

    switch ( msg ) {
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_QUEUENEXT:
                ::queue::next( 1 );
            return NULL;
        case WM_FUNCTION: {
                ::std::function< void() >* f = ( ::std::function< void() >* )( lParam );
                if ( f ) ( *f )();
            } return NULL;
        default:
            return ::DefWindowProcA( hwnd, msg, wParam, lParam );
    }
}

::HWND Window( ::HINSTANCE hInstance ) {
    ::SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );

    ::WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Wedsels' Window";
    wc.hIcon = ( ::HICON )::LoadImageW( hInstance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 256, 256, LR_SHARED );

    ::RegisterClassW( &wc );

    ::HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        wc.lpszClassName,
        wc.lpszClassName,
        WS_POPUP | WS_VISIBLE,
        WINLEFT, WINTOP, WINWIDTH, WINHEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    ::SetLayeredWindowAttributes( hwnd, 0, 255, LWA_ALPHA );
    ::ShowWindow( hwnd, SW_SHOW );

    return hwnd;
}

void SyncDirectory( const ::std::wstring& source, const ::std::wstring& destination ) {
    ::std::filesystem::copy(
        source,
        destination,
        ::std::filesystem::copy_options::recursive | ::std::filesystem::copy_options::skip_existing
    );

    for ( const ::std::filesystem::path& entry : ::std::filesystem::recursive_directory_iterator( L"E:/Sounds" ) )
        if ( !::std::filesystem::exists( source / ::std::filesystem::relative( entry, destination ) ) )
            ::std::filesystem::remove_all( entry );
}

int WINAPI wWinMain( ::HINSTANCE hInstance, ::HINSTANCE, ::PWSTR, int ) {
    ::SetUnhandledExceptionFilter( ::Crash );
    ::SetConsoleCtrlHandler( ::CtrlHandler, TRUE );
    ::std::atexit( ::Save );
    ::std::set_terminate( ::Save );

    ::signal( SIGINT, []( int ) { ::Save(); } );
    ::signal( SIGTERM, []( int ) { ::Save(); } );
    ::signal( SIGSEGV, []( int ) { ::Save(); } );
    ::signal( SIGABRT, []( int ) { ::Save(); } );

    ::AllocConsole();
    ::ShowWindow( ::consolehwnd = ::GetConsoleWindow(), SW_HIDE );
    ::FILE* fp;
    ::freopen_s( &fp, "CONOUT$", "w", stdout );
    ::freopen_s( &fp, "CONOUT$", "w", stderr );
    ::freopen_s( &fp, "CONIN$", "r", stdin );

    ::std::ofstream log( "log", ::std::ios::out );
    ::std::cerr.rdbuf( log.rdbuf() );

    ::Load();   

    ::hwnd = ::Window( hInstance );
    ::desktophwnd = ::FindWindowExW( ::FindWindowW( L"Progman", NULL ), NULL, L"SHELLDLL_DefView", NULL );

    HER( ::CoInitialize( NULL ) );

    HER( ::InitDirectory( L"E:/Apps/", []( ::std::wstring p ) { ::ArchiveLink( p, ::Saved::Apps, ::Saved::AppsPath ); }, []( ::uint32_t id ) { ::DeleteLink( id, ::Saved::Apps, ::Saved::AppsPath ); } ) );
    HER( ::InitDirectory( L"E:/Webs/", []( ::std::wstring p ) { ::ArchiveLink( p, ::Saved::Webs, ::Saved::WebsPath ); }, []( ::uint32_t id ) { ::DeleteLink( id, ::Saved::Webs, ::Saved::WebsPath ); } ) );
    HER( ::InitDirectory( ::SongPath.c_str(), ::ArchiveSong, ::Remove ) );

    HER( ::InitFont() );
    HER( ::InitGraphics() );
    HER( ::InitDevice() );
    HER( ::InitInput() );
    HER( ::InitMixer() );

    ::SyncDirectory( ::SongPath, L"E:/Sounds" );

    ::MSG msg = { 0 };
    while ( ::GetMessageW( &msg, NULL, 0, 0 ) ) {
        if ( msg.message == WM_QUIT )
            break;

        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );
    }

    ::Save();

    return 0;
}