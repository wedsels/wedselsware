#include "audio/audio.hpp"

void WatchDirectory( const std::wstring& directory, ::std::function< void( int, ::std::wstring& ) > action ) {
    ::HANDLE hDir = ::CreateFileW(
        directory.c_str(),
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

bool FileReady( const std::wstring& p ) {
    for ( int i = 0; i < 5; ++i )
        if ( ::std::ifstream( p, ::std::ios::binary ).good() )
            return true;
        else
            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 500 ) );

    return false;
}

::std::wstring drs[] = {
    L"F:/SoundStuff/Sounds/",
    L"E:/Webs/",
    L"E:/Apps/"
};

::HRESULT InitializeDirectory() {
    THREAD( ::WatchDirectory( drs[ 0 ], []( int action, ::std::wstring& name ) {
        ::std::function< void() > func = [ action, &name ]() {
            ::std::wstring path = ::string::wconcat( drs[ 0 ], name );
            ::uint32_t id = ::string::hash( path );

            if ( action == FILE_ACTION_ADDED || action == FILE_ACTION_RENAMED_NEW_NAME ) {
                if ( ::FileReady( path ) )
                    ::ArchiveSong( path );
            } else if ( action == FILE_ACTION_REMOVED || action == FILE_ACTION_RENAMED_OLD_NAME )
                ::Remove( id );
        };

        ::SendMessageW( hwnd, WM_ACTION, ( ::WPARAM )&func, 0 );
    } ); );

    return S_OK;
}