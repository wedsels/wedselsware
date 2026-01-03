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

struct Play {
    ::AVFormatContext* Format;
    ::AVCodecContext* Codec;
    ::SwrContext* SWR;
    ::AVPacket* Packet;
    ::AVFrame* Frame;
    ::std::wstring Encoding;
    ::uint16_t Duration;
    ::uint32_t Cover[ ::MIDPOINT * ::MIDPOINT ];
    double Timebase;
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
extern void UpdateDirectories( ::MSG& msg );

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
    inline ::std::vector< ::uint32_t > Queue;
    inline ::std::unordered_map< ::uint32_t, double > Volumes;
    inline ::std::unordered_map< ::uint32_t, double > Mixers;
    inline ::std::unordered_map< ::uint32_t, ::media > Songs;
};

inline ::std::vector< ::uint32_t > display {};

static const ::std::wstring SongPath = L"F:/SoundStuff/Sounds/";

inline void Sort() {
    if ( ::GridType == ::GridTypes::Queue )
        return;

    ::std::vector< ::uint32_t >& Display = ::GetDisplay();

    switch ( ::Saved::Sorting ) {
        case ::SortTypes::Time:
                ::std::sort( Display.begin(), Display.end(), []( ::uint32_t a, ::uint32_t b ) {
                    return ::Saved::Songs[ a ].Write < ::Saved::Songs[ b ].Write;
                } );
            break;
        case ::SortTypes::Artist:
                ::std::sort( Display.begin(), Display.end(), []( ::uint32_t a, ::uint32_t b ) {
                    int cmp;
                    
                    cmp = ::wcscmp( ::Saved::Songs[ a ].Artist, ::Saved::Songs[ b ].Artist );
                    if ( cmp != 0 ) return cmp < 0;

                    cmp = ::wcscmp( ::Saved::Songs[ a ].Album, ::Saved::Songs[ b ].Album );
                    if ( cmp != 0 ) return cmp < 0;

                    return ::wcscmp(::Saved::Songs[ a ].Title, ::Saved::Songs[ b ].Title ) < 0;
                } );
            break;
        case ::SortTypes::Title:
                ::std::sort( Display.begin(), Display.end(), []( ::uint32_t a, ::uint32_t b ) {
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

    ::DisplayOffset = ::Index( Display, ::Saved::Playing );
}

inline double cursor;
inline ::ma_device Device;

inline ::std::atomic< bool > PauseAudio = true;

inline void Remove( ::uint32_t id ) {
    if ( ::Saved::Songs.contains( id ) )
        ::Saved::Songs.erase( id );

    int index;

    index = ::Index( ::display, id );
    if ( index > -1 )
        ::display.erase( ::display.begin() + index );

    index = ::Index( ::Saved::Queue, id );
    if ( index > -1 )
        ::Saved::Queue.erase( ::Saved::Queue.begin() + index );

    index = ::Index( ::Search, id );
    if ( index > -1 )
        ::Saved::Queue.erase( ::Saved::Queue.begin() + index );
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
extern void SetVolume( double v = 0.0 );
extern void ArchiveSong( ::std::wstring path );
extern void SetSong( ::uint32_t song );
extern ::HRESULT FFMPEG( const wchar_t* path, ::Play& play );
extern ::HRESULT SetDefaultDevice();

namespace queue {
    static inline void clear() {
        ::Saved::Queue.clear();
        ::Saved::Queue.push_back( ::Saved::Playing );
    }

    static inline bool begin() { return ::Saved::Queue.empty() || ::Saved::Queue.front() == ::Saved::Playing; }
    static inline bool end() { return ::Saved::Queue.empty() || ::Saved::Queue.back() == ::Saved::Playing; }
    static inline bool road( int direction ) { return direction > 0 && end() || direction < 0 && begin(); }

    static inline ::uint32_t next( ::std::vector< ::uint32_t >& line, int direction, ::uint32_t fallback = ::Saved::Playing ) {
        int target = direction + ::Index( line, ::Saved::Playing );
        if ( target < 0 || target >= line.size() )
            return fallback;
        return line[ target ];
    }

    static inline void add( ::uint32_t song, int direction ) {
        int in = ::Index( ::Saved::Queue, song );

        if ( in > -1 )
            ::Saved::Queue.erase( ::Saved::Queue.begin() + in );

        if ( direction > 0 )
            ::Saved::Queue.push_back( song );
        else
            ::Saved::Queue.emplace( ::Saved::Queue.begin(), song );
    }

    static inline void set( ::uint32_t song, int direction ) {
        if ( direction > 0 && end() || direction < 0 && begin() )
            add( song, direction );

        ::SetSong( song );
    }

    static inline ::std::unordered_map< ::Playback, ::std::function< void( int ) > > playbacks = {
        { ::Playback::Linear, []( int direction ) { { set( next( ::Saved::Queue, direction, next( ::display, direction ) ), direction ); } } },
        { ::Playback::Repeat, []( int direction ) { { set( ::Saved::Playing, 0 ); } } },
        { ::Playback::Shuffle, []( int direction ) { { set( next( ::Saved::Queue, direction, ::display[ ::RNG() % ::display.size() ] ), direction ); } } },
        { ::Playback::Queue, []( int direction ) { { set( next( ::Saved::Queue, direction, direction > 0 ? ::Saved::Queue.front() : ::Saved::Queue.back() ), 0 ); } } },
    };

    static inline void next( int direction ) { playbacks[ ::Saved::Playback ]( direction ); }
}