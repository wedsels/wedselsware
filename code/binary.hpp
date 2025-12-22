#include "common.hpp"

#define SERIALIZE( name ) do { if ( s.open( #name ) ) s.write( ::Saved::name ); } while ( 0 )
#define DESERIALIZE( name ) do { if ( d.open( #name ) ) d.read( ::Saved::name ); } while ( 0 )

struct Serializer {
    ::std::ofstream file;
    inline bool open( const char* name ) { return ( bool )( file = ::std::ofstream( name, ::std::ios::binary ) ); }

    template< typename T >
    typename ::std::enable_if< ::std::is_trivially_copyable< T >::value, void >::type
    inline write( const T& value ) { file.write( reinterpret_cast< const char* >( &value ), sizeof( T ) ); }

    template< typename T >
    inline void write( const ::std::vector< T >& vec ) {
        ::uint64_t size = vec.size();
        write( size );
        for ( const T& item : vec )
            write( item );
    }

    template< typename K, typename V >
    inline void write( const ::std::unordered_map< K, V >& map ) {
        ::uint64_t size = map.size();
        write( size );
        for ( const auto& pair : map ) {
            write( pair.first );
            write( pair.second );
        }
    }
};

struct Deserializer {
    ::std::ifstream file;
    inline bool open( const char* name ) { return ( bool )( file = ::std::ifstream( name, ::std::ios::binary ) ); }

    template< typename T >
    typename ::std::enable_if< ::std::is_trivially_copyable< T >::value, void >::type
    inline read( T& value ) { file.read( reinterpret_cast< char* >( &value ), sizeof( T ) ); }

    template< typename T >
    inline void read( ::std::vector< T >& vec ) {
        ::uint64_t size;
        read( size );
        vec.resize( size );
        for ( T& item : vec )
            read( item );
    }

    template< typename K, typename V >
    inline void read( ::std::unordered_map< K, V >& map ) {
        ::uint64_t size;
        read( size );
        map.clear();
        for ( ::uint64_t i = 0; i < size; ++i ) {
            K key;
            V value;
            read( key );
            read( value );
            map.emplace( ::std::move( key ), ::std::move( value ) );
        }
    }
};