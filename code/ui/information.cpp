#include "../audio/audio.hpp"

struct Information : ::UI {
    bool Active() { return true; }

    ::Rect LastRect = {};
    ::std::vector< ::std::wstring > LastText;

    bool BlockDraw() { return ::DisplayText == LastText || ::UI::Slide != 0; }
    void Draw() {
        int size = LastRect.width() * LastRect.height();
        ::std::memset( ::Canvas + WINWIDTH * WINHEIGHT - size, 0, size * sizeof( ::uint32_t ) );

        LastRect = { 0, WINHEIGHT - FONTHEIGHT * ( int )DisplayText.size(), WINWIDTH, WINHEIGHT };

        for ( int i = 0; i < DisplayText.size(); i++ )
            if ( !::DisplayText[ i ].empty() )
                ::DrawString( 0, LastRect.t + FONTHEIGHT * i, WINWIDTH, ::DisplayText[ i ] );

        LastText = ::DisplayText;
    }
} Information;