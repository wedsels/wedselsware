#include "../common.hpp"

#define COLORALPHA 0x00000000
#define COLORGHOST 0x64646464
#define COLORPALE 0xFFB0ECDA
#define COLORGOLD 0xFFD7CD64
#define COLORPURPLE 0xFFA04E88
#define COLORGREEN 0xFF71B33C
#define COLORBLUE 0xFFCD5A6A
#define COLORPINK 0xFF6400D7
#define COLORYELLOW 0xFFCFF3FC
#define COLORSTEEL 0xFF6D6D5D
#define COLORCORAL 0xFF6370EC
#define COLORCOLD 0xFFF1F0EC

#define MINICOVER 64
#define SPACING 3
#define COLUMNS 9
#define ROWS 12
#define FONTHEIGHT 24
#define FONTSPACE ( FONTHEIGHT / 3.0 )
#define WINLEFT 3440
#define WINTOP 0
#define WINWIDTH 2560
#define WINHEIGHT 1440

inline constexpr int MIDPOINT = ( MINICOVER + SPACING ) * COLUMNS - SPACING;

inline ::uint32_t Frame;

inline ::std::shared_mutex CanvasMutex;

inline const ::uint32_t CanvasSize = WINWIDTH * WINHEIGHT * sizeof( ::uint32_t );
inline ::uint32_t* Canvas;

inline ::std::wstring Searching;
inline ::std::vector< ::uint32_t > Search;

inline bool PauseDraw = true;

enum struct GridTypes { Songs, Queue, Search, Mixer, Apps, Webs, Count };
inline ::GridTypes GridType = ::GridTypes::Songs;
inline int DisplayOffset = 0;
inline ::std::function< ::std::wstring() > DisplayInformation[ ::MIDPOINT / ( FONTHEIGHT + SPACING ) ];
extern ::std::vector< ::uint32_t >& GetDisplay();

extern void RenderFrame();
extern ::uint32_t* ResizeImage( ::uint32_t* data, int w, int h, int scale );
extern ::uint32_t* ArchiveImage( const char* path, int size );
extern ::uint32_t* ArchiveImage( ::uint8_t* imgdata, int size, int scale );
extern ::uint32_t* ArchiveHICON( ::LPCWSTR path, int size );

extern int TextWidth( ::std::wstring& text );

extern void SetPixel( int x, int y, ::uint32_t color );
extern void DrawBox( ::Rect t, ::uint32_t b, ::std::optional< ::Input::Click > c = {} );
extern void DrawImage( ::Rect r, ::uint32_t* img, ::std::optional< ::Input::Click > c = {} );
extern void DrawString( int x, int y, int width, ::std::wstring& s, ::std::optional< ::Input::Click > c = {} );
extern void CheckClick( ::Rect r, ::std::optional< ::Input::Click > c );

inline bool EmptyImage( const ::uint32_t* img, ::size_t s ) {
    for ( ::size_t i = 0; i < s; ++i )
        if ( img[ i ] != 0 )
            return false;
    return true;
}

struct UI {
    virtual ::uint8_t GetPriority() const { return 255; }

    static ::std::vector< ::UI* >& Registry() {
        static ::std::vector< ::UI* > instances;
        return instances;
    }

    UI() {
        Registry().push_back( this );
        ::std::sort( ::UI::Registry().begin(), ::UI::Registry().end(), []( ::UI* a, ::UI* b ) {
            return a->GetPriority() > b->GetPriority();
        } );
    }
    virtual ~UI() = default;

    virtual void Initialize() {}
    virtual void Draw() {}
    virtual void Clear() {}
};

struct Font {
    static inline ::std::unordered_map< wchar_t, ::Font > Letters = {};

    ::uint32_t* map;
    int width, height, yoff;
};

extern ::Font ArchiveFont( wchar_t c );