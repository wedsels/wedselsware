#include "audio.hpp"

#include <shellapi.h>
#include <audiopolicy.h>

inline ::IAudioSessionManager2* manager;
inline ::std::unordered_map< ::uint32_t, ::std::unordered_set< ::ISimpleAudioVolume* > > instances;

void SetMixerVolume( ::uint32_t entry, double change ) {
    if ( !::MixerEntries.contains( entry ) )
        return;

    double volume = ::std::clamp( ::Saved::Mixers[ entry ] + change, 0.0, 1.0 );

    for ( const auto& v : ::instances[ entry ] )
        if ( !v || FAILED( v->SetMasterVolume( ( float )volume, NULL ) ) )
            return ::SetMixers();
        else ::Saved::Mixers[ entry ] = volume;
}

void Clean( ::uint32_t entry ) {
    for ( const auto& i : ::instances[ entry ] )
        if ( i )
            i->Release();

    ::instances.erase( entry );
    ::MixerEntries.erase( entry );
    ::MixersActive.erase( ::MixersActive.begin() + ::Index( ::MixersActive, entry ) );
}

::uint32_t AddMixer( ::IAudioSessionControl2* control2 ) {
    ::uint32_t id;

    ::DWORD pid;
    control2->GetProcessId( &pid );

    ::HANDLE hProcess = ::OpenProcess( PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid );
    if ( hProcess != NULL ) {
        wchar_t p[ MAX_PATH ];
        ::DWORD dwSize = MAX_PATH;
        if ( ::QueryFullProcessImageNameW( hProcess, 0, p, &dwSize ) != 0 ) {
            ::ISimpleAudioVolume* pVolume;
            control2->QueryInterface( __uuidof( ::ISimpleAudioVolume ), ( ::LPVOID* )&pVolume );

            if ( pVolume ) {
                ::std::wstring name( p );
                name = name.substr( name.find_last_of( '\\' ) + 1 );
                name = name.substr( 0, name.find( '.' ) );

                id = ::String::Hash( name );

                if ( !::Saved::Mixers.contains( id ) )
                    ::Saved::Mixers[ id ] = 0.25;

                if ( !FAILED( pVolume->SetMasterVolume( ( float )::Saved::Mixers[ id ], NULL ) ) ) {
                    if ( !::MixerEntries.contains( id ) ) {
                        ::MixerEntries[ id ] = {};
                        ::wcsncpy_s( ::MixerEntries[ id ].name, ::MINIPATH, name.c_str(), ::MINIPATH - 1 );

                        ::uint32_t* icon = ::ArchiveHICON( p, MINICOVER );
                        if ( icon )
                            ::std::memcpy( ::MixerEntries[ id ].minicover, icon, sizeof( ::MixerEntries[ id ].minicover ) );
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

    return id;
}

struct SessionEvents : public ::IAudioSessionEvents {
    ::uint32_t entry;
    SessionEvents( ::uint32_t entry ) : entry( entry ) {}

    ::ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ::ULONG STDMETHODCALLTYPE Release() override { return 1; }
    ::HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppv ) override {
        if ( !ppv )
            return E_POINTER;

        if ( riid == __uuidof( ::IUnknown ) || riid == __uuidof( ::IAudioSessionEvents ) ) {
            *ppv = ( ::IAudioSessionEvents* )this;
            return S_OK;
        }

        *ppv = nullptr;

        return E_NOINTERFACE;
    }

    ::HRESULT STDMETHODCALLTYPE OnStateChanged( ::AudioSessionState NewState ) override {
        if ( NewState == ::AudioSessionStateExpired )
            ::Clean( entry );
        return S_OK;
    }

    ::HRESULT STDMETHODCALLTYPE OnSessionDisconnected( ::AudioSessionDisconnectReason ) override {
        ::Clean( entry );

        return S_OK;
    }

    ::HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged( ::DWORD, float[], ::DWORD, ::LPCGUID ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnGroupingParamChanged( ::LPCGUID, ::LPCGUID ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged( float, ::BOOL, ::LPCGUID ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnDisplayNameChanged( ::LPCWSTR, ::LPCGUID ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnIconPathChanged( ::LPCWSTR, ::LPCGUID ) override { return S_OK; }
};

struct SessionNotification : public ::IAudioSessionNotification {
    ::ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ::ULONG STDMETHODCALLTYPE Release() override { return 1; }
    ::HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppv ) override {
        if ( !ppv )
            return E_POINTER;

        if ( riid == __uuidof( ::IUnknown ) || riid == __uuidof( ::IAudioSessionNotification ) ) {
            *ppv = ( ::IAudioSessionNotification* )this;
            return S_OK;
        }

        *ppv = nullptr;

        return E_NOINTERFACE;
    }

    ::HRESULT STDMETHODCALLTYPE OnSessionCreated( ::IAudioSessionControl* NewSession ) override {
        ::IAudioSessionControl2* control2 = nullptr;
        if ( SUCCEEDED( NewSession->QueryInterface( IID_PPV_ARGS( &control2 ) ) ) ) {
            ::DWORD pid = 0;
            control2->GetProcessId( &pid );

            ::SessionEvents* events = ::new ::SessionEvents( ::AddMixer( control2 ) );
            control2->RegisterAudioSessionNotification( events );

            control2->Release();
        }

        return S_OK;
    }
};

void SetMixers() {
    for ( auto& i : ::MixerEntries )
        ::Clean( i.first );

    ::IAudioSessionEnumerator* enumerator;
    ::manager->GetSessionEnumerator( &enumerator );

    int sessionCount;
    enumerator->GetCount( &sessionCount );

    for ( int i = 0; i < sessionCount; ++i ) {
        ::IAudioSessionControl* control;
        enumerator->GetSession( i, &control );

        ::IAudioSessionControl2* control2;
        control->QueryInterface( __uuidof( ::IAudioSessionControl2 ), ( ::LPVOID* )&control2 );

        if ( control2 ) {
            ::SessionEvents* events = ::new ::SessionEvents( ::AddMixer( control2 ) );
            control2->RegisterAudioSessionNotification( events );
        }

        control2->Release();
        control->Release();
    }

    enumerator->Release();
}

::HRESULT InitMixer() {
    ::IMMDeviceEnumerator* enumerator;
    HR( ::CoCreateInstance( __uuidof( ::MMDeviceEnumerator ), NULL, CLSCTX_ALL, __uuidof( ::IMMDeviceEnumerator ), ( ::LPVOID* )&enumerator ) );

    ::IMMDevice* device;
    enumerator->GetDefaultAudioEndpoint( ::eRender, ::eConsole, &device );
    device->Activate( __uuidof( ::IAudioSessionManager2 ), CLSCTX_ALL, NULL, ( ::LPVOID* )&::manager );
    enumerator->Release();
    device->Release();

    ::SessionNotification* notification = ::new ::SessionNotification();
    HR( manager->RegisterSessionNotification( notification ) );

    ::SetMixers();

    return S_OK;
}