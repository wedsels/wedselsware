#include "../common.hpp"
#include "../ui/ui.hpp"

#include <miniaudio.h>
#include <mmdeviceapi.h>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
}

inline ::std::mutex SeekMutex;

inline ::relaxed_atomic< bool > SortRequest = false;

inline ::relaxed_atomic< ::uint16_t > Wave[ WINWIDTH / 2 ];

inline ::uint32_t* PlayingCover;

struct Play {
    ::AVFormatContext* Format;
    ::AVCodecContext* Codec;
    ::SwrContext* SWR;
    ::AVPacket* Packet;
    ::AVFrame* Frame;
    ::std::wstring Encoding;
    ::uint16_t Duration;
    ::AVRational Timebase;
    int Samplerate;
    int Bitrate;
    int CoverStream = -1;
    int AudioStream = -1;
};
inline Play Playing;

struct media {
    ::std::wstring Encoding;
    ::std::wstring Artist;
    ::std::wstring Album;
    ::std::wstring Title;
    ::std::wstring Path;
    ::uint64_t Write;
    ::uint32_t ID;
    ::uint16_t Duration;
    ::uint32_t* Minicover;
    ::size_t Size;
    int Samplerate;
    int Bitrate;

    bool operator==( const ::media& m ) const { return ID == m.ID; }
};

inline ::std::unordered_map< ::uint32_t, ::media > Library;

enum struct Playback { Linear, Repeat, Shuffle, Queue, Count };
enum struct SortTypes { Time, Artist, Title, Count };

struct Volume {
    ::std::wstring name;
    ::uint32_t* minicover;
};
inline ::std::unordered_map< ::uint32_t, ::Volume > MixerEntries;
inline ::std::vector< ::uint32_t > MixersActive;

namespace Saved {
    inline ::uint32_t Playing;
    inline ::SortTypes Sorting;
    inline ::Playback Playback;
    inline ::uint8_t Queue;
    inline ::std::vector< ::std::vector< ::uint32_t > > Queues;
    inline ::std::unordered_map< ::uint32_t, double > Volumes;
    inline ::std::unordered_map< ::uint32_t, double > Mixers;
};

inline ::std::vector< ::uint32_t >& Queue() { return ::Saved::Queues[ ::Saved::Queue ]; }

inline ::std::vector< ::uint32_t > SongDisplay {};

static const ::std::wstring SongPath = L"F:/SoundStuff/Sounds/";

inline void SongInfo( ::uint32_t song ) {
    ::DisplayText.clear();

    if ( !::Library.contains( song ) )
        return;

    ::DisplayText.push_back( ::Library[ song ].Title );
    ::DisplayText.push_back( ::Library[ song ].Artist );
    ::DisplayText.push_back( ::Library[ song ].Album );
    ::DisplayText.push_back( ::Library[ song ].Encoding );
    ::DisplayText.push_back( ::String::WConcat( ::Library[ song ].Duration, L"s" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Library[ song ].Size, L"mb" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Library[ song ].Bitrate, L"kbps" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Library[ song ].Samplerate, L"Hz" ) );
    ::DisplayText.push_back( ::String::WConcat( ::Saved::Volumes[ song ], L"%" ) );
}

inline double cursor;
inline ::ma_device Device;

inline ::relaxed_atomic< bool > PauseAudio = true;

inline void Remove( ::uint32_t id ) {
    ::std::unique_lock lock( ::CanvasMutex );

    if ( ::Library.contains( id ) )
        ::Library.erase( id );

    int index;

    index = ::Index( ::SongDisplay, id );
    if ( index > -1 )
        ::SongDisplay.erase( ::SongDisplay.begin() + index );

    index = ::Index( ::Queue(), id );
    if ( index > -1 )
        ::Queue().erase( ::Queue().begin() + index );
}

inline void Clean( ::Play& Play ) {
    if ( Play.Format ) ::avformat_close_input( &Play.Format );
    if ( Play.Codec ) ::avcodec_free_context( &Play.Codec );
    if ( Play.Packet ) ::av_packet_free( &Play.Packet );
    if ( Play.Frame ) ::av_frame_free( &Play.Frame );
    if ( Play.SWR ) ::swr_free( &Play.SWR );
    Play = {};
}

extern void DefaultDisplay( ::uint32_t entry );

extern void SetMixers();
extern void SetMixerVolume( ::uint32_t entry, double change );

extern void WriteCovers( ::media& media );

extern void Sort();
extern void Seek( int time );
extern void Decode( ::ma_device* device, ::uint8_t* output, ::ma_uint32 framecount );
extern void ArchiveSong( ::std::wstring& path );
extern void SetSong( ::uint32_t song );

namespace queue {
    inline static void clear() {
        ::Queue().clear();
        ::Queue().push_back( ::Saved::Playing );
    }

    inline static ::uint32_t road( ::std::vector< ::uint32_t >& line, int direction ) {
        if ( line.size() == 0 )
            return ::Saved::Playing;

        int target = direction + ::Index( line, ::Saved::Playing );

        if ( target < 0 )
            return line.back();
        if ( target >= line.size() )
            return line.front();
        return line[ target ];
    }

    inline static void add( ::uint32_t song, int direction ) {
        ::std::vector< ::uint32_t >& q = ::Queue();

        int in = ::Index( q, song );

        if ( in > -1 ) 
            q.erase( q.begin() + in );

        if ( direction > 0 )
            q.push_back( song );
        else
            q.emplace( q.begin(), song );
    }

    inline static void set( ::uint32_t song, int direction ) {
        if ( direction != 0 )
            add( song, direction );
        ::SetSong( song );
    }

    inline static void next( int direction ) {
        ::uint32_t target = 0;

        ::std::vector< ::uint32_t >& q = ::Queue();

        bool end = direction < 0 && ::Saved::Playing == q.front() || direction > 0 && ::Saved::Playing == q.back();

        switch ( ::Saved::Playback ) {
            case ::Playback::Repeat:
                    if ( end )
                        target = road( ::SongDisplay, direction );
                    else
                        target = road( q, direction );
                break;
            case ::Playback::Linear:
                    if ( end )
                        target = road( ::SongDisplay, direction );
                    else
                        target = road( q, direction );
                break;
            case ::Playback::Queue:
                    target = road( q, direction );
                break;
            case ::Playback::Shuffle:
                    if ( end )
                        target = ::SongDisplay[ ::RNG() % ::SongDisplay.size() ];
                    else
                        target = road( q, direction );
                break;
            default:
                break;
        }

        if ( ::Library.contains( target ) && target != ::Saved::Playing )
            set( target, direction );
        else
            ::Seek( 0 );
    }
}