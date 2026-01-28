#include "ui.hpp"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <dcomp.h>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "dcomp.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

#define SAFE_RELEASE( p ) do { if ( p ) { p->Release(); p = nullptr; } } while ( 0 )

::ID3D11Device* Device = nullptr;
::ID3D11Buffer* Buffer = nullptr;
::IDXGISwapChain1* Swap = nullptr;
::ID3D11Texture2D* Texture = nullptr;
::ID3D11PixelShader* PShader = nullptr;
::ID3D11InputLayout* Layout = nullptr;
::ID3D11VertexShader* VShader = nullptr;
::ID3D11SamplerState* Sampler = nullptr;
::IDCompositionDevice* DCDevice = nullptr;
::IDCompositionTarget* DCTarget = nullptr;
::IDCompositionVisual* Root = nullptr;
::IDCompositionVisual* D3D = nullptr;
::ID3D11DeviceContext* Context = nullptr;
::ID3D11RenderTargetView* Target = nullptr;
::ID3D11ShaderResourceView* Resource = nullptr;

struct Vertex {
    float pos[ 2 ];
    float uv[ 2 ];
};

const char* g_vsCode = R"(
    struct VS_INPUT {
        float2 pos : POSITION;
        float2 uv : TEXCOORD;
    };

    struct PS_INPUT {
        float4 pos : SV_POSITION;
        float2 uv : TEXCOORD;
    };

    PS_INPUT main( VS_INPUT input ) {
        PS_INPUT output;
        output.pos = float4( input.pos, 0.0f, 1.0f );
        output.uv = input.uv;
        return output;
    }
)";

const char* g_psCode = R"(
    Texture2D tex : register( t0 );
    SamplerState samp : register( s0 );

    struct PS_INPUT {
        float4 pos : SV_POSITION;
        float2 uv : TEXCOORD;
    };

    float4 main( PS_INPUT input ) : SV_TARGET {
        return tex.Sample( samp, input.uv );
    }
)";

::HRESULT InitD3D11() {
    ::D3D_FEATURE_LEVEL feature[] = { ::D3D_FEATURE_LEVEL_11_0 };

    HR( ::D3D11CreateDevice( nullptr, ::D3D_DRIVER_TYPE_HARDWARE, nullptr, ::D3D11_CREATE_DEVICE_BGRA_SUPPORT, feature, 1, D3D11_SDK_VERSION, &::Device, nullptr, &::Context ) );

    ::IDXGIFactory2* factory = nullptr;
    ::IDXGIDevice* ddevice = nullptr;
    ::IDXGIAdapter* adapter = nullptr;

    HR( ::Device->QueryInterface( __uuidof( ::IDXGIDevice ), ( void** )&ddevice ) );
    HR( ddevice->GetAdapter( &adapter ) );
    ddevice->Release();
    HR( adapter->GetParent( __uuidof( ::IDXGIFactory2 ), ( void** )&factory ) );
    adapter->Release();

    ::DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = WINWIDTH;
    desc.Height = WINHEIGHT;
    desc.Format = ::DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.Stereo = FALSE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.Scaling = ::DXGI_SCALING_STRETCH;
    desc.SwapEffect = ::DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode = ::DXGI_ALPHA_MODE_PREMULTIPLIED;
    desc.Flags = ::DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

    HR( factory->CreateSwapChainForComposition( ::Device, &desc, nullptr, &::Swap ) );
    factory->Release();

    ::ID3D11Texture2D* bbuffer = nullptr;
    HR( ::Swap->GetBuffer( 0, __uuidof( ::ID3D11Texture2D ), ( void** )&bbuffer ) );

    HR( ::Device->CreateRenderTargetView( bbuffer, nullptr, &::Target ) );
    bbuffer->Release();

    return S_OK;
}

