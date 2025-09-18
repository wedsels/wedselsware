#include "audio/audio.hpp"

struct Directory {
    ::std::wstring path;
    ::std::function< void( ::std::wstring ) > add;
    ::std::function< void( ::std::wstring ) > remove;
    mutable ::std::filesystem::directory_iterator it;
};

::std::vector< Directory > Directories = {};

void UpdateDirectories( ::MSG& msg ) {
    static bool Updated;
    static ::std::filesystem::directory_iterator EndIt;

    if ( Updated ) return;

    Updated = true;
    bool all = false;

    for ( auto& dir : Directories ) {
        while ( !( all = dir.it == EndIt ) && !::PeekMessageW( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
            if ( dir.it->is_regular_file() && !dir.it->is_symlink() )
                dir.add( dir.it->path().wstring() );

            ++dir.it;
        }

        Updated &= all;
    }

    if ( Updated )
        ::std::vector< Directory >().swap( Directories );
}

bool FileReady( const std::wstring& p ) {
    for ( int i = 0; i < 5; ++i )
        if ( ::std::ifstream( p, ::std::ios::binary ).good() )
            return true;
        else
            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 500 ) );

    return false;
}

void WatchDirectory( const std::wstring& path, ::std::function< void( int, ::std::wstring& ) > action ) {
    ::HANDLE hDir = ::CreateFileW(
        path.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL
    );

    if ( hDir == INVALID_HANDLE_VALUE )
        return;

    char buffer[ 1024 ];
    ::DWORD bytesReturned;

    while ( true ) {
        if ( ::ReadDirectoryChangesW(
            hDir,
            &buffer,
            sizeof( buffer ),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME,
            &bytesReturned,
            NULL,
            NULL
        ) ) {
            ::FILE_NOTIFY_INFORMATION* fni = reinterpret_cast< ::FILE_NOTIFY_INFORMATION* >( buffer );
            do {
                ::std::wstring fileName( fni->FileName, fni->FileNameLength / sizeof( ::WCHAR ) );
                action( fni->Action, fileName );

                if ( fni->NextEntryOffset == 0 )
                    break;
                fni = reinterpret_cast< ::FILE_NOTIFY_INFORMATION* >( reinterpret_cast< ::BYTE* >( fni ) + fni->NextEntryOffset );
            } while ( true );
        }
    }

    ::CloseHandle( hDir );
}

::HRESULT InitializeDirectory( ::std::wstring path, ::std::function< void( ::std::wstring ) > add, ::std::function< void( ::std::wstring ) > remove ) {
    ::Directory dir;
    dir.path = path;
    dir.add = add;
    dir.remove = remove;
    dir.it = ::std::filesystem::directory_iterator( path );
    Directories.push_back( dir );

    THREAD( ::WatchDirectory( path, [ &dir ]( int action, ::std::wstring& name ) {
        ::std::function< void() > func = [ action, &name, &dir ]() {
            ::std::wstring fpath = ::string::wconcat( dir.path, name );

            if ( action == FILE_ACTION_ADDED || action == FILE_ACTION_RENAMED_NEW_NAME ) {
                if ( ::FileReady( fpath ) )
                    dir.add( fpath );
            } else if ( action == FILE_ACTION_REMOVED || action == FILE_ACTION_RENAMED_OLD_NAME )
                dir.remove( fpath );
        };

        ::SendMessageW( hwnd, WM_ACTION, ( ::WPARAM )&func, 0 );
    } ); );

    return S_OK;
}