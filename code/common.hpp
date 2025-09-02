#pragma once

#define STRICT 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <concurrent_unordered_map.h>
#include <condition_variable>
#include <unordered_set>
#include <filesystem>
#include <functional>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <mutex>
#include <map>

#define HR( hr ) do { ::HRESULT res = hr; if ( FAILED( res ) ) return res; } while ( 0 )
#define HER( hr ) do { ::HRESULT res = hr; if ( FAILED( res ) ) { ::box( ::std::system_category().message( res ).c_str() ); return res; } } while ( 0 )
#define THREAD( body ) do { ::std::thread( [ & ] { body } ).detach(); } while ( 0 )

#define WM_QUEUENEXT ( WM_USER + 1 )
#define WM_KEYBOARD ( WM_USER + 2 )
#define WM_REDRAW ( WM_USER + 3 )
#define WM_ACTION ( WM_USER + 4 )
#define WM_MIXER ( WM_USER + 5 )
#define WM_MOUSE ( WM_USER + 6 )
#define WM_SONG ( WM_USER + 7 )

#define WINLEFT 3440
#define WINTOP 5
#define WINWIDTH 2560
#define WINHEIGHT 1440
#define MIDPOINT 600
#define CURSOR ::LoadCursor( NULL, IDC_ARROW )

inline ::HWND hwnd;
inline ::HWND desktophwnd;

extern ::HRESULT InitializeDirectory();
extern ::HRESULT InitializeMixer();
extern ::HRESULT InitializeFont();
extern ::HRESULT InitGraphics();
extern ::HRESULT InitDevice();
extern ::HRESULT InitInput();
extern ::HRESULT InitDraw();

extern void Mouse( int c, ::LPARAM l );
extern void Keyboard( int c, ::LPARAM l );

namespace string {
    template< typename... Args >
    inline ::std::wstring wconcat( Args&&... args ) {
        ::std::wstringstream wss;
        ( wss << ... << args );
        return wss.str();
    }

    inline void key( ::std::wstring& str, ::DWORD key ) {
        switch ( key ) {
            case VK_BACK: if ( str.size() > 0 ) str.pop_back(); break;
            case VK_SPACE: str += L" "; break;
            case VK_DELETE: str.clear(); break;
            default: str += ( wchar_t )( key ); break;
        }
    }

    template< typename str >
    inline ::uint32_t hash( str& s ) {
        ::uint32_t hash = 2166136261u;

        auto ps = s.c_str();

        while ( *ps ) {
            hash ^= static_cast< ::uint32_t >( *ps++ );
            hash *= 16777619u;
        }

        return hash;
    }

    ::std::wstring slower( ::std::wstring& string );
    ::std::wstring supper( ::std::wstring& string );
    ::std::string clower( char* str );
    ::std::string cupper( char* str );
    ::std::string wideutf8( const ::std::wstring& str );
    ::std::string wideansii( const ::std::wstring& str );
    ::std::wstring utf8wide( const ::std::string& str );
};

template < typename... T >
inline void box( T... text ) {
    ::std::wstringstream wss;
    ( wss << ... << text );
    ::MessageBoxW( NULL, wss.str().c_str(), L"", MB_OK );
}

inline void execute( ::std::wstring str, int type = 0 ) {
    for ( auto& i : str )
        if ( i == '/' )
            i = '\\';

    switch ( type ) {
        case 0: str = ::string::wconcat( L"explorer.exe \"", str, L"\"" ); break;
        case 1: str = ::string::wconcat( L"explorer.exe /select,\"", str, L"\"" ); break;
        case 2: break;
    }

    ::PROCESS_INFORMATION pi = { 0 };
    ::STARTUPINFOW si = { 0 };
    si.cb = sizeof( si );
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_SHOW;
    ::CreateProcessW( NULL, ( ::LPWSTR )str.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi );
}

template < typename T >
inline int Index( ::std::vector< T >& list, const T& item  ) {
    auto it = ::std::find( list.begin(), list.end(), item );

    if ( it != list.end() )
        return ::std::distance( list.begin(), it );
    return -1;
}

inline int RNG() {
    static ::std::mt19937 seed( ::std::random_device{}() );
    static ::std::uniform_int_distribution<> distribution( 0, INT_MAX );

    return distribution( seed );
}

inline void Message( ::UINT wm, ::WPARAM wParam, ::LPARAM lParam ) {
    ::MSG msg = { 0 };
    if ( !::PeekMessageW( &msg, ::hwnd, wm, wm, PM_NOREMOVE ) )
        ::PostMessageW( ::hwnd, wm, wParam, lParam );
}

template < typename T >
inline void EnumNext( T& e ) { e = ( T )( ( ( int )e + 1 ) % ( int )T::Count ); }

struct rect {
    int l, t, r, b;

    rect() { l = t = r = b = 0; }
    
    rect( int left, int top, int right, int bottom ) {
        l = left;
        t = top;
        r = right;
        b = bottom;
    }
    
    rect( int left, int top, int size ) {
        l = left;
        t = top;
        r = left + size;
        b = top + size;
    }

    int width() const { return r - l; }
    int height() const { return b - t; }
    bool empty() const { return t == b || l == r; }
    bool within( ::POINT p ) const { return p.x >= l && p.x <= r && p.y >= t && p.y <= b; }
    bool operator==( const ::rect& c ) const { return l == c.l && t == c.t && r == c.r && b == c.b; }
    ::POINT topleft() const { return { l, t }; }
    ::POINT topright() const { return { r, t }; }
    ::POINT bottomleft() const { return { l, b }; }
    ::POINT bottomright() const { return { r, b }; }
    ::std::wstring bounds() const { return ::string::wconcat( l, " : ", t, " : ", r, " : ", b ); }
};

namespace std {
    template <>
    struct hash< ::rect > {
        ::size_t operator()( const ::rect& r ) const {
            ::size_t h1 = hash< int >{}( r.l );
            ::size_t h2 = hash< int >{}( r.t );
            ::size_t h3 = hash< int >{}( r.r );
            ::size_t h4 = hash< int >{}( r.b );

            return ( ( ( h1 ^ ( h2 << 1 ) ) ^ ( h3 << 1 ) ) ^ ( h4 << 1 ) );
        }
    };
}

enum struct DrawType { None, Normal, Redo };

namespace input {
    inline ::rect hover;

    namespace state {
        inline bool lmb;
        inline bool rmb;
        inline bool mmb;
        inline bool shift;
        inline bool ctrl;
        inline bool alt;
    };

    inline ::POINT mouse;

    struct click;
    inline ::std::unordered_map< ::rect, ::input::click > clicks;

    extern ::std::unordered_map< int, ::std::function< bool( bool ) > > globalkey;

    struct click {
        ::DrawType intensity = ::DrawType::Redo;

        ::std::function< void() > hvr;
        ::std::function< void() > lmb;
        ::std::function< void() > rmb;
        ::std::function< void() > mmb;
        ::std::function< void( int ) > xmb;
        ::std::function< void( int ) > scrl;
        ::std::function< void( ::DWORD ) > key;

        static void create( ::rect& rect, click& c ) { clicks[ rect ] = ::std::move( c ); }
    };
};