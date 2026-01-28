#include "audio.hpp"

#include <regex>

void SetSong( ::uint32_t song ) {
    ::std::lock_guard< ::std::mutex > lock( ::SeekMutex );

    if ( !::Saved::Songs.contains( song ) )
        return::Remove( song );

    ::Clean( ::Playing );

    auto& s = ::Saved::Songs[ song ];

    if ( FAILED( ::FFMPEG( s.Path, ::Playing ) ) ) {
        ::Clean( ::Playing );
        ::Remove( song );
        return;
    }

    ::Saved::Playing = s.ID;

    ::cursor = 0;
}

::std::wstring NormalizeString( const char str[] ) {
    static const ::std::string invalid = "\\/:*?\"<>|.";

    ::std::string ret;

    for ( ::size_t i = 0; str[ i ] != '\0'; ++i )
        if ( str[ i ] == ';' || i >= 86 )
            break;
        else if ( invalid.find( str[ i ] ) == ::std::string::npos )
            ret += ::std::toupper( str[ i ] );

    return ::String::Utf8Wide( ::std::regex_replace( ret, ::std::regex( "^ +| +$|( ) +" ), "$1" ) );
}

void ArchiveSong( ::std::wstring p ) {
    static ::Play SecondPlay;

    ::Path( p );

    ::uint32_t pid = ::String::Hash( p );
    if ( ::Saved::Songs.contains( pid ) && ::Saved::Songs[ pid ].ID ) {
        if ( !::std::filesystem::exists( p ) )
            ::Remove( pid );
        return;
    }

    ::media media = {};

    ::HANDLE file = ::CreateFileW( p.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
    if ( file == INVALID_HANDLE_VALUE )
        return::Clean( SecondPlay );

    ::FILETIME time;
    ::GetFileTime( file, &time, nullptr, nullptr );
    ::CloseHandle( file );

    ::ULARGE_INTEGER ul;
    ul.LowPart = time.dwLowDateTime;
    ul.HighPart = time.dwHighDateTime;
    media.Write = ul.QuadPart / 10000;

    if ( FAILED( ::FFMPEG( p.c_str(), SecondPlay ) ) )
        return::Clean( SecondPlay );

    media.Artist[ 0 ] = L'_';
    media.Title[ 0 ] = L'_';
    media.Album[ 0 ] = L'_';

    ::AVDictionary* metadata = SecondPlay.Format->metadata;
    ::AVDictionaryEntry* tag = nullptr;
    while ( ( tag = ::av_dict_get( metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
        if ( ::_stricmp( tag->key, "ARTIST" ) == 0 )
            ::wcsncpy_s( media.Artist, _countof( media.Artist ), ::NormalizeString( tag->value ).c_str(), _TRUNCATE );
        else if ( ::_stricmp( tag->key, "TITLE" ) == 0 )
            ::wcsncpy_s( media.Title, _countof( media.Title ), ::NormalizeString( tag->value ).c_str(), _TRUNCATE );
        else if ( ::_stricmp( tag->key, "ALBUM" ) == 0 )
            ::wcsncpy_s( media.Album, _countof( media.Album ), ::NormalizeString( tag->value ).c_str(), _TRUNCATE );

    ::wcsncpy_s( media.Encoding, _countof( media.Encoding ), SecondPlay.Encoding.c_str(), _TRUNCATE );

    media.Samplerate = SecondPlay.Samplerate;
    media.Duration = SecondPlay.Duration;
    media.Bitrate = SecondPlay.Bitrate;

    ::uint32_t* mini = ::ResizeImage( SecondPlay.Cover, MIDPOINT, MIDPOINT, MINICOVER );
    if ( mini )
        ::std::memcpy( media.Minicover, mini, sizeof( media.Minicover ) );
    ::delete[] mini;

    ::Clean( SecondPlay );

    ::std::wstring dir = ::String::WConcat( ::SongPath, media.Artist, L"/", media.Album, L"/" );
    ::Path( dir );

    if ( !::std::filesystem::is_directory( dir ) )
        ::std::filesystem::create_directories( dir );

    ::std::wstring expected = ::String::WConcat( dir, media.Title, L'.', media.Encoding );
    ::Path( expected );

    if ( p != expected && ::std::filesystem::exists( expected ) )
        ::Path( expected = ::String::WConcat( dir, media.Title, L" - ", media.Write, L'.', media.Encoding ) );

    if ( p != expected )
        ::std::filesystem::rename( p, expected );

    media.Size = ::std::filesystem::file_size( expected ) / 1e6;
    media.ID = ::String::Hash( expected );
    ::wcsncpy_s( media.Path, MAX_PATH, expected.c_str(), MAX_PATH - 1 );

    if ( ::Saved::Volumes.find( media.ID ) == ::Saved::Volumes.end() )
        ::Saved::Volumes[ media.ID ] = 0.15;

    ::std::lock_guard< ::std::mutex > lock( ::CanvasMutex );

    ::Saved::Songs[ media.ID ] = media;
    ::SongDisplay.push_back( media.ID );

    if ( media.ID == ::Saved::Playing )
        ::SetSong( media.ID );

    ::Sort();
}

::HRESULT FFMPEG( const wchar_t* path, ::Play& play ) {
    HR( ::avformat_open_input( &play.Format, ::String::WideUtf8( path ).c_str(), 0, 0 ) );

    if ( !play.Format ) return E_FAIL;

    HR( ::avformat_find_stream_info( play.Format, 0 ) );

    if ( play.Format->iformat )
        play.Encoding = ::NormalizeString( play.Format->iformat->name );

    for ( unsigned int i = 0; i < play.Format->nb_streams; i++ ) {
        ::AVStream* stream = play.Format->streams[ i ];

        if ( !stream || !stream->codecpar )
            continue;

        ::AVMediaType mt = ::avcodec_get_type( stream->codecpar->codec_id );

        if ( mt == ::AVMEDIA_TYPE_VIDEO ) {
            ::AVPacket* p = ::av_packet_alloc();
            if ( ::av_read_frame( play.Format, p ) == 0 )
                if ( p->size > 0 && p->stream_index == i ) {
                    if ( !play.Cover[ 0 ] ) {
                        ::uint32_t* cover = ::ArchiveImage( p->data, p->size, MIDPOINT );
                        if ( cover )
                            ::std::memcpy( play.Cover, cover, sizeof( play.Cover ) );
                        ::delete[] cover;
                    }
                }
            ::av_packet_free( &p );
        } else if ( mt == ::AVMEDIA_TYPE_AUDIO ) {
            if ( stream->time_base.den == 0 )
                continue;

            play.Duration = ( ::uint16_t )( stream->duration / ( double )stream->time_base.den );
            play.Timebase = stream->time_base;
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