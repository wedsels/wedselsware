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
#define FONTHEIGHT 16
#define FONTSPACE 8
#define WINLEFT 3440
#define WINTOP 0
#define WINWIDTH 2560
#define WINHEIGHT 1440
#define MIDPOINT 512
#define SEEK 12

inline ::uint32_t Frame;

inline ::std::mutex CanvasMutex;

inline const ::uint32_t CanvasSize = WINWIDTH * WINHEIGHT * sizeof( ::uint32_t );
inline ::uint32_t* Canvas;

enum struct GridTypes { Songs, Search, Mixer, Apps, Webs, Count };
inline ::GridTypes GridType = ::GridTypes::Songs;

inline ::std::vector< ::std::wstring > DisplayText;

extern ::uint32_t* ResizeImage( ::uint32_t* data, int w, int h, int scale );
extern ::uint32_t* ArchiveImage( const char* path, int size );
extern ::uint32_t* ArchiveImage( ::uint8_t* imgdata, int size, int scale );
extern ::uint32_t* ArchiveHICON( ::LPCWSTR path, int size );

extern int TextWidth( ::std::wstring& text );

extern void SetPixel( int x, int y, ::uint32_t color );
extern void DrawBox( ::Rect& t, ::uint32_t b );
extern void DrawImage( ::Rect& r, ::uint32_t* img );
extern void DrawString( int x, int y, int width, ::std::wstring& s );

inline void SetTop() { ::SetWindowPos( ::hwnd, HWND_TOPMOST, WINLEFT, WINTOP, WINWIDTH, WINHEIGHT, SWP_NOACTIVATE ); }

inline bool EmptyImage( const ::uint32_t* img, ::size_t s ) {
    for ( ::size_t i = 0; i < s; ++i )
        if ( img[ i ] )
            return false;
    return true;
}

inline ::uint32_t ToColor( double t ) {
    t *= 32.0;

    static const ::std::function< ::uint8_t( double, double ) > color = []( double t, double off ) {
        return ::uint8_t( 255.0 * ( 0.5 + 0.5 * ::std::sin( t + off ) ) );
    };

    return 0xFF000000 | ( color( t, 0.0 ) << 16 ) | ( color( t, 2.094 ) << 8 ) | color( t, 4.188 );
}

struct Font {
    static inline ::std::unordered_map< wchar_t, ::Font > Letters = {};

    ::uint32_t* map;
    int width, height, yoff;
};

extern ::Font ArchiveFont( wchar_t c );

struct UI {
    inline static const int MaxSlide = MIDPOINT + SEEK;

    inline static ::relaxed_atomic< int > Slide = MaxSlide;
    inline static ::relaxed_atomic< bool > Expand;

    inline static ::std::unordered_set< ::UI* > UIs;
    UI() { UIs.insert( this ); }

    inline static void Redraw() {
        for ( ::UI* i : ::UI::UIs )
            i->Clear();
    }

    virtual void Initialize() {}
    virtual void Clear() {}

    virtual bool BlockDraw() { return true; }
    virtual void Draw() {}
    inline static void PreDraw() {
        static const int step = 20;

        if ( Expand && Slide > 0 )
            Slide = ::std::max( Slide - step, 0 );
        else if ( !Expand && Slide < MaxSlide )
            Slide = ::std::min( Slide + step, MaxSlide );
    }

    virtual bool Active() { return true; }

    virtual void Key() {}
    virtual void Move() {}
    virtual void Click() {}
    virtual void Scroll() {}
    virtual void XButton() {}
    virtual void Enter() {}
    virtual void Leave() {}

    virtual bool BlockKey() { return false; }
    virtual bool BlockClick() { return true; }

    inline static ::Rect _;
    virtual ::Rect& Rect() { return _; }
};

struct BarUI : ::UI {
    inline static double none;
    virtual double& Reference() { return none; }
    virtual double Count() const { return none; }
    virtual double Size() const { return none; }

    ::Rect Bounds = {};
    ::Rect& Rect() { return Bounds; }

    virtual ::Rect& BaseRect() { return _; };

    virtual void OffsetChanged() {}

    void SetOffset() {
        const double half = BaseRect().width() / 2.0;
        double loc = ( ::Input::mouse.x - BaseRect().l );
        if ( loc >= half )
            loc = half - ( loc - half );
        loc /= ::std::max( 1.0, half );
        Reference() = loc * ::std::max( 1.0, Count() - Size() );
    }

    void Move() {
        if ( !HELD( VK_LBUTTON ) )
            return;

        SetOffset();
        OffsetChanged();
    }

    void Click() {
        if ( PRESSED( VK_LBUTTON ) )
            SetOffset();
        else if ( PRESSED( VK_RBUTTON ) )
            Reference() = 0.0;
        else if ( PRESSED( VK_MBUTTON ) )
            Reference() = Count() - Size();

        OffsetChanged();
    }

    void Scroll() {
        Reference() = ::std::clamp( Reference() - DIRECTION( VK_SCROLL ), 0.0, ::std::max( 0.0, Count() - Size() ) );
        OffsetChanged();
    }

    void Enter() { ::DisplayText = { ::String::WConcat( Reference() + Size(), L" / ", Count(), L"" ) }; }
    void Leave() { ::DisplayText.clear(); }

    double OldReference;

    void Clear() { OldReference = 0.0; }

