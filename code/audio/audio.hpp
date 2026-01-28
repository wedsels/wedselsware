#include "../common.hpp"
#include "../ui/ui.hpp"

#include <miniaudio.h>
#include <mmdeviceapi.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

inline constexpr int MINIPATH = MAX_PATH / 3;

inline ::std::mutex SeekMutex;

inline ::relaxed_atomic< ::uint16_t > Wave[ WINWIDTH / 2 ];

struct Play {
    ::AVFormatContext* Format;
    ::AVCodecContext* Codec;
    ::SwrContext* SWR;
    ::AVPacket* Packet;
    ::AVFrame* Frame;
    ::std::wstring Encoding;
    ::uint16_t Duration;
    ::uint32_t Cover[ MIDPOINT * MIDPOINT ];
    ::AVRational Timebase;
    int Samplerate;
    int Bitrate;
    int Stream = -1;
};
inline Play Playing;

struct media {
    wchar_t Encoding[ 8 ];
    wchar_t Artist[ ::MINIPATH ];
    wchar_t Album[ ::MINIPATH ];
    wchar_t Title[ ::MINIPATH ];
    wchar_t Path[ MAX_PATH ];
    ::uint64_t Write;
    ::uint32_t ID;
    ::uint16_t Duration;
    ::uint32_t Minicover[ MINICOVER * MINICOVER ];
    ::size_t Size;
    int Samplerate;
    int Bitrate;

    bool operator==( const ::media& m ) const { return ID == m.ID; }
};

enum struct Playback { Linear, Repeat, Shuffle, Queue, Count };
enum struct SortTypes { Time, Artist, Title, Count };

struct Launch {
    wchar_t path[ MAX_PATH ];
    ::uint32_t img[ MINICOVER * MINICOVER ];
};

extern void DeleteLink( ::uint32_t id, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map );
extern void ArchiveLink( ::std::wstring path, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map );
extern void UpdateDirectories();

struct Volume {
    wchar_t name[ ::MINIPATH ];
    ::uint32_t minicover[ MINICOVER * MINICOVER ];
};
inline ::std::unordered_map< ::uint32_t, ::Volume > MixerEntries;
inline ::std::vector< ::uint32_t > MixersActive;

namespace Saved {
    inline ::std::vector< ::uint32_t > Apps;
    inline ::std::unordered_map< ::uint32_t, ::Launch > AppsPath;
    inline ::std::vector< ::uint32_t > Webs;
    inline ::std::unordered_map< ::uint32_t, ::Launch > WebsPath;

    inline ::uint32_t Playing;
    inline ::SortTypes Sorting;
    inline ::Playback Playback;
    inline ::uint8_t Queue;
    inline ::std::vector< ::std::vector< ::uint32_t > > Queues;
    inline ::std::unordered_map< ::uint32_t, double > Volumes;
    inline ::std::unordered_map< ::uint32_t, double > Mixers;
    inline ::std::unordered_map< ::uint32_t, ::media > Songs;
};

inline ::std::vector< ::uint32_t >& Queue() { return ::Saved::Queues[ ::Saved::Queue ]; }

inline ::std::vector< ::uint32_t > SongDisplay {};

static const ::std::wstring SongPath = L"F:/SoundStuff/Sounds/";

inline void SongInfo( ::uint32_t song ) {
    ::DisplayText.clear();

    if ( !::Saved::Songs.contains( song ) )
        return;

    ::DisplayText.push_back( ::Saved::Songs[ song ].Title );
    ::DisplayText.push_back( ::Saved::Songs[ song ].Artist );
    ::DisplayText.push_back( ::Saved::Songs[ song ].Album );
    ::DisplayText.push_back( ::Saved::Songs[ song ].Encoding );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Songs[ song ].Duration, L"s" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Songs[ song ].Size, L"mb" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Songs[ song ].Bitrate, L"kbps" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Songs[ song ].Samplerate, L"Hz" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Volumes[ song ], L"%" ) );
}

