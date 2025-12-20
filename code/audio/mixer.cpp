#include "audio.hpp"

#include <shellapi.h>
#include <audiopolicy.h>

inline ::IAudioSessionManager2* pSessionManager;
inline ::std::unordered_map< ::uint32_t, ::std::unordered_set< ::ISimpleAudioVolume* > > instances;

void Clean( ::uint32_t entry ) {
    ::Volume& t = MixerEntries[ entry ];

    for ( const auto& i : ::instances[ entry ] )
        if ( i )
            i->Release();

    ::instances.erase( entry );
    ::MixerEntries.erase( entry );
    ::MixersActive.erase( ::MixersActive.begin() + ::Index( ::MixersActive, entry ) );
}

void SetMixerVolume( ::uint32_t entry, double change ) {
    if ( !::MixerEntries.contains( entry ) )
        return;

    double volume = ::std::clamp( ::Saved::Mixers[ entry ] + change, 0.0, 1.0 );

    for ( const auto& v : ::instances[ entry ] )
        if ( !v || FAILED( v->SetMasterVolume( ( float )volume, NULL ) ) )
            return ::SetMixers();
        else ::Saved::Mixers[ entry ] = volume;
}

void SetMixers() {
    for ( auto& i : ::MixerEntries )
        ::Clean( i.first );

    ::IAudioSessionEnumerator* pSessionEnumerator;
    ::pSessionManager->GetSessionEnumerator( &pSessionEnumerator );

    int sessionCount;
    pSessionEnumerator->GetCount( &sessionCount );

    for ( int i = 0; i < sessionCount; ++i ) {
        ::IAudioSessionControl* pSessionControl;
        pSessionEnumerator->GetSession( i, &pSessionControl );

        ::IAudioSessionControl2* pSessionControl2;
        pSessionControl->QueryInterface( __uuidof( ::IAudioSessionControl2 ), ( ::LPVOID* )&pSessionControl2 );

        if ( pSessionControl2 ) {
            ::DWORD processId;
            pSessionControl2->GetProcessId( &processId );

            ::HANDLE hProcess = ::OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId );
            if ( hProcess != NULL ) {
                wchar_t p[ MAX_PATH ];
                ::DWORD dwSize = MAX_PATH;
                if ( ::QueryFullProcessImageNameW( hProcess, 0, p, &dwSize ) != 0 ) {
                    ::ISimpleAudioVolume* pVolume;
                    pSessionControl->QueryInterface( __uuidof( ::ISimpleAudioVolume ), ( ::LPVOID* )&pVolume );

                    if ( pVolume ) {
                        ::std::wstring name( p );
                        name = name.substr( name.find_last_of( '\\' ) + 1 );
                        name = name.substr( 0, name.find( '.' ) );

                        ::uint32_t id = ::String::Hash( name );

                        if ( !::Saved::Mixers.contains( id ) )
                            ::Saved::Mixers[ id ] = 0.45;

                        if ( !FAILED( pVolume->SetMasterVolume( ( float )::Saved::Mixers[ id ], NULL ) ) ) {
                            if ( !::MixerEntries.contains( id ) ) {
                                ::MixerEntries[ id ] = {};
                                ::wcsncpy_s( ::MixerEntries[ id ].name, ::MINIPATH, name.c_str(), ::MINIPATH - 1 );

                                ::uint8_t* icon = ::ArchiveHICON( p, MINICOVER );
                                if ( icon ) {
                                    ::MixerEntries[ id ].minicover[ ARRAYSIZE( ::MixerEntries[ id ].minicover ) - 1 ] = 255;
                                    ::std::memcpy( ::MixerEntries[ id ].minicover, icon, ARRAYSIZE( ::MixerEntries[ id ].minicover ) - 1 );
                                }
                                ::delete[] icon;

                                ::instances[ id ] = {};
                                ::MixersActive.push_back( id );
                            }
                            ::instances[ id ].insert( pVolume );
                        }
                        else pVolume->Release();
                    } else pVolume->Release();
                }
            }

            ::CloseHandle( hProcess );
        }

        pSessionControl2->Release();
        pSessionControl->Release();
    }

    pSessionEnumerator->Release();
}

void CALLBACK WinEventProc( ::HWINEVENTHOOK hook, ::DWORD event, ::HWND hwnd, ::LONG idObject, ::LONG idChild, ::DWORD dwEventThread, ::DWORD dwmsEventTime ) {
    if ( event == EVENT_OBJECT_CREATE
        && idObject == OBJID_WINDOW
        && ::IsWindow( hwnd )
        && ::GetParent( hwnd ) == nullptr
        && ::GetAncestor( hwnd, GA_ROOT ) == hwnd
        && ( ::GetWindowLongPtrW( hwnd, GWL_STYLE ) & WS_CHILD ) == 0
        && ( ::GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW ) == 0
    ) ::Message( WM_MIXER, 0, 0 );
}

::HRESULT InitializeMixer() {
    ::HWINEVENTHOOK hook = ::SetWinEventHook( EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, NULL, ::WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT );

    ::IMMDeviceEnumerator* enumerator;
    HR( ::CoCreateInstance( __uuidof( ::MMDeviceEnumerator ), NULL, CLSCTX_ALL, __uuidof( ::IMMDeviceEnumerator ), ( ::LPVOID* )&enumerator ) );

    ::IMMDevice* device;
    enumerator->GetDefaultAudioEndpoint( ::eRender, ::eConsole, &device );
    device->Activate( __uuidof( ::IAudioSessionManager2 ), CLSCTX_ALL, NULL, ( ::LPVOID* )&::pSessionManager );
    enumerator->Release();
    device->Release();

    ::SetMixers();

    return S_OK;
}