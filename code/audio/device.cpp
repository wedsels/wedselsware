#include "audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ASSERT
#define MA_NO_DEBUG_OUTPUT
#include <miniaudio.h>

#include <atlbase.h>

static ::ma_context Context;
static ::ma_device_config Config;

::HRESULT CleanDevice() {
    ::ma_device_stop( &::Device );
    ::ma_device_uninit( &::Device );
    HR( ::ma_context_uninit( &::Context ) );
    ::InitDevice();

    return S_OK;
}

::HRESULT SetDefaultDevice() {
    ::ma_uint32 count;
    ::ma_device_info* infos;
    HR( ::ma_context_get_devices( &::Context, &infos, &count, NULL, NULL) );

    for ( ::ma_uint32 iDevice = 0; iDevice < count; iDevice++ )
        if ( infos[ iDevice ].isDefault ) {
            ::Config.playback.pDeviceID = &infos[ iDevice ].id;
            break;
        }

    HR( ::ma_device_init( &::Context, &::Config, &::Device ) );
    HR( ::ma_device_start( &::Device ) );

    return S_OK;
}

::HRESULT InitDevice() {
    ::av_log_set_level( AV_LOG_QUIET );

    ::Config = ::ma_device_config_init( ::ma_device_type_playback );
    ::Config.dataCallback = []( ::ma_device* device, void* output, const void*, ::ma_uint32 framecount ) { ::Decode( device, ( ::uint8_t* )output, framecount ); };
    ::Config.notificationCallback = []( const ::ma_device_notification* n ) { if ( n->type == 1 || n->type == 2 ) FUNCTIONVOID( ::CleanDevice, false ); };
    ::Config.playback.format = ::ma_format_f32;
    ::Config.periodSizeInFrames = 0;
    ::Config.sampleRate = 0;
    ::Config.periods = 0;

    HR( ::ma_context_init( NULL, 0, NULL, &::Context ) );

    HR( ::SetDefaultDevice() );

    return S_OK;
}