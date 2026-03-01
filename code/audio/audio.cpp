#include "audio.hpp"

#include <fileref.h>
#include <regex>

::std::wstring NormalizeString( const char str[] ) {
    constexpr int MINIPATH = MAX_PATH / 3;

    static const ::std::string invalid = "\\/:*?\"<>|.";

    ::std::string ret;

    for ( ::size_t i = 0; str[ i ] != '\0'; ++i )
        if ( i >= MINIPATH || str[ i ] == ';' || i > 0 && str[ i ] == '(' && str[ i - 1 ] == ' ' )
            break;
        else if ( invalid.find( str[ i ] ) == ::std::string::npos )
            ret += ::std::toupper( str[ i ] );

    return ::String::Utf8Wide( ::std::regex_replace( ret, ::std::regex( "^ +| +$|( ) +" ), "$1" ) );
}

::HRESULT FFMPEG( const wchar_t* path, ::Play& play, ::uint32_t** cover = nullptr ) {
    if ( 0 > ::avformat_open_input( &play.Format, ::String::WideUtf8( path ).c_str(), 0, 0 ) ) return E_FAIL;
    if ( !play.Format ) return E_FAIL;
    if ( 0 > ::avformat_find_stream_info( play.Format, 0 ) ) return E_FAIL;

    if ( play.Format->iformat )
        play.Encoding = ::NormalizeString( play.Format->iformat->name );

    play.Duration = play.Format->duration / ( double )AV_TIME_BASE;
    play.Bitrate = play.Format->bit_rate / 1e3;

    for ( unsigned int i = 0; i < play.Format->nb_streams; i++ ) {
        ::AVStream* stream = play.Format->streams[ i ];

        if ( !stream || !stream->codecpar )
            continue;

        if ( stream->codecpar->codec_type == ::AVMEDIA_TYPE_VIDEO ) {
            if ( stream->disposition & AV_DISPOSITION_ATTACHED_PIC ) {
                play.CoverStream = i;

                if ( cover ) {
                    const ::AVPacket& p = stream->attached_pic;

                    if ( p.data && p.size > 0 )
                        *cover = ::ArchiveImage( p.data, p.size, MINICOVER );
                }

                if ( play.AudioStream > -1 )
                    break;
            }
        } else if ( stream->codecpar->codec_type == ::AVMEDIA_TYPE_AUDIO ) {
            if ( stream->time_base.den == 0 )
                continue;

            play.Samplerate = stream->codecpar->sample_rate;
            play.Timebase = stream->time_base;
            play.AudioStream = i;

            if ( play.Duration <= 0 )
                play.Duration = stream->duration * ::av_q2d( play.Timebase );

            if ( play.CoverStream > -1 )
                break;
        }
    }

    return S_OK;
}

::HRESULT CollectFFMPEG( ::Play& play ) {
    const ::AVStream* astream = play.Format->streams[ play.AudioStream ];

    const ::AVCodec* codec = ::avcodec_find_decoder( astream->codecpar->codec_id );
    if ( !codec ) return E_FAIL;
    if ( !( play.Codec = ::avcodec_alloc_context3( codec ) ) ) return E_FAIL;
    if ( 0 > ::avcodec_parameters_to_context( play.Codec, astream->codecpar ) ) return E_FAIL;
    play.Codec->request_sample_fmt = ::AV_SAMPLE_FMT_FLT;
    if ( 0 > ::avcodec_open2( play.Codec, codec, nullptr ) ) return E_FAIL;

    ::AVChannelLayout layout;
    ::av_channel_layout_default( &layout, ::Device.playback.channels );

    if ( 0 > ::swr_alloc_set_opts2(
        &play.SWR,
        &layout,
        ::AV_SAMPLE_FMT_FLT,
        ::Device.sampleRate,
        &play.Codec->ch_layout,
        play.Codec->sample_fmt,
        play.Codec->sample_rate,
        0,
        nullptr
    ) ) return E_FAIL;

    if ( !play.SWR ) return E_FAIL;
    if ( 0 > ::swr_init( play.SWR ) ) return E_FAIL;
    if ( !( play.Frame = ::av_frame_alloc() ) ) return E_FAIL;
    if ( !( play.Packet = ::av_packet_alloc() ) ) return E_FAIL;

    if ( !play.Codec ) return E_FAIL;

    {
        ::std::unique_lock lock( ::CanvasMutex );

        if ( ::PlayingCover ) {
            ::delete[] ::PlayingCover;
            ::PlayingCover = nullptr;
        }

        if ( play.CoverStream > -1 ) {
            const ::AVStream* cstream = play.Format->streams[ play.CoverStream ];
            if ( cstream ) {
                const ::AVPacket& p = cstream->attached_pic;

                if ( p.data && p.size > 0 )
                    ::PlayingCover = ::ArchiveImage( p.data, p.size, MIDPOINT );
            }
        }
    }

    return S_OK;
}

