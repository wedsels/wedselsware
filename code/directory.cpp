#include "audio/audio.hpp"

struct Directory {
    const wchar_t* path;
    ::std::function< void( const wchar_t* ) > add;
    ::std::function< void( ::uint32_t ) > remove;
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
                dir.add( dir.it->path().wstring().c_str() );

            ++dir.it;
        }

        Updated &= all;
    }

    if ( Updated )
        ::std::vector< Directory >().swap( Directories );
}

bool FileReady( const wchar_t* p ) {
    for ( int i = 0; i < 5; ++i )
        if ( ::std::ifstream( p, ::std::ios::binary ).good() )
            return true;
        else
            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 500 ) );

    return false;
}

void WatchDirectory( const wchar_t* path, ::std::function< void( int, const wchar_t* ) > action ) {
    ::HANDLE hDir = ::CreateFileW(
        path,
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
            ::DWORD offset = 0;

            do {
                ::FILE_NOTIFY_INFORMATION* fni = reinterpret_cast< ::FILE_NOTIFY_INFORMATION* >( buffer + offset );
                ::std::wstring name( fni->FileName, fni->FileNameLength / sizeof( ::WCHAR ) );
                action( fni->Action, name.c_str() );

                if ( fni->NextEntryOffset == 0 )
                    break;

                offset += fni->NextEntryOffset;
            } while ( true );
        }
    }

    ::CloseHandle( hDir );
}

void ArchiveLink( ::std::wstring path, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map ) {
    ::std::wstring res = ::String::ResolveLnk( path );
    if ( res.empty() )
        res = path;

    ::uint32_t id = ::String::Hash( path );

    ids.push_back( id );
    map.emplace( id, ::Launch{ .path = res, .img = ::ArchiveHICON( res.c_str(), MINICOVER ) } );
}

void DeleteLink( ::uint32_t id, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map ) {
    int i = ::Index( ids, id );
    if ( i < 0 )
        return;

    ::delete[] map[ id ].img;
    ids.erase( ids.begin() + i );
    map.erase( id );
}

::HRESULT InitializeDirectory( const wchar_t* path, ::std::function< void( const wchar_t* ) > add, ::std::function< void( ::uint32_t ) > remove ) {
    ::Directory dir = { path, add, remove, ::std::filesystem::directory_iterator( path ) };
    Directories.push_back( dir );

    THREAD(
        ::WatchDirectory( dir.path, [ = ]( int action, const wchar_t* name ) {
            ::std::function< void() > func = [ = ]() {
                ::std::wstring fpath = ::String::WConcat( dir.path, name );

                if ( action == FILE_ACTION_ADDED || action == FILE_ACTION_RENAMED_NEW_NAME ) {
                    if ( ::FileReady( fpath.c_str() ) )
                        dir.add( fpath.c_str() );
                } else if ( action == FILE_ACTION_REMOVED || action == FILE_ACTION_RENAMED_OLD_NAME )
                    dir.remove( ::String::Hash( fpath ) );
            };

            ::SendMessageW( hwnd, WM_ACTION, ( ::WPARAM )&func, 0 );
        } );
    );

    return S_OK;
}