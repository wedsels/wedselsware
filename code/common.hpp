#pragma once

#define STRICT 1
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <unordered_set>
#include <filesystem>
#include <functional>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <ranges>
#include <mutex>
#include <map>

#define MINICOVER 64
#define FONTHEIGHT 16
#define FONTSPACE 8
#define WINLEFT 3440
#define WINTOP 0
#define WINWIDTH 2560
#define WINHEIGHT 1440
#define MIDPOINT 512
#define SEEK 12

#define HR( hr ) do { ::std::cerr<<#hr<<'\n'; ::HRESULT res = hr; if ( FAILED( res ) ) return res; } while ( 0 )
#define HER( hr ) do { ::std::cerr<<#hr<<'\n'; ::HRESULT res = hr; if ( FAILED( res ) ) { ::Box( ::std::system_category().message( res ).c_str() ); return res; } } while ( 0 )
#define THREAD( body ) do { ::std::thread( [ = ] { body } ).detach(); } while ( 0 )

#define FUNCTION( func, ... ) do { ::std::cerr<<"function-"<<#func<<'\n'; ::Message( WM_FUNCTION, 0, ( ::LPARAM )&func, __VA_ARGS__ ); } while ( 0 )

#define WM_QUEUENEXT ( WM_USER + 1 )
#define WM_FUNCTION ( WM_USER + 2 )

#define VK_SCROLL1 0x0E
#define VK_SCROLL2 0x0F

#define HELD( vk ) ( ::Toggled[ vk ] )
#define PRESSED( vk ) ( ::Toggled[ vk ] && !::OldToggled[ vk ] )
#define RELEASED( vk ) ( !::Toggled[ vk ] && ::OldToggled[ vk ] )
#define DIRECTION( vk ) ( PRESSED( vk##1 ) ? 1 : PRESSED( vk##2 ) ? -1 : 0 )

inline ::std::vector< ::std::function< void() > > FunctionQueue;

inline ::HWND hwnd;
inline ::HWND desktophwnd;
inline ::HWND consolehwnd;

extern ::HRESULT InitDirectory( const wchar_t* path, ::std::function< void( ::std::wstring& ) > add, ::std::function< void( ::std::wstring& ) > remove );
extern ::HRESULT InitGraphics();
extern ::HRESULT InitDevice();
extern ::HRESULT InitInput();
extern ::HRESULT InitMixer();
extern ::HRESULT InitFont();

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
        else if ( i == '\n' || i == '\r' )
            i = ' ';
        else
            i = ::toupper( i );
}

template < typename... T >
inline void Box( T... text ) {
    ::std::wstringstream wss;
    ( wss << ... << text );
    ::MessageBoxW( NULL, wss.str().c_str(), L"", MB_OK );
}

inline void SetTop() { ::SetWindowPos( ::hwnd, HWND_TOPMOST, WINLEFT, WINTOP, WINWIDTH, WINHEIGHT, SWP_NOACTIVATE ); }

inline void Execute( ::std::wstring str, int type = 0 ) {
    ::SetTop();

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
    ::CreateProcessW( NULL, ( ::LPWSTR )str.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi );
}

template < typename T >
inline int Index( ::std::vector< T >& list, const T& item  ) {
    if ( !item || list.empty() )
        return -1;

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

inline void Message( ::UINT wm, ::WPARAM wParam = 0, ::LPARAM lParam = 0, bool block = true ) {
    ::MSG msg = { 0 };
    if ( !::PeekMessageW( &msg, ::hwnd, wm, wm, PM_NOREMOVE ) ) {
        if ( block )
            ::SendMessageW( ::hwnd, wm, wParam, lParam );
        else
            ::PostMessageW( ::hwnd, wm, wParam, lParam );
    }
}

template < typename T >
inline void EnumNext( T& e, bool b = false ) { e = ( T )( ( ( int )e - ( ( int )b * 2 - 1 ) + ( int )T::Count ) % ( int )T::Count ); }

template< typename T >
struct relaxed_atomic {
    ::std::atomic< T > v;

    relaxed_atomic() = default;
    constexpr relaxed_atomic( T value ) : v( value ) {}

    operator T() const noexcept {
        return v.load( ::std::memory_order_relaxed );
    }

    relaxed_atomic& operator=( T value ) noexcept {
        v.store( value, ::std::memory_order_relaxed );
        return *this;
    }
};

inline ::relaxed_atomic< bool > Toggled[ UINT8_MAX ];
inline ::relaxed_atomic< bool > OldToggled[ UINT8_MAX ];

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
    bool within( ::POINT p ) const { return p.x >= l && p.x < r && p.y >= t && p.y < b; }
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

namespace Input {
    inline ::POINT mouse;

    extern ::std::unordered_map< ::DWORD, ::std::function< bool() > > globalkey;
};