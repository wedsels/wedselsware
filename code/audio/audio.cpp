#include "audio.hpp"

#include <regex>

void SetSong( ::uint32_t song ) {
    ::std::lock_guard< ::std::mutex > lock( ::PlayerMutex );

    if ( !::songs.contains( song ) )
        return;

    ::Clean( ::Playing );

    auto& s = ::songs[ song ];
    ::Playing.File = ::CreateFileW( s.path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );

    if ( FAILED( ::FFMPEG( ::Playing ) ) ) {
        ::std::wcout<<s.path<<"\n";
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

::std::wstring NormalizeString( char str[] ) {
    static const ::std::string invalid = "\\/:*?\"<>|.";

    ::std::string ret;

    for ( ::size_t i = 0; str[ i ] != '\0'; ++i )
        if ( str[ i ] == ';' || i >= 86 )
            break;
        else if ( invalid.find( str[ i ] ) == ::std::string::npos )
            ret += ::std::toupper( str[ i ] );

    ret = ::std::regex_replace( ret, ::std::regex( "^ +| +$|( ) +" ), "$1" );

    return ::String::Utf8Wide( ret );
}

void ArchiveSong( ::std::wstring p ) {
    for ( auto& i : p )
        if ( i == L'\\' )
            i = L'/';

    if ( ::songs.contains( ::String::Hash( p ) ) )
        return;

    ::media media = {};
    ::Play play = {};

    ::std::filesystem::path path( p );

    play.File = ::CreateFileW( path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( !play.File )
        return::Clean( play );

    ::FILETIME creationTime;
    ::GetFileTime( play.File, &creationTime, nullptr, nullptr );
    ::ULARGE_INTEGER ul;
    ul.LowPart = creationTime.dwLowDateTime;
    ul.HighPart = creationTime.dwHighDateTime;
    media.write = ul.QuadPart / 10000;

    if ( FAILED( ::FFMPEG( play ) ) )
        return::Clean( play );

    media.artist = L"_";
    media.title = L"_";
    media.album = L"_";

    ::AVDictionary* metadata = play.Format->metadata;
    ::AVDictionaryEntry* tag = nullptr;
    while ( ( tag = ::av_dict_get( metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
        if ( ::_stricmp( tag->key, "ARTIST" ) == 0 )
            media.artist = ::NormalizeString( tag->value );
        else if ( ::_stricmp( tag->key, "TITLE" ) == 0 )
            media.title = ::NormalizeString( tag->value );
        else if ( ::_stricmp( tag->key, "ALBUM" ) == 0 )
            media.album = ::NormalizeString( tag->value );

    media.encoding = path.extension().wstring();

    ::std::wstring dir = ::String::WConcat( SongPath, media.artist, L"/", media.album, L"/" );

    if ( !::std::filesystem::is_directory( dir ) )
        ::std::filesystem::create_directories( dir );

    ::std::wstring name = ::String::WConcat( media.title, media.encoding );
    ::std::wstring expected = ::String::WConcat( dir, name );

    if ( path.wstring() != expected && ::std::filesystem::exists( ::String::WConcat( dir, name ) ) )
        expected = ::String::WConcat( dir, media.title, L" - ", media.write, media.encoding );

    for ( auto& i : expected )
        if ( i == L'\\' )
            i = L'/';

    if ( path.wstring() != expected )
        ::std::filesystem::rename( path.wstring(), expected );

    media.id = ::String::Hash( expected );

    media.Duration = play.Duration;
    media.Size = ::std::filesystem::file_size( expected ) / 1000000;
    media.Samplerate = play.Samplerate;
    media.Bitrate = play.Bitrate;
    media.path = expected;

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

static int rpacket( void* opaque, ::uint8_t* buf, int buf_size ) {
    ::DWORD bytes = 0;

    if ( !::ReadFile( ( ::HANDLE )opaque, buf, buf_size, &bytes, NULL ) || bytes <= 0 )
        return AVERROR_EOF;

    return bytes;
}

::HRESULT FFMPEG( ::Play& play ) {
    if ( !play.File )
        return E_FAIL;

    static const int bsize = 4096;
    unsigned char* buffer = ( unsigned char* )::av_malloc( bsize );
    if ( !buffer )
        return E_FAIL;

    ::AVIOContext* avioCtx = ::avio_alloc_context( buffer, bsize, 0, play.File, &::rpacket, nullptr, nullptr );
    if ( !avioCtx )
        return E_FAIL;

    if ( !( play.Format = ::avformat_alloc_context() ) )
        return E_FAIL;

    play.Format->pb = avioCtx;
    play.Format->flags |= AVFMT_FLAG_CUSTOM_IO;

    HR( ::avformat_open_input( &play.Format, nullptr, nullptr, nullptr ) );

    if ( !play.Format )
        return E_FAIL;

    HR( ::avformat_find_stream_info( play.Format, 0 ) );

    for ( unsigned int i = 0; i < play.Format->nb_streams; i++ ) {
        ::AVStream* stream = play.Format->streams[ i ];

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
            if ( ::av_read_frame( play.Format, p ) == 0 )
                if ( p->size > 0 && p->stream_index == i ) {
                    ::delete[] play.Cover;
                    play.Cover = ::ArchiveImage( p->data, p->size, ::MIDPOINT );
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

            play.Duration = ( ::uint16_t )( stream->duration / ( double )stream->time_base.den );
            play.Timebase = ::av_q2d( stream->time_base );
            play.Stream = i;

            const ::AVCodec* codec = ::avcodec_find_decoder( stream->codecpar->codec_id );
            if ( !codec ) return E_FAIL;
            if ( !( play.Codec = ::avcodec_alloc_context3( codec ) ) ) return E_FAIL;
            HR( ::avcodec_parameters_to_context( play.Codec, stream->codecpar ) );
            play.Codec->request_sample_fmt = ::AV_SAMPLE_FMT_FLT;
            HR( ::avcodec_open2( play.Codec, codec, nullptr ) );

            ::AVChannelLayout layout;
            ::av_channel_layout_default( &layout, ::Device.playback.channels );

            HR( ::swr_alloc_set_opts2(
                &play.SWR,
                &layout,
                ::AV_SAMPLE_FMT_FLT,
                ::Device.sampleRate,
                &play.Codec->ch_layout,
                play.Codec->sample_fmt,
                play.Codec->sample_rate,
                0,
                nullptr
            ) );

            if ( !play.SWR ) return E_FAIL;
            HR( ::swr_init( play.SWR ) );
            if ( !( play.Frame = ::av_frame_alloc() ) ) return E_FAIL;
            if ( !( play.Packet = ::av_packet_alloc() ) ) return E_FAIL;
        }
    }

    if ( !play.Codec ) return E_FAIL;

    play.Samplerate = play.Codec->sample_rate;
    play.Bitrate = play.Format->bit_rate / 1000;

    return S_OK;
}