inline void Sort() {
    switch ( ::Saved::Sorting ) {
        case ::SortTypes::Time:
                ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                    return ::Saved::Songs[ a ].Write < ::Saved::Songs[ b ].Write;
                } );
            break;
        case ::SortTypes::Artist:
                ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                    int cmp;
                    
                    cmp = ::wcscmp( ::Saved::Songs[ a ].Artist, ::Saved::Songs[ b ].Artist );
                    if ( cmp != 0 ) return cmp < 0;

                    cmp = ::wcscmp( ::Saved::Songs[ a ].Album, ::Saved::Songs[ b ].Album );
                    if ( cmp != 0 ) return cmp < 0;

                    return ::wcscmp(::Saved::Songs[ a ].Title, ::Saved::Songs[ b ].Title ) < 0;
                } );
            break;
        case ::SortTypes::Title:
                ::std::sort( ::SongDisplay.begin(), ::SongDisplay.end(), []( ::uint32_t a, ::uint32_t b ) {
                    int cmp;
                    
                    cmp = ::wcscmp( ::Saved::Songs[ a ].Title, ::Saved::Songs[ b ].Title );
                    if ( cmp != 0 ) return cmp < 0;

                    cmp = ::wcscmp( ::Saved::Songs[ a ].Artist, ::Saved::Songs[ b ].Artist );
                    if ( cmp != 0 ) return cmp < 0;

                    return ::wcscmp( ::Saved::Songs[ a ].Album, ::Saved::Songs[ b ].Album ) < 0;
                } );
            break;
        default: break;
    }
}

inline double cursor;
inline ::ma_device Device;

inline ::relaxed_atomic< bool > PauseAudio = true;

inline void Remove( ::uint32_t id ) {
    ::std::lock_guard< ::std::mutex > lock( ::CanvasMutex );

    if ( ::Saved::Songs.contains( id ) )
        ::Saved::Songs.erase( id );

    int index;

    index = ::Index( ::SongDisplay, id );
    if ( index > -1 )
        ::SongDisplay.erase( ::SongDisplay.begin() + index );

    index = ::Index( ::Queue(), id );
    if ( index > -1 )
        ::Queue().erase( ::Queue().begin() + index );
}

inline void Clean( ::Play& Play ) {
    ::std::memset( Play.Cover, 0, sizeof( Play.Cover ) );
    if ( Play.Format ) ::avformat_close_input( &Play.Format );
    if ( Play.Codec ) ::avcodec_free_context( &Play.Codec );
    if ( Play.Packet ) ::av_packet_free( &Play.Packet );
    if ( Play.Frame ) ::av_frame_free( &Play.Frame );
    if ( Play.SWR ) ::swr_free( &Play.SWR );
}

extern void DefaultDisplay( ::uint32_t entry );

extern void SetMixers();
extern void SetMixerVolume( ::uint32_t entry, double change );

extern void Seek( int time );
extern void Decode( ::ma_device* device, ::uint8_t* output, ::ma_uint32 framecount );
extern void ArchiveSong( ::std::wstring path );
extern void SetSong( ::uint32_t song );
extern ::HRESULT FFMPEG( const wchar_t* path, ::Play& play );
extern ::HRESULT SetDefaultDevice();

namespace queue {
    static inline void clear() {
        ::Queue().clear();
        ::Queue().push_back( ::Saved::Playing );
    }

    static inline bool begin() { return ::Queue().empty() || ::Queue().front() == ::Saved::Playing; }
    static inline bool end() { return ::Queue().empty() || ::Queue().back() == ::Saved::Playing; }
    static inline bool road( int direction ) { return direction > 0 && end() || direction < 0 && begin(); }

    static inline ::uint32_t next( ::std::vector< ::uint32_t >& line, int direction, ::uint32_t fallback = ::Saved::Playing ) {
        int target = direction + ::Index( line, ::Saved::Playing );
        if ( target < 0 || target >= line.size() )
            return fallback;
        return line[ target ];
    }

    static inline void add( ::uint32_t song, int direction ) {
        int in = ::Index( ::Queue(), song );

        if ( in > -1 )
            ::Queue().erase( ::Queue().begin() + in );

        if ( direction > 0 )
            ::Queue().push_back( song );
        else
            ::Queue().emplace( ::Queue().begin(), song );
    }

    static inline void set( ::uint32_t song, int direction ) {
        if ( direction > 0 && end() || direction < 0 && begin() )
            add( song, direction );

        ::SetSong( song );
    }

    static inline ::std::unordered_map< ::Playback, ::std::function< void( int ) > > playbacks = {
        { ::Playback::Linear, []( int direction ) { { set( next( ::Queue(), direction, next( ::SongDisplay, direction ) ), direction ); } } },
        { ::Playback::Repeat, []( int direction ) { { ::Seek( -INT_MAX ); } } },
        { ::Playback::Shuffle, []( int direction ) { { set( next( ::Queue(), direction, ::SongDisplay[ ::RNG() % ::SongDisplay.size() ] ), direction ); } } },
        { ::Playback::Queue, []( int direction ) { { set( next( ::Queue(), direction, direction > 0 ? ::Queue().front() : ::Queue().back() ), 0 ); } } },
    };

    static inline void next( int direction ) { playbacks[ ::Saved::Playback ]( direction ); }
}