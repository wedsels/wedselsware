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
#define HER( hr ) do { ::HRESULT res = hr; if ( FAILED( res ) ) { ::Box( ::std::system_category().message( res ).c_str() ); return res; } } while ( 0 )
#define THREAD( body ) do { ::std::thread( [ = ] { body } ).detach(); } while ( 0 )

#define WM_QUEUENEXT ( WM_USER + 1 )
#define WM_KEYBOARD ( WM_USER + 2 )
#define WM_REDRAW ( WM_USER + 3 )
#define WM_ACTION ( WM_USER + 4 )
#define WM_DEVICE ( WM_USER + 5 )
#define WM_MIXER ( WM_USER + 6 )
#define WM_MOUSE ( WM_USER + 7 )

struct Launch {
    ::std::wstring path;
    ::uint8_t* img;
};

inline ::HWND hwnd;
inline ::HWND desktophwnd;

extern ::HRESULT InitializeDirectory( const wchar_t* path, ::std::function< void( const wchar_t* ) > add, ::std::function< void( ::uint32_t ) > remove );
extern ::HRESULT InitializeMixer();
extern ::HRESULT InitializeFont();
extern ::HRESULT InitGraphics();
extern ::HRESULT InitDevice();
extern ::HRESULT InitInput();
extern ::HRESULT InitDraw();

extern void Mouse( int c, ::LPARAM l );
extern void Keyboard( int c, ::LPARAM l );

extern void DeleteLink( ::uint32_t id, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map );
extern void ArchiveLink( ::std::wstring path, ::std::vector< ::uint32_t >& ids, ::std::unordered_map< ::uint32_t, ::Launch >& map );
extern void UpdateDirectories( ::MSG& msg );

inline ::std::vector< ::uint32_t > Apps;
inline ::std::unordered_map< ::uint32_t, ::Launch > AppsPath;
inline ::std::vector< ::uint32_t > Webs;
inline ::std::unordered_map< ::uint32_t, ::Launch > WebsPath;

namespace String {
    template< typename... Args >
    inline ::std::wstring WConcat( Args&&... args ) {
        ::std::wstringstream wss;
        ( wss << ... << args );
        return wss.str();
    }

    inline int Key( ::std::wstring& str, ::DWORD key ) {
        switch ( key ) {
            case VK_BACK: if ( str.size() > 0 ) str.pop_back(); return 2;
            case VK_SPACE: str += L" "; return 1;
            case VK_DELETE: str.clear(); return 2;
            default:
                    if ( key >= '0' && key <= '9' || key >= 'A' && key <= 'Z' ) {
                        str += ( wchar_t )( key );
                        return 1;
                    }
                return 0;
        }
    }

    template< typename str >
    inline ::uint32_t Hash( str& s ) {
        ::uint32_t hash = 2166136261u;

        auto ps = s.c_str();

        while ( *ps ) {
            hash ^= static_cast< ::uint32_t >( *ps++ );
            hash *= 16777619u;
        }

        return hash;
    }

    ::std::wstring SLower( ::std::wstring& string );
    ::std::wstring SUpper( ::std::wstring& string );
    ::std::string CLower( char* str );
    ::std::string CUpper( char* str );
    ::std::string WideUtf8( const ::std::wstring& str );
    ::std::string WideAnsii( const ::std::wstring& str );
    ::std::wstring Utf8Wide( const ::std::string& str );
    ::std::wstring ResolveLnk( const ::std::wstring& str );
};

inline void Path( ::std::wstring& path ) {
    for ( auto& i : path )
        if ( i == L'\\' )
            i = L'/';
}

template < typename... T >
inline void Box( T... text ) {
    ::std::wstringstream wss;
    ( wss << ... << text );
    ::MessageBoxW( NULL, wss.str().c_str(), L"", MB_OK );
}

inline void Execute( ::std::wstring str, int type = 0 ) {
    for ( auto& i : str )
        if ( i == '/' )
            i = '\\';

    switch ( type ) {
        case 0: str = ::String::WConcat( L"explorer.exe \"", str, L"\"" ); break;
        case 1: str = ::String::WConcat( L"explorer.exe /select,\"", str, L"\"" ); break;
        case 2: break;
    }

    ::PROCESS_INFORMATION pi = { 0 };
    ::STARTUPINFOW si = { 0 };
    si.cb = sizeof( si );
    ::SetForegroundWindow( desktophwnd );
    ::SetFocus( desktophwnd );
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
    static ::std::uniform_int_distribution distribution( 0, INT_MAX );

    return distribution( seed );
}

inline void Message( ::UINT wm, ::WPARAM wParam, ::LPARAM lParam ) {
    ::MSG msg = { 0 };
    if ( !::PeekMessageW( &msg, ::hwnd, wm, wm, PM_NOREMOVE ) )
        ::PostMessageW( ::hwnd, wm, wParam, lParam );
}

template < typename T >
inline void EnumNext( T& e, bool b = false ) { e = ( T )( ( ( int )e - ( ( int )b * 2 - 1 ) + ( int )T::Count ) % ( int )T::Count ); }

struct Rect {
    int l, t, r, b;

    Rect() { l = t = r = b = 0; }

    Rect( int left, int top, int right, int bottom ) {
        l = left;
        t = top;
        r = right;
        b = bottom;
    }

    Rect( int left, int top, int size ) {
        l = left;
        t = top;
        r = left + size;
        b = top + size;
    }

    int width() const { return r - l; }
    int height() const { return b - t; }
    bool empty() const { return t == b || l == r; }
    bool within( ::POINT p ) const { return p.x >= l && p.x <= r && p.y >= t && p.y <= b; }
    bool operator==( const ::Rect& c ) const { return l == c.l && t == c.t && r == c.r && b == c.b; }
    ::POINT topleft() const { return { l, t }; }
    ::POINT topright() const { return { r, t }; }
    ::POINT bottomleft() const { return { l, b }; }
    ::POINT bottomright() const { return { r, b }; }
    ::std::wstring bounds() const { return ::String::WConcat( l, " : ", t, " : ", r, " : ", b ); }
};

namespace std {
    template <>
    struct hash< ::Rect > { int operator()( const ::Rect& r ) const { return ( ( ( r.l ^ ( r.t << 1 ) ) ^ ( r.r << 1 ) ) ^ ( r.b << 1 ) ); } };
}

enum struct DrawType { None, Normal, Redo };

namespace Input {
    inline ::Rect hover;

    inline bool passthrough;

    inline ::std::atomic< bool > State[ VK_OEM_CLEAR ];

    inline ::POINT mouse;

    struct Click;
    inline ::std::unordered_map< ::Rect, Click > clicks;

    extern ::std::unordered_map< int, ::std::function< bool( bool ) > > globalkey;

    struct Click {
        ::DrawType intensity = ::DrawType::Redo;

        ::std::function< void() > hvr;
        ::std::function< void() > lmb;
        ::std::function< void() > rmb;
        ::std::function< void() > mmb;
        ::std::function< void( int ) > xmb;
        ::std::function< void( int ) > scrl;
        ::std::function< void( ::DWORD ) > key;

        static void create( ::Rect& rect, Click& c ) { clicks[ rect ] = ::std::move( c ); }
    };
};  