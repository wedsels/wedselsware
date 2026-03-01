#include "audio/audio.hpp"

bool FileReady( ::std::wstring& p ) {
    ::HANDLE h = ::CreateFileW( p.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

    if ( h == INVALID_HANDLE_VALUE )
        return false;

    ::CloseHandle( h );

    return true;
}

::std::vector< ::std::wstring > IterateDirectory( const ::std::filesystem::path& path ) {
    ::std::vector< ::std::wstring > paths;

    if ( ::std::filesystem::is_regular_file( path ) && !::std::filesystem::is_symlink( path ) )
        paths.push_back( path.wstring() );
    else if ( ::std::filesystem::is_directory( path ) ) {
        for ( const auto& entry : ::std::filesystem::directory_iterator( path, ::std::filesystem::directory_options::skip_permission_denied ) )
            paths.insert_range( paths.end(), ::IterateDirectory( entry.path() ) );

        if ( ::std::filesystem::is_empty( path ) )
            ::std::filesystem::remove( path );
    }

    return paths;
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
    launch.Path = res;

    launch.IMG = ::ArchiveHICON( res.c_str(), MINICOVER );

    ids.push_back( id );
    map.emplace( id, launch );
}

void DeleteLink( ::uint32_t id, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map ) {
    int i = ::Index( ids, id );
    if ( i < 0 )
        return;

    if ( map[ id ].IMG ) {
        ::delete[] map[ id ].IMG;
        map[ id ].IMG = nullptr;
    }

    ids.erase( ids.begin() + i );
    map.erase( id );
}

void SortLink( ::std::vector< ::uint32_t >& display, ::std::unordered_map< ::uint32_t, ::Launch >& names ) {
    ::std::sort( display.begin(), display.end(), [ & ]( ::uint32_t a, ::uint32_t b ) {
        return names[ a ].Path < names[ b ].Path;
    } );
}

::HRESULT InitDirectory( const wchar_t* path, ::std::function< void( ::std::wstring& ) > add, ::std::function< void( ::std::wstring& ) > remove, ::std::function< void() > sort ) {
    THREAD(
        ::std::vector< ::std::wstring > fileremove = {};
        ::std::vector< ::std::wstring > fileadd = {};
        ::relaxed_atomic< bool > update = true;

        for ( const auto& i : ::std::filesystem::directory_iterator( path, ::std::filesystem::directory_options::skip_permission_denied ) )
            fileadd.push_back( i.path().wstring() );

        THREAD(
            while ( true ) {
                update.v.wait( false );
                update = false;

                for ( auto& i : fileremove )
                    remove( i );
                fileremove.clear();

                ::std::vector< ::std::wstring > keep = {};

                for ( auto& i : fileadd )
                    for ( auto& f : ::IterateDirectory( i ) )
                        if ( ::FileReady( f ) )
                            add( f );
                        else keep.push_back( f );
                fileremove = keep;

                sort();

                ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 500 ) );
            }
            , &
        );

        ::WatchDirectory( path, [ & ]( int action, const wchar_t* name ) {
            ::std::wstring fpath = ::String::WConcat( path, name );
            ::Path( fpath );

            if ( action == FILE_ACTION_ADDED || action == FILE_ACTION_RENAMED_NEW_NAME )
                fileadd.push_back( fpath );
            else if ( action == FILE_ACTION_REMOVED || action == FILE_ACTION_RENAMED_OLD_NAME )
                fileremove.push_back( fpath );

            update = true;
            update.v.notify_one();
        } );
        , =
    );

    return S_OK;
}