#include "audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#include <atlbase.h>

static ::ma_context Context;
static ::ma_device_config Config;

struct DeviceWatcher : public ::IMMNotificationClient {
    ::LONG refCount = 1;

    DeviceWatcher() = default;

    ::ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement( &refCount );
    }

    ::ULONG STDMETHODCALLTYPE Release() override {
        ::LONG result = InterlockedDecrement( &refCount );
        if ( result == 0 )
            ::delete this;
        return result;
    }

    ::HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, VOID** ppv ) override {
        if ( riid == __uuidof( ::IUnknown ) || riid == __uuidof( ::IMMNotificationClient ) ) {
            *ppv = static_cast< ::IMMNotificationClient* >( this );
            AddRef();
            return S_OK;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    ::HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged( ::EDataFlow flow, ::ERole role, ::LPCWSTR pwstrDeviceId ) override {
        if ( flow == eRender && role == eMultimedia )
            ::SetDefaultDevice();
        return S_OK;
    }

    ::HRESULT STDMETHODCALLTYPE OnDeviceAdded( ::LPCWSTR ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnDeviceRemoved( ::LPCWSTR ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnDeviceStateChanged( ::LPCWSTR, ::DWORD ) override { return S_OK; }
    ::HRESULT STDMETHODCALLTYPE OnPropertyValueChanged( ::LPCWSTR, const ::PROPERTYKEY ) override { return S_OK; }
};

void CleanDevice() {
    ::ma_device_uninit( &::Device );
    ::ma_context_uninit( &::Context );
}

void SetVolume( double v ) {
    ::Saved::Volumes[ ::Saved::Playing ] = ::std::clamp( ::Saved::Volumes[ ::Saved::Playing ] + v, 0.0, 1.0 );
    ::ma_device_set_master_volume( &::Device, ( float )::Saved::Volumes[ ::Saved::Playing ] );
}

::HRESULT SetDefaultDevice() {
    ::PauseAudio = true;

    ::ma_device_uninit( &::Device );

    ::ma_uint32 count;
    ::ma_device_info* infos;
    HR( ::ma_context_init( NULL, 0, NULL, &::Context ) );
    HR( ::ma_context_get_devices( &::Context, &infos, &count, NULL, NULL) );

    for ( ::ma_uint32 iDevice = 0; iDevice < count; iDevice++ )
        if ( infos[ iDevice ].isDefault ) {
            ::Config.playback.pDeviceID = &infos[ iDevice ].id;
            break;
        }

    HR( ::ma_device_init( &::Context, &::Config, &::Device ) );
    HR( ::ma_device_start( &::Device ) );

    ::SetVolume();

    return S_OK;
}

::HRESULT InitDevice() {
    ::av_log_set_level( AV_LOG_QUIET );

    ::Config = ::ma_device_config_init( ::ma_device_type_playback );
    ::Config.dataCallback = []( ::ma_device* device, void* output, const void* input, ::ma_uint32 framecount ) { ::Decode( device, ( ::uint8_t* )output, framecount ); };

    HR( ::SetDefaultDevice() );

    ::CComPtr< ::IMMDeviceEnumerator > enumerator;
    ::CComPtr< ::DeviceWatcher > client = ::new ::DeviceWatcher();

    HR( ::CoCreateInstance( __uuidof( ::MMDeviceEnumerator ), NULL, CLSCTX_ALL, __uuidof( ::IMMDeviceEnumerator ), ( void** )&enumerator ) );

    enumerator->RegisterEndpointNotificationCallback( client );

    return S_OK;
}