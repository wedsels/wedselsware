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

    SERIALIZE( Playing );
    SERIALIZE( Sorting );
    SERIALIZE( Playback );
    SERIALIZE( Queue );
    SERIALIZE( Queues );
    SERIALIZE( Volumes );
    SERIALIZE( Mixers );
}

void Load() {
    if ( ::Loaded )
        return;

    ::Deserializer d;

    DESERIALIZE( Playing );
    DESERIALIZE( Sorting );
    DESERIALIZE( Playback );
    DESERIALIZE( Queue );
    DESERIALIZE( Queues );
    DESERIALIZE( Volumes );
    DESERIALIZE( Mixers );

    if ( ::Saved::Queues.empty() || ::Saved::Queue >= ::Saved::Queues.size() ) {
        ::Saved::Queue = 0;
        ::Saved::Queues = { { ::Saved::Playing } };
    }

    ::Loaded = true;
}

::LONG WINAPI Crash( ::EXCEPTION_POINTERS* ) {
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
    if ( MQUIT( msg ) )
        ::Save();

    switch ( msg ) {
        case WM_NCHITTEST:
            return HTTRANSPARENT;
        case WM_FUNCTION: {
                ::std::function< void() >* f = ( ::std::function< void() >* )( lParam );
                if ( f ) ( *f )();
            } break;
        case WM_VOID: {
                auto* v = ( void( * )() )( lParam );
                if ( v ) ( *v )();
            } break;
        case WM_SLEEP:
            if ( wParam )
                ::SetThreadExecutionState( ES_CONTINUOUS );
            else
                ::SetThreadExecutionState( ES_CONTINUOUS | ES_DISPLAY_REQUIRED );
    }

    return ::DefWindowProcW( hwnd, msg, wParam, lParam );
}

::HWND Window( ::HINSTANCE hInstance ) {
    ::SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );

    ::WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = ::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"wedselswindow";
    wc.hIcon = ( ::HICON )::LoadImageW( hInstance, MAKEINTRESOURCEW( IDI_ICON1 ), IMAGE_ICON, 256, 256, LR_SHARED );

    ::RegisterClassW( &wc );

    ::HWND hwnd = ::CreateWindowExW(
        NULL,
        wc.lpszClassName,
        wc.lpszClassName,
        WS_POPUP | WS_VISIBLE,
        WINLEFT, WINTOP, WINWIDTH, WINHEIGHT,
        nullptr, nullptr, hInstance, nullptr
    );

    ::ShowWindow( hwnd, SW_SHOW );

    return hwnd;
}

void RemoveSong( ::std::wstring& p ) {
    ::uint32_t hash = ::String::Hash( p );

    if ( ::Library.contains( hash ) )
        ::Remove( hash );
    else
        for ( auto& i : ::Library | ::std::views::reverse )
            if ( i.second.Path.contains( p ) )
                ::Remove( i.first );
}

void SyncDirectory( const ::std::wstring& source, const ::std::wstring& destination ) {
    ::std::filesystem::copy(
        source,
        destination,
        ::std::filesystem::copy_options::recursive | ::std::filesystem::copy_options::skip_existing
    );

    ::std::vector< ::std::filesystem::path > remove = {};

    for ( const ::std::filesystem::path& entry : ::std::filesystem::recursive_directory_iterator( destination ) )
        if ( !::std::filesystem::exists( source / ::std::filesystem::relative( entry, destination ) ) )
            remove.push_back( entry );
    
    for ( auto& i : remove )
        ::std::filesystem::remove_all( i );
}

::HRESULT Main( ::HINSTANCE hInstance ) {
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

    HR( ::CoInitialize( NULL ) );

    ::hwnd = ::Window( hInstance );
    ::desktophwnd = ::FindWindowExW( ::FindWindowW( L"Progman", NULL ), NULL, L"SHELLDLL_DefView", NULL );

    HR( ::InitFont() );
    HR( ::InitGraphics() );
    HR( ::InitDevice() );
    HR( ::InitMixer() );
    HR( ::InitAudio() );
    HR( ::InitKeys() );
    HR( ::InitInput() );

    ::SyncDirectory( ::SongPath, L"E:/Sounds" );

    HR( ::InitDirectory( L"E:/Apps/", []( ::std::wstring& p ) { ::ArchiveLink( p, ::Apps, ::AppsPath ); }, []( ::std::wstring& p ) { ::DeleteLink( ::String::Hash( p ), ::Apps, ::AppsPath ); }, []() { ::SortLink( ::Apps, ::AppsPath ); } ) );
    HR( ::InitDirectory( L"E:/Webs/", []( ::std::wstring& p ) { ::ArchiveLink( p, ::Webs, ::WebsPath ); }, []( ::std::wstring& p ) { ::DeleteLink( ::String::Hash( p ), ::Webs, ::WebsPath ); }, []() { ::SortLink( ::Webs, ::WebsPath ); } ) );
    HR( ::InitDirectory( ::SongPath.c_str(), ::ArchiveSong, ::RemoveSong, ::Sort ) );

    ::MSG msg = { 0 };
    while ( ::GetMessageW( &msg, NULL, 0, 0 ) ) {
        if ( MQUIT( msg.message ) )
            break;

        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );
    }

    ::Save();

    return S_OK;
}

int WINAPI wWinMain( ::HINSTANCE hInstance, ::HINSTANCE, ::PWSTR, int ) {
    TRY( ::Main( hInstance ) );

    ::Save();

    return 0;
}