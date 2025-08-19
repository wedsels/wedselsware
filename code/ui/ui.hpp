#include "../common.hpp"

#define MINICOVER 64
#define SPACING 3
#define COLUMNS 9
#define ROWS 12
#define FONTHEIGHT 22
#define FONTSPACE ( FONTHEIGHT / 3 )

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

inline ::std::atomic< ::uint32_t > Canvas[ WINWIDTH * WINHEIGHT ];

inline ::std::wstring Searching;
inline ::std::vector< ::uint32_t > Search;

inline ::std::atomic< bool > PauseDraw = true;

enum struct GridTypes { Songs, Queue, Search, Mixer, Count };
inline GridTypes GridType = ::GridTypes::Songs;
inline int DisplayOffset = 0;
inline ::std::function< ::std::wstring() > DisplayInformation[ MIDPOINT / ( FONTHEIGHT + SPACING ) ];
extern ::std::vector< ::uint32_t >& GetDisplay();

inline ::std::mutex CanvasMutex;
inline ::std::atomic< bool > CanvasBool;
inline ::std::condition_variable CanvasCondition;

extern void RenderFrame();
extern ::uint8_t* ResizeImage( ::uint8_t* data, int w, int h, int scale );
extern ::uint8_t* ArchiveImage( const char* path, int size );
extern ::uint8_t* ArchiveImage( uint8_t* imgdata, int size, int scale );
extern ::uint8_t* ArchiveHICON( ::HICON& icon, int size );

extern int TextWidth( ::std::wstring& text );

extern void DrawCursor();

extern void Draw( ::DrawType dt );
extern void DrawBox( ::rect t, ::uint32_t b, float& light, ::std::optional< ::input::click > c = {} );
extern void DrawImage( ::rect r, ::uint8_t* img, float light, ::std::optional< ::input::click > c = {}, ::uint8_t channels = 3 );
extern void DrawString( int x, int y, int width, ::std::wstring& s, ::std::optional< ::input::click > c = {} );
extern void CheckClick( ::rect r, float* light, ::std::optional< ::input::click > c );

inline ::uint32_t BGRA( ::uint8_t r, ::uint8_t g, ::uint8_t b, ::uint8_t a = 255, float light = 1.0f ) {
    float alpha = a / 255.0f;
    return ( ::uint8_t )( b * light * alpha ) | ( ( ::uint8_t )( g * light * alpha ) << 8 ) | ( ( ::uint8_t )( r * light * alpha ) << 16 ) | ( ( a ) << 24 );
}

inline ::uint32_t Multiply( ::uint32_t color, float mult ) {
    ::uint8_t b = ( color & 0x000000FF );
    ::uint8_t g = ( color & 0x0000FF00 ) >> 8;
    ::uint8_t r = ( color & 0x00FF0000 ) >> 16;
    ::uint8_t a = ( color & 0xFF000000 ) >> 24;

    return ::BGRA( r, g, b, a, mult );
}

inline ::uint32_t SetAlpha( ::uint32_t color, ::uint8_t alpha ) {
    return ( color & 0x00FFFFFF ) | ( alpha << 24 );
}

inline void SetPixel( int x, int y, ::uint32_t color ) {
    if ( y * WINWIDTH + x > WINWIDTH * WINHEIGHT )
        return;

    ::Canvas[ y * WINWIDTH + x ] = color;
}

inline void InitiateDraw() {
    ::CanvasBool = true;
    ::CanvasCondition.notify_one();
}

inline void Redraw( ::DrawType dt = ::DrawType::Normal ) {
    ::Message( WM_REDRAW, ( int )dt, NULL );
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
    virtual void Draw( ::DrawType dt ) {}
};

struct Font {
    static inline ::std::unordered_map< wchar_t, ::Font > Letters = {};

    ::uint32_t* map;
    int width, height, yoff;
};

extern ::Font ArchiveFont( wchar_t c );