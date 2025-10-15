#include "common.hpp"

#include <shobjidl.h>
#include <shlguid.h>

::std::wstring String::SUpper( ::std::wstring& string ) {
    ::std::transform( string.begin(), string.end(), string.begin(), ::toupper );
    return string;
}

::std::wstring String::SLower( ::std::wstring& string ) {
    ::std::transform( string.begin(), string.end(), string.begin(), ::towlower );
    return string;
}

::std::string String::CLower( char str[] ) {
    ::std::string string;

    for ( size_t i = 0; str[ i ] != '\0'; ++i )
        string += ::std::tolower( str[ i ] );

    return string;
}

::std::string String::CUpper( char str[] ) {
    ::std::string string;

    for ( size_t i = 0; str[ i ] != '\0'; ++i )
        string += ::std::toupper( str[ i ] );

    return string;
}

::std::string String::WideUtf8( const ::std::wstring& str ) {
    if ( str.empty() )
        return ::std::string();

    int len = ::WideCharToMultiByte( CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr );
    ::std::string utf8( len, 0 );
    ::WideCharToMultiByte( CP_UTF8, 0, str.c_str(), -1, &utf8[ 0 ], len, nullptr, nullptr );

    utf8.pop_back();

    return utf8;
}

::std::string String::WideAnsii( const ::std::wstring& str ) {
    if ( str.empty() )
        return ::std::string();

    int len = ::WideCharToMultiByte( CP_ACP, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr );
    ::std::string ansii( len, 0 );
    ::WideCharToMultiByte( CP_ACP, 0, str.c_str(), -1, &ansii[ 0 ], len, nullptr, nullptr );
    
    ansii.pop_back();

    return ansii;
}

::std::wstring String::Utf8Wide( const ::std::string& str ) {
    if ( str.empty() )
        return ::std::wstring();

    int len = ::MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, nullptr, 0 );
    ::std::wstring wide( len, 0 );
    ::MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, &wide[ 0 ], len );

    wide.pop_back();

    return wide;
}

::std::wstring String::ResolveLnk( const ::std::wstring& str ) {
    static ::IShellLinkW* shell = nullptr;
    static ::IPersistFile* file = nullptr;
    
    ::WCHAR path[ MAX_PATH ] = { 0 };

    if ( !shell )
        ::CoCreateInstance( CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, ( void** )&shell );
    if ( !shell ) return L"";

    if ( !file )
        shell->QueryInterface( IID_IPersistFile, ( void** )&file );
    if ( !file ) return L"";

    if ( SUCCEEDED( file->Load( str.c_str(), STGM_READ ) ) )
        if ( SUCCEEDED( shell->GetPath( path, MAX_PATH, nullptr, SLGP_UNCPRIORITY ) ) ) {
            for ( auto& i : path )
                if ( i == '\\' )
                    i = '/';

            return path;
        }

    return L"";
}