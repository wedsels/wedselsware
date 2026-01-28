#include "../audio/audio.hpp"

struct Mixer : ::GridUI {
    ::Rect Bounds = { 0, 0, ( WINWIDTH - MIDPOINT ) / 2, MIDPOINT };
    ::Rect& Rect() { return Bounds; }

    bool Active() { return ::GridType == ::GridTypes::Mixer; }

    ::std::vector< ::uint32_t >& GetDisplay() { return ::MixersActive; }
    ::uint32_t* GetImage( ::uint32_t item ) { return ::MixerEntries[ item ].minicover; }

    void GridEnter( ::uint32_t item ) { ::DisplayText = { ::MixerEntries[ item ].name, ::String::WConcat( ::Saved::Mixers[ item ] ) }; }

    bool GridScroll( ::uint32_t item ) {
        if ( HELD( VK_SHIFT ) ) {
            ::SetMixerVolume( item, DIRECTION( VK_SCROLL ) * ( HELD( VK_CONTROL ) ? -0.1 : -0.01 ) );

            return true;
        }

        return false;
    }
} Mixer;