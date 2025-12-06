#include "audio.hpp"

void SetSong( ::uint32_t song ) {
    ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

    if ( !::songs.contains( song ) )
        return;

    ::Clean( ::Playing );

    auto& s = ::songs[ song ];

    if ( FAILED( ::FFMPEG( s.path, ::Playing ) ) ) {
        ::Clean( ::Playing );

        return;
    }

    ::Saved::Playing = s.id;
    ::SetVolume();
    ::swr_inject_silence( ::Playing.SWR, ::Device.sampleRate / 100 * 2 );

    ::cursor = 0;
    ::Redraw( ::DrawType::Redo );

    ::DisplayOffset = ::Index( ::display, s.id );
}

void ArchiveSong( ::std::wstring path ) {
    ::media media = {};
    media.id = ::String::Hash( path );
    media.path = path;
    media.encoding = path.substr( path.rfind( L'.' ) + 1 );

    ::HANDLE file = ::CreateFileW( media.path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    ::FILETIME creationTime;
    ::GetFileTime( file, &creationTime, nullptr, nullptr );
    ::ULARGE_INTEGER ul;
    ul.LowPart = creationTime.dwLowDateTime;
    ul.HighPart = creationTime.dwHighDateTime;
    media.write = ul.QuadPart / 10000;
    ::CloseHandle( file );

    ::Play play = {};

    if ( FAILED( ::FFMPEG( media.path, play ) ) ) {
        ::Clean( play );
        return;
    }

    ::std::wstring artist = L"", title = L"";

    ::AVDictionary* metadata = play.Format->metadata;
    ::AVDictionaryEntry* tag = nullptr;
    while ( ( tag = ::av_dict_get( metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
        if ( ::_stricmp( tag->key, "ARTIST" ) == 0 )
            artist = ::String::Utf8Wide( tag->value );
        else if ( ::_stricmp( tag->key, "TITLE" ) == 0 )
            title = ::String::Utf8Wide( tag->value );

    for ( auto& i : artist )
        if ( i == L'\\' ) continue;
        else if ( i == L';' ) break;
        else media.artist += ::std::toupper( i );
    for ( auto& i : title )
        if ( i == L'\\' ) continue;
        else if ( i == L';' ) break;
        else media.title += ::std::toupper( i );

    media.Duration = play.Duration;
    media.Size = play.Size;
    media.Samplerate = play.Samplerate;
    media.Bitrate = play.Bitrate;

    media.minicover = ::ResizeImage( play.Cover, ::MIDPOINT, ::MIDPOINT, MINICOVER );
    ::Clean( play );

    if ( ::Saved::Volumes.find( media.id ) == ::Saved::Volumes.end() )
        ::Saved::Volumes[ media.id ] = 0.2;

    ::songs[ media.id ] = media;
    ::display.push_back( media.id );

    if ( media.id == ::Saved::Playing )
        ::SetSong( media.id );

    ::Sort();
    ::Draw( ::DrawType::Redo );
}

::HRESULT FFMPEG( ::std::wstring& path, ::Play& Playing ) {
    HR( ::avformat_open_input( &Playing.Format, ::String::WideUtf8( path ).c_str(), 0, 0 ) );

    if ( !Playing.Format ) return E_FAIL;

    HR( ::avformat_find_stream_info( Playing.Format, 0 ) );

    for ( unsigned int i = 0; i < Playing.Format->nb_streams; i++ ) {
        ::AVStream* stream = Playing.Format->streams[ i ];

        if ( !stream || !stream->codecpar )
            continue;
    
        auto id = stream->codecpar->codec_id;
        if ( id == ::AV_CODEC_ID_MJPEG
            || id == ::AV_CODEC_ID_PNG
            || id == ::AV_CODEC_ID_BMP
            || id == ::AV_CODEC_ID_TARGA
            || id == ::AV_CODEC_ID_PSD
            || id == ::AV_CODEC_ID_PICTOR
            || id == ::AV_CODEC_ID_PAM
            || id == ::AV_CODEC_ID_PPM
        ) {
            ::AVPacket* p = ::av_packet_alloc();
            if ( ::av_read_frame( Playing.Format, p ) == 0 )
                if ( p->size > 0 && p->stream_index == i ) {
                    ::delete[] Playing.Cover;
                    Playing.Cover = ::ArchiveImage( p->data, p->size, ::MIDPOINT );
                }
            ::av_packet_free( &p );
        }

        if ( id == ::AV_CODEC_ID_FLAC
            || id == ::AV_CODEC_ID_MP3
            || id == ::AV_CODEC_ID_AAC
            || id == ::AV_CODEC_ID_AC3
            || id == ::AV_CODEC_ID_EAC3
            || id == ::AV_CODEC_ID_VORBIS
            || id == ::AV_CODEC_ID_OPUS
            || id == ::AV_CODEC_ID_WAVPACK
            || id == ::AV_CODEC_ID_MLP
            || id == ::AV_CODEC_ID_ALAC
            || id == ::AV_CODEC_ID_DTS
            || id == ::AV_CODEC_ID_TRUEHD
            || id == ::AV_CODEC_ID_SPEEX
            || id == ::AV_CODEC_ID_CELT
            || id == ::AV_CODEC_ID_SIREN
            || id == ::AV_CODEC_ID_TTA
            || id == ::AV_CODEC_ID_PCM_S16LE
            || id == ::AV_CODEC_ID_PCM_S16BE
            || id == ::AV_CODEC_ID_PCM_U16LE
            || id == ::AV_CODEC_ID_PCM_U16BE
            || id == ::AV_CODEC_ID_PCM_S8
            || id == ::AV_CODEC_ID_PCM_U8
            || id == ::AV_CODEC_ID_PCM_F32LE
            || id == ::AV_CODEC_ID_PCM_F32BE
            || id == ::AV_CODEC_ID_PCM_F64LE
            || id == ::AV_CODEC_ID_PCM_F64BE
            || id == ::AV_CODEC_ID_GSM
            || id == ::AV_CODEC_ID_PCM_ALAW
            || id == ::AV_CODEC_ID_PCM_MULAW
            || id == ::AV_CODEC_ID_ADPCM_4XM
            || id == ::AV_CODEC_ID_ADPCM_IMA_QT
            || id == ::AV_CODEC_ID_ADPCM_MS
            || id == ::AV_CODEC_ID_ADPCM_SWF
            || id == ::AV_CODEC_ID_ADPCM_XA
            || id == ::AV_CODEC_ID_ADPCM_YAMAHA
            || id == ::AV_CODEC_ID_SIPR
            || id == ::AV_CODEC_ID_RA_144
            || id == ::AV_CODEC_ID_SONIC
            || id == ::AV_CODEC_ID_SONIC_LS
            || id == ::AV_CODEC_ID_PCM_MULAW
            || id == ::AV_CODEC_ID_QCELP
            || id == ::AV_CODEC_ID_WMAV1
            || id == ::AV_CODEC_ID_WMAV2
            || id == ::AV_CODEC_ID_WMAVOICE
            || id == ::AV_CODEC_ID_COOK
        ) {
            if ( stream->time_base.den == 0 )
                continue;

            Playing.Duration = ( ::uint16_t )( stream->duration / ( double )stream->time_base.den );
            Playing.Timebase = ::av_q2d( stream->time_base );
            Playing.Stream = i;

            const ::AVCodec* codec = ::avcodec_find_decoder( stream->codecpar->codec_id );
            if ( !codec ) return E_FAIL;
            if ( !( Playing.Codec = ::avcodec_alloc_context3( codec ) ) ) return E_FAIL;
            HR( ::avcodec_parameters_to_context( Playing.Codec, stream->codecpar ) );
            Playing.Codec->request_sample_fmt = ::AV_SAMPLE_FMT_FLT;
            HR( ::avcodec_open2( Playing.Codec, codec, nullptr ) );

            ::AVChannelLayout layout;
            ::av_channel_layout_default( &layout, ::Device.playback.channels );

            HR( ::swr_alloc_set_opts2(
                &Playing.SWR,
                &layout,
                ::AV_SAMPLE_FMT_FLT,
                ::Device.sampleRate,
                &Playing.Codec->ch_layout,
                Playing.Codec->sample_fmt,
                Playing.Codec->sample_rate,
                0,
                nullptr
            ) );

            if ( !Playing.SWR ) return E_FAIL;
            HR( ::swr_init( Playing.SWR ) );
            if ( !( Playing.Frame = ::av_frame_alloc() ) ) return E_FAIL;
            if ( !( Playing.Packet = ::av_packet_alloc() ) ) return E_FAIL;
        }
    }

    if ( !Playing.Codec ) return E_FAIL;

    Playing.Samplerate = Playing.Codec->sample_rate;
    Playing.Bitrate = Playing.Format->bit_rate / 1000;
    Playing.Size = ::std::filesystem::file_size( path ) / 1000000;

    return S_OK;
}