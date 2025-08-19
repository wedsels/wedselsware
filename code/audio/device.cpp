#include "audio.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

static ::ma_context Context;

void DataCallback( ::ma_device* device, void* output, const void* input, ::ma_uint32 framecount ) {
    ::Decode( device, ( ::uint8_t* )output, framecount );
}

void CleanDevice() {
    ::ma_device_uninit( &::Device );
    ::ma_context_uninit( &::Context );
}

void SetVolume( double v ) {
    ::Saved::Volumes[ ::Saved::Playing ] = ::std::clamp( ::Saved::Volumes[ ::Saved::Playing ] + v, 0.0, 1.0 );
    ::ma_device_set_master_volume( &::Device, ( float )::Saved::Volumes[ ::Saved::Playing ] );
}

::HRESULT InitDevice() {
    ::av_log_set_level( AV_LOG_QUIET );

    ::ma_device_config config = ::ma_device_config_init( ::ma_device_type_playback );
    config.dataCallback = ::DataCallback;

    ::ma_uint32 playbackCount;
    ::ma_device_info* pPlaybackInfos;
    HR( ::ma_context_init( NULL, 0, NULL, &::Context ) );
    HR( ::ma_context_get_devices( &::Context, &pPlaybackInfos, &playbackCount, NULL, NULL) );

    for ( ::ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice++ )
        if ( pPlaybackInfos[ iDevice ].isDefault ) {
            config.playback.pDeviceID = &pPlaybackInfos[ iDevice ].id;
            break;
        }

    HR( ::ma_device_init( &::Context, &config, &::Device ) );
    HR( ::ma_device_start( &::Device ) );

    return S_OK;
}