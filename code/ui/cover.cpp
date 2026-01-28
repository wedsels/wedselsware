#include "../audio/audio.hpp"

::std::unordered_map< ::Playback, ::std::wstring > PBNames = {
    { ::Playback::Queue, L"Queue" },
    { ::Playback::Linear, L"Linear" },
    { ::Playback::Repeat, L"Repeat" },
    { ::Playback::Shuffle, L"Shuffle" }
};

::std::unordered_map< ::SortTypes, ::std::wstring > STNames = {
    { ::SortTypes::Time, L"Time" },
    { ::SortTypes::Title, L"Title" },
    { ::SortTypes::Artist, L"Artist" }
};

struct Cover : ::UI {
    ::Rect Bounds = { ( WINWIDTH - MIDPOINT ) / 2, 0, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    struct SeekBarUI : ::BarUI {
        ::Cover* UI;
        SeekBarUI( ::Cover* ui ) : UI( ui ) {}

        ::Rect& BaseRect() { return UI->Rect(); }

        bool Active() { return UI->Active(); }

        void OffsetChanged() { ::Seek( ::cursor ); }

        double& Reference() { return ::cursor; }
        double Count() const { return ::Playing.Duration; }
    };
    SeekBarUI SeekBar;
    Cover() : SeekBar( this ) {}

    void Enter() {
        ::SongInfo( ::Saved::Playing );
        ::DisplayText.push_back( ::String::WConcat( L"Playback: ", PBNames[ ::Saved::Playback ] ) );
        ::DisplayText.push_back( ::String::WConcat( L"Sorting: ", STNames[ ::Saved::Sorting ] ) );
    }

    ::uint32_t LastPlayed;
    double LastVolume;

    void Clear() { LastPlayed = 0; LastVolume = 0.0; }

    bool BlockDraw() { return ::Saved::Playing == LastPlayed && LastVolume == ::Saved::Volumes[ ::Saved::Playing ]; }
    void Draw() {
        ::Rect b = Bounds;
        if ( Slide > 0 ) {
            b.t -= Slide;
            b.b -= Slide;
        }

        ::DrawImage( b, ::Playing.Cover + MIDPOINT * Slide );

        LastPlayed = ::Saved::Playing;
        LastVolume = ::Saved::Volumes[ ::Saved::Playing ];
    }

    void Leave() { ::DisplayText.clear(); }

    void Click() {
        if ( PRESSED( VK_LBUTTON ) )
            ::EnumNext( ::Saved::Playback, HELD( VK_SHIFT ) );
        else if ( PRESSED( VK_RBUTTON ) ) {
            ::EnumNext( ::Saved::Sorting, HELD( VK_SHIFT ) );
            ::Sort();
            Redraw();
        }
    }

    void XButton() {
        if ( PRESSED( VK_XBUTTON1 ) || PRESSED( VK_XBUTTON2 ) )
            ::queue::next( -DIRECTION( VK_XBUTTON ) );
    }

    void Scroll() {
        ::Saved::Volumes[ ::Saved::Playing ] = ::std::max( 0.0, ::Saved::Volumes[ ::Saved::Playing ] + DIRECTION( VK_SCROLL ) * ( HELD( VK_CONTROL ) ? -0.01 : -0.001 ) );
    }
} Cover;