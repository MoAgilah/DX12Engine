#pragma once

#include <wrl/client.h>

/////////////
// LINKING //
/////////////
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

//////////////
// INCLUDES //
//////////////
#include <d3d12.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <dxgi1_4.h>

struct DX12RendererImpl
{
    HWND m_hwnd = nullptr;

#if defined(DEBUG) || defined(_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12Debug> m_debugController;
#endif

    Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgiFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;

    HANDLE m_fenceEvent = nullptr;
    UINT64 m_currentFence = 0;

    UINT m_rtvDescriptorSize = 0;
    UINT m_dsvDescriptorSize = 0;
    UINT m_cbvSrvUavDescriptorSize = 0;

    bool m_enable4xMsaa = false;
    UINT m_msaaQuality = 0;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;

    static constexpr int s_swapChainBufferCount = 2;
    int m_currBackBuffer = 0;
    Microsoft::WRL::ComPtr<IDXGISwapChain> m_swapChain;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffer[s_swapChainBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

    D3D12_DEPTH_STENCIL_DESC m_dsDesc{};
    D3D12_VIEWPORT m_screenViewport{};
    D3D12_RECT m_scissorRect{};
};