    bool BlockDraw() { return Reference() == OldReference; }
    void Draw() {
        OldReference = Reference();

        Bounds = BaseRect();
        Bounds.t = Bounds.b;
        Bounds.b += SEEK;
        if ( Slide > 0 ) {
            Bounds.t -= Slide;
            Bounds.b -= Slide;
        }

        double per = Reference() / ::std::max( 1.0, Count() - Size() );

        ::DrawBox( Bounds, ::ToColor( per ) );

        ::Rect b = Bounds;
        b.l += b.width() / 2.0 * per;
        b.r += Bounds.l - b.l;
        ::DrawBox( b, COLORGHOST );

        if ( Rect().within( ::Input::mouse ) )
            Enter();
    }
};

struct GridUI : ::UI {
    ::std::vector< ::uint32_t > Grid;

    const int Columns() { return Rect().width() / MINICOVER; };
    const int Rows() { return Rect().height() / MINICOVER; };
    double Offset;

    struct GridBarUI : ::BarUI {
        ::GridUI* UI;
        GridBarUI( ::GridUI* ui ) : UI( ui ) {}

        ::Rect& BaseRect() { return UI->Rect(); }

        bool Active() { return UI->Active(); }

        double& Reference() { return UI->Offset; }
        double Count() const { return UI->GetDisplay().size(); }
        double Size() const { return UI->Columns() * UI->Rows(); }
    };
    GridBarUI Bar;
    GridUI() : Bar( this ) {}

    int Index() {
        if ( !Expand )
            return -1;

        int index = ( int )Offset + ( ::Input::mouse.x - Rect().l ) / MINICOVER + ( ::Input::mouse.y - Rect().t ) / MINICOVER * Columns();
        if ( index < 0 || GetDisplay().size() <= index )
            return -1;

        return index;
    }

    virtual ::std::vector< ::uint32_t >& GetDisplay() { return Grid; }
    virtual ::uint32_t* GetImage( ::uint32_t item ) { return nullptr; }

    int LastIndex = -1;
    double LastOffset;
    ::size_t LastSize = -1;
    virtual void GridClear() {}
    void Clear() { LastIndex = -1; LastOffset = 0; LastSize = -1; GridClear(); }

    void SetOffset( int off ) {
        Offset = ::std::clamp( off, 0, ::std::max( 0, ( int )GetDisplay().size() - Columns() * Rows() ) );
    }

    virtual bool GridBlockDraw() { return true; }
    bool BlockDraw() { return Offset == LastOffset && GetDisplay().size() == LastSize && GridBlockDraw(); }
    void Draw() {
        LastOffset = Offset;
        LastSize = GetDisplay().size();

        ::Rect b = Rect();
        if ( Slide > 0 ) {
            b.t -= Slide;
            b.b -= Slide;
        }

        const int columns = b.width() / MINICOVER;
        const int rows = b.height() / MINICOVER;

        Grid.clear();

        for ( int i = 0; i < columns * rows; i++ ) {
            int loc = i + Offset;
            ::Rect rect { b.l + ( MINICOVER ) * ( i % columns ), b.t + ( MINICOVER ) * ( i / columns ), MINICOVER };

            if ( loc >= LastSize )
                ::DrawBox( rect, COLORALPHA );
            else {
                ::DrawImage( rect, GetImage( GetDisplay()[ loc ] ) );
                Grid.push_back( GetDisplay()[ loc ] );
            }
        }
    }

    virtual void GridKey( ::uint32_t item ) {}
    virtual void OutKey() {}
    void Key() {
        int i = Index();
        if ( i > -1 )
            GridKey( GetDisplay()[ i ] );
        else
            OutKey();
    }

    virtual void GridClick( ::uint32_t item ) {}
    virtual void OutClick() {}
    void Click() {
        int i = Index();
        if ( i > -1 )
            GridClick( GetDisplay()[ i ] );
        else
            OutClick();
    }

    virtual void GridLeave( ::uint32_t item ) {}
    virtual void OutLeave() {}
    virtual void GridEnter( ::uint32_t item ) {}
    virtual void OutEnter() {}

    void Enter() {
        int i = Index();
        if ( i > -1 )
            GridEnter( GetDisplay()[ i ] );
        else OutEnter();
    }

    void Leave() {
        ::DisplayText.clear();
        if ( LastIndex > -1 )
            GridLeave( GetDisplay()[ LastIndex ] );
        else OutLeave();
    }

    virtual void GridMove( ::uint32_t item ) {}
    void Move() {
        int i = Index();

        if ( LastIndex != i ) {
            Leave();
            Enter();

            GridMove( GetDisplay()[ i ] );
        }

        LastIndex = i;
    }

    void XButton() {
        if ( PRESSED( VK_XBUTTON1 ) || PRESSED( VK_XBUTTON2 ) ) {
            ::EnumNext( ::GridType, PRESSED( VK_XBUTTON2 ) );
            Redraw();
        }
    }

    virtual bool GridScroll( ::uint32_t item ) { return false; }
    void Scroll() {
        if ( HELD( VK_SHIFT ) || HELD( VK_CONTROL ) || HELD( VK_MENU ) ) {
            int i = Index();
            if ( i > -1 )
                GridScroll( GetDisplay()[ i ] );

            return;
        }

        SetOffset( Offset + DIRECTION( VK_SCROLL ) );
    }
};