void WriteCovers( ::media& media ) {
    ::std::wstring dir = ::String::WConcat( ::SongPath, media.Artist, L"/", media.Album, L"/" );
    ::Path( dir );
    if ( !::std::filesystem::exists( dir ) )
        return;

    THREAD(
        ::std::string file = ::ClipboardText();
        file = file.substr( 1, file.size() - 2 );
        if ( !::std::filesystem::exists( file ) )
            return;

        for ( const ::std::filesystem::path& entry : ::std::filesystem::directory_iterator( dir, ::std::filesystem::directory_options::skip_permission_denied ) ) {
            ::std::wstring p = entry.wstring();
            ::Path( p );

            ::uint32_t hash = ::String::Hash( p );

            if ( !::Library.contains( hash ) )
                continue;

            ::TagLib::FileRef f( entry.wstring().c_str(), false, ::TagLib::AudioProperties::Fast );
            
            ::std::ifstream image( file, ::std::ios::binary );
            if ( !image )
                continue;

            ::std::vector< char > data( ( ::std::istreambuf_iterator< char >( image ) ), ::std::istreambuf_iterator< char >() );
            if ( data.size() <= 0 )
                continue;

            f.setComplexProperties( "PICTURE", { { { "data", ::TagLib::ByteVector( data.data(), data.size() ) }, { "pictureType", "Front Cover" }, { "mimeType", "image/jpeg" } } } );
            f.save();

            if ( ::Library[ hash ].Minicover ) {
                ::delete[] ::Library[ hash ].Minicover;
                ::Library[ hash ].Minicover = nullptr;
            }

            ::Library[ hash ].Minicover = ::ArchiveImage( ( ::uint8_t* )data.data(), data.size(), MINICOVER );
        }

        ::UI::Redraw();

        ::std::filesystem::remove( file );
        , dir
    );
}

void SetSong( ::uint32_t song ) {
    if ( !::Library.contains( song ) )
        return::Remove( song );

    auto& s = ::Library[ song ];

    {
        ::std::unique_lock lock( ::SeekMutex );

        ::Clean( ::Playing );

        if ( FAILED( ::FFMPEG( s.Path.c_str(), ::Playing ) ) || FAILED( ::CollectFFMPEG( ::Playing ) ) ) {
            ::Clean( ::Playing );
            ::Remove( song );
            return;
        }
    }

    ::Saved::Playing = s.ID;

    ::Seek( 0 );
}