::HRESULT InitTexture() {
    ::D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = WINWIDTH;
    desc.Height = WINHEIGHT;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = ::DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = ::D3D11_USAGE_DYNAMIC;
    desc.BindFlags = ::D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = ::D3D11_CPU_ACCESS_WRITE;

    HR( ::Device->CreateTexture2D( &desc, nullptr, &::Texture ) );

    ::D3D11_SHADER_RESOURCE_VIEW_DESC rdesc = {};
    rdesc.Format = desc.Format;
    rdesc.ViewDimension = ::D3D11_SRV_DIMENSION_TEXTURE2D;
    rdesc.Texture2D.MipLevels = 1;

    HR( ::Device->CreateShaderResourceView( ::Texture, &rdesc, &::Resource ) );

    ::D3D11_SAMPLER_DESC sdesc = {};
    sdesc.Filter = ::D3D11_FILTER_MIN_MAG_MIP_POINT;
    sdesc.AddressU = ::D3D11_TEXTURE_ADDRESS_CLAMP;
    sdesc.AddressV = ::D3D11_TEXTURE_ADDRESS_CLAMP;
    sdesc.AddressW = ::D3D11_TEXTURE_ADDRESS_CLAMP;
    sdesc.ComparisonFunc = ::D3D11_COMPARISON_NEVER;
    sdesc.MinLOD = 0;
    sdesc.MaxLOD = D3D11_FLOAT32_MAX;

    HR( ::Device->CreateSamplerState( &sdesc, &::Sampler ) );

    return S_OK;
}

::HRESULT InitShaders() {
    ::ID3DBlob* vblob = nullptr;
    ::ID3DBlob* pblob = nullptr;
    ::ID3DBlob* eblob = nullptr;

    HR( ::D3DCompile( ::g_vsCode, ::strlen( ::g_vsCode ), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vblob, &eblob ) );
    HR( ::D3DCompile( ::g_psCode, ::strlen( ::g_psCode ), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pblob, &eblob ) );
    HR( ::Device->CreateVertexShader( vblob->GetBufferPointer(), vblob->GetBufferSize(), nullptr, &::VShader ) );
    HR( ::Device->CreatePixelShader( pblob->GetBufferPointer(), pblob->GetBufferSize(), nullptr, &::PShader ) );

    ::D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, ::DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( ::Vertex, pos ), ::D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, ::DXGI_FORMAT_R32G32_FLOAT, 0, offsetof( ::Vertex, uv ),  ::D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    HR( ::Device->CreateInputLayout( layout, ARRAYSIZE( layout ), vblob->GetBufferPointer(), vblob->GetBufferSize(), &::Layout ) );

    vblob->Release();
    pblob->Release();

    ::Vertex vertices[] = {
        { { -1, -1 }, { 0, 1 } },
        { { -1, 1 }, { 0, 0 } },
        { { 1, -1 }, { 1, 1 } },
        { { 1, -1 }, { 1, 1 } },
        { { -1, 1 }, { 0, 0 } },
        { { 1,  1 }, { 1, 0 } },
    };

    ::D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof( vertices );
    desc.Usage = ::D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = ::D3D11_BIND_VERTEX_BUFFER;

    ::D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = vertices;

    HR( ::Device->CreateBuffer( &desc, &data, &::Buffer ) );

    return S_OK;
}

::HRESULT InitComposition() {
    HR( ::DCompositionCreateDevice( nullptr, __uuidof( ::IDCompositionDevice ), ( void** )&::DCDevice ) );
    HR( ::DCDevice->CreateTargetForHwnd( hwnd, TRUE, &::DCTarget ) );
    HR( ::DCDevice->CreateVisual( &::Root ) );
    HR( ::DCDevice->CreateVisual( &::D3D ) );
    HR( ::D3D->SetContent( ::Swap ) );
    HR( ::Root->AddVisual( ::D3D, FALSE, nullptr ) );

    ::DCTarget->SetRoot( ::Root );

    HR( ::DCDevice->Commit() );

    return S_OK;
}

