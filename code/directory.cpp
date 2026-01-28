#include "audio/audio.hpp"

struct Directory {
    ::std::function< void( ::std::wstring& ) > add;
    ::std::function< void( ::std::wstring& ) > remove;
    mutable ::std::vector< ::std::wstring > fileremove;
    mutable ::std::vector< ::std::wstring > fileadd;
};

::std::unordered_map< const wchar_t*, ::Directory > Directories = {};

bool FileReady( const wchar_t* p ) {
    ::HANDLE h = ::CreateFileW( p, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

    if ( h == INVALID_HANDLE_VALUE )
        return false;

    ::CloseHandle( h );

    return true;
}

::std::vector< ::std::wstring > IterateDirectory( ::MSG& msg, const ::std::filesystem::path& path ) {
    ::std::vector< ::std::wstring > paths;

    if ( ::std::filesystem::is_regular_file( path ) && !::std::filesystem::is_symlink( path ) )
        paths.push_back( path.wstring() );
    else if ( ::std::filesystem::is_directory( path ) ) {
        for ( const auto& entry : ::std::filesystem::directory_iterator( path, ::std::filesystem::directory_options::skip_permission_denied ) )
            paths.insert_range( paths.end(), ::IterateDirectory( msg, entry.path() ) );

        if ( ::std::filesystem::is_empty( path ) )
            ::std::filesystem::remove( path );
    }

    return paths;
}

void UpdateDirectories() {
    ::MSG msg = { 0 };

    for ( auto& dir : ::Directories ) {
        ::Directory& d = dir.second;

        while ( !d.fileremove.empty() && !::PeekMessageW( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
            d.remove( d.fileremove.back() );

            d.fileremove.pop_back();
        }

        bool status = false;
        while ( !status && !d.fileadd.empty() && !::PeekMessageW( &msg, NULL, 0, 0, PM_NOREMOVE ) ) {
            for ( auto& i : ::IterateDirectory( msg, d.fileadd.back() ) )
                if ( ::FileReady( i.c_str() ) )
                    d.add( i );
                else status = true;

            if ( status )
                d.fileadd.emplace( d.fileadd.begin(), d.fileadd.back() );

            d.fileadd.pop_back();
        }
    }
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

    if ( ::Index( ids, id ) > -1 )
        return;

    ::Launch launch;
    ::wcsncpy_s( launch.path, MAX_PATH, res.c_str(), MAX_PATH - 1 );

    ::std::memset( launch.img, 0, sizeof( launch.img ) );

    ::uint32_t* icon = ::ArchiveHICON( res.c_str(), MINICOVER );
    if ( icon )
        ::std::memcpy( launch.img, icon, sizeof( launch.img ) );
    ::delete[] icon;

    ids.push_back( id );
    map.emplace( id, launch );
}

void DeleteLink( ::uint32_t id, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map ) {
    int i = ::Index( ids, id );
    if ( i < 0 )
        return;

    ids.erase( ids.begin() + i );
    map.erase( id );
}

::HRESULT InitDirectory( const wchar_t* path, ::std::function< void( ::std::wstring& ) > add, ::std::function< void( ::std::wstring& ) > remove ) {
    ::Directory dir = { add, remove, {} };

    for ( auto& i : ::std::filesystem::directory_iterator( path, ::std::filesystem::directory_options::skip_permission_denied ) )
        dir.fileadd.push_back( i.path().wstring() );

    ::Directories[ path ] = dir;

    THREAD(
        ::WatchDirectory( path, [ &path ]( int action, const wchar_t* name ) {
            ::std::function< void() > func = [ &path, &action, &name ]() {
                ::std::wstring fpath = ::String::WConcat( path, name );
                ::Path( fpath );

                if ( action == FILE_ACTION_ADDED || action == FILE_ACTION_RENAMED_NEW_NAME )
                    ::Directories[ path ].fileadd.push_back( fpath );
                else if ( action == FILE_ACTION_REMOVED || action == FILE_ACTION_RENAMED_OLD_NAME )
                    ::Directories[ path ].fileremove.push_back( fpath );
            };

            FUNCTION( func );
        } );
    );

    return S_OK;
}