void ArchiveSong( ::std::wstring& p ) { 
    ::Path( p );

    THREAD(
        ::uint32_t pid = ::String::Hash( p );

        {
            ::std::unique_lock lock( ::CanvasMutex );

            if ( ::Library.contains( pid ) && ::Library[ pid ].ID )
                return;
        }

        ::Play play = {};
        ::media media = {};

        if ( FAILED( ::FFMPEG( p.c_str(), play, &media.Minicover ) ) )
            return::Clean( play );

        media.Artist = L'_';
        media.Title = L'_';
        media.Album = L'_';

        ::AVDictionary* metadata = play.Format->metadata;
        ::AVDictionaryEntry* tag = nullptr;
        while ( ( tag = ::av_dict_get( metadata, "", tag, AV_DICT_IGNORE_SUFFIX ) ) )
            if ( media.Artist != L"_" && media.Title != L"_" && media.Album != L"_" )
                break;
            else if ( ::_stricmp( tag->key, "ARTIST" ) == 0 )
                media.Artist = ::NormalizeString( tag->value );
            else if ( ::_stricmp( tag->key, "TITLE" ) == 0 )
                media.Title = ::NormalizeString( tag->value );
            else if ( ::_stricmp( tag->key, "ALBUM" ) == 0 )
                media.Album = ::NormalizeString( tag->value );

        media.Encoding = play.Encoding;

        media.Samplerate = play.Samplerate;
        media.Duration = play.Duration;
        media.Bitrate = play.Bitrate;

        ::Clean( play );

        ::std::wstring dir = ::String::WConcat( ::SongPath, media.Artist, L"/", media.Album, L"/" );
        ::Path( dir );
        ::std::filesystem::create_directories( dir );

        media.Path = ::String::WConcat( dir, media.Title, L'.', media.Encoding );
        ::Path( media.Path );

        int count = 0;
        while ( p != media.Path && ::std::filesystem::exists( media.Path ) )
            ::Path( media.Path = ::String::WConcat( dir, media.Title, L" - ", count++, L'.', media.Encoding ) );

        if ( p != media.Path )
            ::std::filesystem::rename( p, media.Path );

        media.ID = ::String::Hash( media.Path );

        if ( ::Library.contains( media.ID ) && ::Library[ media.ID ].ID )
            return;

        media.Size = ::std::filesystem::file_size( media.Path ) / 1e6;
        media.Write = ::std::filesystem::last_write_time( media.Path ).time_since_epoch().count();

        if ( ::Saved::Volumes.find( media.ID ) == ::Saved::Volumes.end() )
            ::Saved::Volumes[ media.ID ] = 0.15;

        {
            ::std::unique_lock lock( ::CanvasMutex );

            ::Library[ media.ID ] = media;
            ::SongDisplay.push_back( media.ID );
        }

        if ( media.ID == ::Saved::Playing )
            ::SetSong( media.ID );

        ::Sort();
        , p
    );
}

void Sort() {
    ::SortRequest = true;
    ::SortRequest.v.notify_one();
}

::HRESULT InitAudio() {
    THREAD(
        while( true ) {
            ::SortRequest.v.wait( false );
            ::SortRequest = false;

            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 50 ) );

            if ( ::SortRequest )
                continue;

            ::std::unique_lock lock( ::CanvasMutex );

            switch ( ::Saved::Sorting ) {
                case ::SortTypes::Time:
                        ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                            return ::Library[ a ].Write < ::Library[ b ].Write;
                        } );
                    break;
                case ::SortTypes::Artist:
                        ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                            const ::media& l = ::Library[ a ];
                            const ::media& r = ::Library[ b ];

                            if ( l.Artist != r.Artist )
                                return l.Artist < r.Artist;

                            if ( l.Album != r.Album )
                                return l.Album < r.Album;

                            return l.Title < r.Title;
                        } );
                    break;
                case ::SortTypes::Title:
                        ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                            const ::media& l = ::Library[ a ];
                            const ::media& r = ::Library[ b ];

                            if ( l.Title != r.Title )
                                return l.Title < r.Title;

                            if ( l.Artist != r.Artist )
                                return l.Artist < r.Artist;

                            return l.Album < r.Album;
                        } );
                    break;
                default: break;
            }
        }
    );

    return S_OK;
}