::HRESULT InitFrame() {
    ::D3D11_VIEWPORT vp = { 0.0f, 0.0f, WINWIDTH, WINHEIGHT, 0.0f, 1.0f };
    ::Context->RSSetViewports( 1, &vp );

    ::UINT stride = sizeof( ::Vertex );
    ::UINT offset = 0;
    ::Context->IASetVertexBuffers( 0, 1, &::Buffer, &stride, &offset );
    ::Context->IASetInputLayout( ::Layout );
    ::Context->IASetPrimitiveTopology( ::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    ::Context->VSSetShader( ::VShader, nullptr, 0 );
    ::Context->PSSetShader( ::PShader, nullptr, 0 );

    ::Context->PSSetSamplers( 0, 1, &::Sampler );

    ::Context->PSSetShaderResources( 0, 1, &::Resource );

    HR( ::DCDevice->Commit() );

    ::D3D11_MAPPED_SUBRESOURCE map;
    HR( ::Context->Map( ::Texture, 0, ::D3D11_MAP_WRITE_DISCARD, 0, &map ) );

    if ( !( ::Canvas = ( ::uint32_t* )map.pData ) )
        return E_FAIL;

    return S_OK;
}

void Restart() {
    if ( ::Context ) {
        if( ::Texture )
            ::Context->Unmap(Texture, 0);
        ::Context->ClearState();
        ::Context->Flush();
    }

    SAFE_RELEASE( ::Resource );
    SAFE_RELEASE( ::Target );
    SAFE_RELEASE( ::Root );
    SAFE_RELEASE( ::Swap );
    SAFE_RELEASE( ::D3D );
    SAFE_RELEASE( ::DCDevice );
    SAFE_RELEASE( ::DCTarget );
    SAFE_RELEASE( ::Context );
    SAFE_RELEASE( ::Texture );
    SAFE_RELEASE( ::VShader );
    SAFE_RELEASE( ::PShader );
    SAFE_RELEASE( ::Sampler );
    SAFE_RELEASE( ::Device );
    SAFE_RELEASE( ::Layout );
    SAFE_RELEASE( ::Buffer );

    ::InitGraphics();
}

bool RenderFrame() {
    ::Context->OMSetRenderTargets( 1, &::Target, nullptr );
    ::Context->Draw( 6, 0 );

    if ( FAILED( ::Swap->Present( 0, DXGI_PRESENT_ALLOW_TEARING ) ) )
        return false;

    return true;
}

bool CheckSlide() {
    static int last;

    int s = ::UI::Slide;

    if ( last == s )
        return false;

    if ( last < s )
        ::std::memset( ::Canvas, 0, ::CanvasSize );

    last = s;

    return true;
}

::HRESULT InitGraphics() {
    HR( ::InitD3D11() );
    HR( ::InitTexture() );
    HR( ::InitShaders() );
    HR( ::InitComposition() );
    HR( ::InitFrame() );

    for ( auto& i : ::UI::UIs )
        i->Initialize();

    THREAD(
        const auto period = ::std::chrono::duration_cast< ::std::chrono::steady_clock::duration >( ::std::chrono::duration< double >( 1.0 / 240.0 ) );
        ::std::chrono::time_point next = ::std::chrono::steady_clock::now();

        while ( true ) {
            ::std::this_thread::sleep_until( next += period );

            bool draw = false;

            ::UI::PreDraw();

            bool slid = ::CheckSlide();

            for ( ::UI* i : ::UI::UIs ) {
                ::std::lock_guard< ::std::mutex > lock( ::CanvasMutex );

                if ( i->Active() && ( slid || !i->BlockDraw() ) ) {
                    i->Draw();
                    draw = true;
                }
            }

            if ( draw )
                if ( !::RenderFrame() )
                    break;

            ::Frame++;
        }

        ::Frame = 0;
        ::Restart();
    );

    return S_OK;
}