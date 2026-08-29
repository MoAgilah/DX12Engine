#include "DX12Renderer.h"

#include "Win32Window.h"
#include <Engine/Core/Constants.h>
#include <Engine/Interface/Renderer/IRenderable.h>
#include <Utilities/Guards.h>
#include <Utilities/HRGuards.h>

#include "DX12RendererImpl.h"
using namespace DirectX;

#include <format>


DX12Renderer::DX12Renderer()
	: m_impl(std::make_unique<DX12RendererImpl>())
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		throw std::invalid_argument("DX12Renderer requires a valid DX12RendererImpl");
}

DX12Renderer::~DX12Renderer()
{
	if (!m_impl)
		return;

	try
	{
		if (m_impl->m_device &&
			m_impl->m_commandQueue &&
			m_impl->m_fence)
		{
			FlushCommandQueue();
		}
	}
	catch (...)
	{
		// Destructor must not throw.
	}

	if (m_impl->m_fenceEvent)
	{
		CloseHandle(m_impl->m_fenceEvent);
		m_impl->m_fenceEvent = nullptr;
	}
}

bool DX12Renderer::Initialise(const Vector2f& screenDims, const std::string& title)
{
	m_window = std::make_shared<Win32Window>();
	if (!CheckNotNull(m_window.get(), "Invalid Pointer 'm_window'"))
		return false;

	ThrowIfFalse(m_window->Create(screenDims, title), "Win32Window::Create Failed.");

	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	m_impl->m_hwnd = static_cast<HWND>(m_window->GetNativeHandle());
	if (!CheckNotNull(m_impl->m_hwnd, "Invalid Handle 'm_impl->m_hwnd'"))
		return false;

	return IntialiseDX12(screenDims);
}

void DX12Renderer::PollWindowEvents()
{
	if (!CheckNotNull(m_window.get(), "Invalid Pointer 'm_window'"))
		return;

	m_window->PollEvents();
}

void DX12Renderer::Clear()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return;

	if (!CheckNotNull(m_impl->m_cmdListAlloc.Get(), "Invalid Pointer 'm_impl->m_cmdListAlloc'"))
		return;

	ThrowIfHResultFailed(m_impl->m_cmdListAlloc->Reset(), "ID3D12CommandAllocator::Reset Failed.");

	if (!CheckNotNull(m_impl->m_commandList.Get(), "Invalid Pointer 'm_impl->m_commandList'"))
		return;

	ThrowIfHResultFailed(
		m_impl->m_commandList->Reset(m_impl->m_cmdListAlloc.Get(), nullptr),
		"ID3D12GraphicsCommandList::Reset Failed."
	);

	// Viewport + scissor MUST be set after Reset
	m_impl->m_commandList->RSSetViewports(1, &m_impl->m_screenViewport);

	m_impl->m_commandList->RSSetScissorRects(1, &m_impl->m_scissorRect);

	if (!CheckNotNull(m_impl->m_rtvHeap.Get(), "Invalid Pointer 'm_impl->m_rtvHeap'"))
		return;

	if (!CheckNotNull(m_impl->m_dsvHeap.Get(), "Invalid Pointer 'm_impl->m_dsvHeap'"))
		return;

	if (!CheckNotNull(
		m_impl->m_swapChainBuffer[m_impl->m_currBackBuffer].Get(),
		std::format("Invalid Pointer 'm_impl->m_swapChainBuffer[{}]'", m_impl->m_currBackBuffer)))
		return;

	// Compute RTV handle for current back buffer
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = m_impl->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	rtv.ptr += SIZE_T(m_impl->m_currBackBuffer) * SIZE_T(m_impl->m_rtvDescriptorSize);

	// Transition: PRESENT -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER bbBarrier = {};
	bbBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bbBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	bbBarrier.Transition.pResource =
		m_impl->m_swapChainBuffer[m_impl->m_currBackBuffer].Get();

	bbBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	bbBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	bbBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_impl->m_commandList->ResourceBarrier(1, &bbBarrier);

	// Bind RT + DSV
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = m_impl->m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_impl->m_commandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);

	// Clear render target + depth/stencil
	const float clearColor[4] =
	{
		0.0f,
		0.0f,
		1.0f,
		1.0f
	};

	m_impl->m_commandList->ClearRenderTargetView(rtv, clearColor, 0,
		nullptr);

	m_impl->m_commandList->ClearDepthStencilView(
		dsv,
		D3D12_CLEAR_FLAG_DEPTH |
		D3D12_CLEAR_FLAG_STENCIL,
		1.0f,
		0,
		0,
		nullptr);
}

void DX12Renderer::Draw(IRenderable* object)
{
}

void DX12Renderer::Draw(IRenderable* object, IShader* shader)
{
}

void DX12Renderer::Present()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return;

	if (!CheckNotNull(m_impl->m_commandList.Get(), "Invalid Pointer 'm_impl->m_commandList'"))
		return;

	if (!CheckNotNull(
		m_impl->m_swapChainBuffer[m_impl->m_currBackBuffer].Get(),
		std::format("Invalid Pointer 'm_impl->m_swapChainBuffer[{}]'", m_impl->m_currBackBuffer)))
		return;

	D3D12_RESOURCE_BARRIER bbBarrier = {};
	bbBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	bbBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	bbBarrier.Transition.pResource =
		m_impl->m_swapChainBuffer[m_impl->m_currBackBuffer].Get();
	bbBarrier.Transition.StateBefore =
		D3D12_RESOURCE_STATE_RENDER_TARGET;
	bbBarrier.Transition.StateAfter =
		D3D12_RESOURCE_STATE_PRESENT;
	bbBarrier.Transition.Subresource =
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_impl->m_commandList->ResourceBarrier(1, &bbBarrier);

	ThrowIfHResultFailed(m_impl->m_commandList->Close(), "ID3D12GraphicsCommandList::Close Failed.");

	ID3D12CommandList* lists[] =
	{
		m_impl->m_commandList.Get()
	};

	if (!CheckNotNull(m_impl->m_commandQueue.Get(), "Invalid Pointer 'm_impl->m_commandQueue'"))
		return;

	m_impl->m_commandQueue->ExecuteCommandLists(1, lists);

	if (!CheckNotNull(m_impl->m_swapChain.Get(), "Invalid Pointer 'm_impl->m_swapChain'"))
		return;

	ThrowIfHResultFailed(m_impl->m_swapChain->Present(1, 0), "IDXGISwapChain::Present Failed.");

	m_impl->m_currBackBuffer = (m_impl->m_currBackBuffer + 1) % m_impl->s_swapChainBufferCount;

	m_impl->m_currentFence++;

	if (!CheckNotNull(m_impl->m_fence.Get(), "Invalid Pointer 'm_impl->m_fence'"))
		return;

	ThrowIfHResultFailed(
		m_impl->m_commandQueue->Signal(m_impl->m_fence.Get(), m_impl->m_currentFence),
		"ID3D12CommandQueue::Signal Failed."
	);

	if (m_impl->m_fence->GetCompletedValue() < m_impl->m_currentFence)
	{
		if (!CheckNotNull(m_impl->m_fenceEvent, "Invalid Handle 'm_impl->m_fenceEvent'"))
			return;

		ThrowIfHResultFailed(
			m_impl->m_fence->SetEventOnCompletion(m_impl->m_currentFence, m_impl->m_fenceEvent),
			"ID3D12Fence::SetEventOnCompletion Failed."
		);

		const DWORD waitResult = WaitForSingleObject(m_impl->m_fenceEvent, INFINITE);

		ThrowIfFalse(waitResult != WAIT_FAILED, "WaitForSingleObject Failed.");
	}
}

bool DX12Renderer::IntialiseDX12(const Vector2f& screenDims)
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	HRESULT hr;

#if defined(DEBUG) || defined(_DEBUG)

	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_impl->m_debugController));
	if (!CheckHResult(hr, "D3D12GetDebugInterface Failed."))
		return false;

	if (!CheckNotNull(m_impl->m_debugController.Get(), "Invalid Pointer 'm_impl->m_debugController'"))
		return false;

	m_impl->m_debugController->EnableDebugLayer();

#endif

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_impl->m_dxgiFactory));
	if (!CheckHResult(hr, "CreateDXGIFactory1 Failed."))
		return false;

	ThrowIfFalse(CreateDX12Device(), "DX12Renderer::CreateDX12Device Failed.");

	if (!CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'"))
		return false;

	hr = m_impl->m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_impl->m_fence));
	if (!CheckHResult(hr, "ID3D12Device::CreateFence Failed."))
		return false;

	m_impl->m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!CheckNotNull(m_impl->m_fenceEvent, "CreateEvent failed."))
		return false;

	m_impl->m_rtvDescriptorSize = m_impl->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_impl->m_dsvDescriptorSize = m_impl->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_impl->m_cbvSrvUavDescriptorSize = m_impl->m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;

	hr = m_impl->m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels, sizeof(msQualityLevels));
	if (!CheckHResult(hr, "ID3D12Device::CheckFeatureSupport Failed."))
		return false;

	m_impl->m_msaaQuality = msQualityLevels.NumQualityLevels;

	assert(m_impl->m_msaaQuality > 0 && "Unexpected MSAA quality level.");

	ThrowIfFalse(CreateCommandQueueAndList(), "DX12Renderer::CreateCommandQueueAndList Failed.");

	ThrowIfFalse(CreateSwapChain(screenDims), "DX12Renderer::CreateSwapChain Failed.");

	ThrowIfFalse(CreateDescriptorHeaps(), "DX12Renderer::CreateDescriptorHeaps Failed.");

	ThrowIfFalse(CreateRenderTargetView(), "DX12Renderer::CreateRenderTargetView Failed.");

	ThrowIfFalse(CreateDepthStencilBuffer(screenDims), "DX12Renderer::CreateDepthStencilBuffer Failed.");

	UpdateViewportAndScissor(screenDims);

	if (!CheckNotNull(m_impl->m_commandList.Get(), "Invalid Pointer 'm_impl->m_commandList'"))
		return false;

	// Close + execute upload commands
	if (!CheckHResult(m_impl->m_commandList->Close(), "ID3D12GraphicsCommandList::Close failed (DX12Renderer::IntialiseDX12)."))
		return false;

	ID3D12CommandList* lists[] = { m_impl->m_commandList.Get() };

	if (!CheckNotNull(m_impl->m_commandQueue.Get(), "Invalid Pointer 'm_impl->m_commandQueue'"))
		return false;

	m_impl->m_commandQueue->ExecuteCommandLists(1, lists);

	FlushCommandQueue();

	return true;
}

bool DX12Renderer::CreateDX12Device()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	HRESULT hr = D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_impl->m_device));
	if (!CheckHResult(hr, "D3D12CreateDevice Failed. Fallback to warp device"))
	{
		if (!CheckNotNull(m_impl->m_dxgiFactory.Get(), "Invalid Pointer 'm_impl->m_dxgiFactory'"))
			return false;

		Microsoft::WRL::ComPtr<IDXGIAdapter> pWarpAdapter;
		hr = m_impl->m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter));
		if (!CheckHResult(hr, "IDXGIFactory4::EnumWarpAdapter Failed."))
			return false;

		if (!CheckNotNull(pWarpAdapter.Get(), "Invalid Pointer 'pWarpAdapter'"))
			return false;

		hr = D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_impl->m_device));
		if (!CheckHResult(hr, "D3D12CreateDevice Failed. With warp device"))
			return false;
	}

	return CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'");
}

bool DX12Renderer::CreateCommandQueueAndList()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	if (!CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'"))
		return false;

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

	HRESULT hr = m_impl->m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_impl->m_commandQueue));
	if (!CheckHResult(hr, "ID3D12Device::CreateCommandQueue Failed."))
		return false;

	hr = m_impl->m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_impl->m_cmdListAlloc.GetAddressOf()));
	if (!CheckHResult(hr, "ID3D12Device::CreateCommandAllocator Failed."))
		return false;

	hr = m_impl->m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_impl->m_cmdListAlloc.Get(), // Associated command allocator
		nullptr, // Initial PipelineStateObject
		IID_PPV_ARGS(m_impl->m_commandList.GetAddressOf()));

	if (!CheckHResult(hr, "ID3D12Device::CreateCommandList Failed."))
		return false;

	if (!CheckNotNull(m_impl->m_commandList.Get(), "Invalid Pointer 'm_impl->m_commandList'"))
		return false;

	// Start off in a closed state.  This is because the first time we refer
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	if (!CheckHResult(m_impl->m_commandList->Close(), "ID3D12GraphicsCommandList::Close Failed."))
		return false;

	return true;
}

bool DX12Renderer::CreateSwapChain(const Vector2f& screenDims)
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	if (!CheckNotNull(m_impl->m_hwnd, "Invalid Handle 'm_impl->m_hwnd'"))
		return false;

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.Width = static_cast<UINT>(screenDims.x); // use window's client area dims
	sd.BufferDesc.Height = static_cast<UINT>(screenDims.y);
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Use 4X MSAA?
	if (m_impl->m_enable4xMsaa)
	{
		sd.SampleDesc.Count = 4;
		// m4xMsaaQuality is returned via CheckMultisampleQualityLevels().
		sd.SampleDesc.Quality = m_impl->m_msaaQuality - 1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
	}

	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = m_impl->s_swapChainBufferCount;
	sd.OutputWindow = m_impl->m_hwnd;
	sd.Windowed = true;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = 0;

	if (!CheckNotNull(m_impl->m_dxgiFactory.Get(), "Invalid Pointer 'm_impl->m_dxgiFactory'"))
		return false;

	if (!CheckNotNull(m_impl->m_commandQueue.Get(), "Invalid Pointer 'm_impl->m_commandQueue'"))
		return false;

	HRESULT hr = m_impl->m_dxgiFactory->CreateSwapChain(m_impl->m_commandQueue.Get(), &sd, m_impl->m_swapChain.GetAddressOf());
	if (!CheckHResult(hr, "IDXGIFactory::CreateSwapChain Failed."))
		return false;

	return true;
}

bool DX12Renderer::CreateDescriptorHeaps()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	if (!CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'"))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = m_impl->s_swapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	HRESULT hr = m_impl->m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_impl->m_rtvHeap.GetAddressOf()));
	if (!CheckHResult(hr, "ID3D12Device::CreateDescriptorHeap Failed. For rtvHeap."))
		return false;

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	hr = m_impl->m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_impl->m_dsvHeap.GetAddressOf()));
	if (!CheckHResult(hr, "ID3D12Device::CreateDescriptorHeap Failed. For dsvHeap."))
		return false;

	return true;
}

bool DX12Renderer::CreateRenderTargetView()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	if (!CheckNotNull(m_impl->m_rtvHeap.Get(), "Invalid Pointer 'm_impl->m_rtvHeap'"))
		return false;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle = m_impl->m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

	HRESULT hr;

	if (!CheckNotNull(m_impl->m_swapChain.Get(), "Invalid Pointer 'm_impl->m_swapChain'"))
		return false;

	if (!CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'"))
		return false;

	for (UINT i = 0; i < m_impl->s_swapChainBufferCount; i++)
	{
		hr = m_impl->m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_impl->m_swapChainBuffer[i]));

		if (!CheckHResult(hr, std::format("IDXGISwapChain::GetBuffer Failed. For index {}", i)))
			return false;

		if (!CheckNotNull(m_impl->m_swapChainBuffer[i].Get(), std::format("Invalid Pointer 'm_impl->m_swapChainBuffer[{}]'", i)))
			return false;

		m_impl->m_device->CreateRenderTargetView(m_impl->m_swapChainBuffer[i].Get(), nullptr, rtvHeapHandle);

		rtvHeapHandle.ptr += m_impl->m_rtvDescriptorSize;
	}

	return true;
}

bool DX12Renderer::CreateDepthStencilBuffer(const Vector2f& screenDims)
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return false;

	D3D12_RESOURCE_DESC dsd;
	dsd.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	dsd.Alignment = 0;
	dsd.Width = static_cast<UINT64>(screenDims.x);
	dsd.Height = static_cast<UINT64>(screenDims.y);
	dsd.MipLevels = 1;
	dsd.DepthOrArraySize = 1;
	dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if (m_impl->m_enable4xMsaa)
	{
		dsd.SampleDesc.Count = 4;
		dsd.SampleDesc.Quality = m_impl->m_msaaQuality - 1;
	}
	// No MSAA
	else
	{
		dsd.SampleDesc.Count = 1;
		dsd.SampleDesc.Quality = 0;
	}

	dsd.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	dsd.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	if (!CheckNotNull(m_impl->m_device.Get(), "Invalid Pointer 'm_impl->m_device'"))
		return false;

	HRESULT hr = m_impl->m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&dsd,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_impl->m_depthStencilBuffer.GetAddressOf()));

	if (!CheckHResult(hr, "ID3D12Device::CreateCommittedResource Failed."))
		return false;

	if (!CheckNotNull(m_impl->m_depthStencilBuffer.Get(), "Invalid Pointer 'm_impl->m_depthStencilBuffer'"))
		return false;

	if (!CheckNotNull(m_impl->m_dsvHeap.Get(), "Invalid Pointer 'm_impl->m_dsvHeap'"))
		return false;

	// Create descriptor to mip level 0 of entire resource using the
	// format of the resource.

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle = m_impl->m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	m_impl->m_device->CreateDepthStencilView(m_impl->m_depthStencilBuffer.Get(), nullptr, dsvHeapHandle);

	if (!CheckNotNull(m_impl->m_commandList.Get(), "Invalid Pointer 'm_impl->m_commandList'"))
		return false;

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

	barrier.Transition.pResource = m_impl->m_depthStencilBuffer.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	m_impl->m_commandList->ResourceBarrier(1, &barrier);

	m_impl->m_dsDesc.DepthEnable = TRUE;
	m_impl->m_dsDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	m_impl->m_dsDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	m_impl->m_dsDesc.StencilEnable = FALSE;
	m_impl->m_dsDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	m_impl->m_dsDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

	return true;
}

void DX12Renderer::UpdateViewportAndScissor(const Vector2f& screenDims)
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return;

	m_impl->m_screenViewport.TopLeftX = 0.0f;
	m_impl->m_screenViewport.TopLeftY = 0.0f;
	m_impl->m_screenViewport.Width = screenDims.x;
	m_impl->m_screenViewport.Height = screenDims.y;
	m_impl->m_screenViewport.MinDepth = 0.0f;
	m_impl->m_screenViewport.MaxDepth = 1.0f;

	m_impl->m_scissorRect.left = 0;
	m_impl->m_scissorRect.top = 0;
	m_impl->m_scissorRect.right = static_cast<LONG>(screenDims.x);
	m_impl->m_scissorRect.bottom = static_cast<LONG>(screenDims.y);
}

void DX12Renderer::FlushCommandQueue()
{
	if (!CheckNotNull(m_impl.get(), "Invalid Pointer 'm_impl'"))
		return;

	m_impl->m_currentFence++;

	if (!CheckNotNull(m_impl->m_commandQueue.Get(), "Invalid Pointer 'm_impl->m_commandQueue'"))
		return;

	if (!CheckNotNull(m_impl->m_fence.Get(), "Invalid Pointer 'm_impl->m_fence'"))
		return;

	ThrowIfHResultFailed(m_impl->m_commandQueue->Signal(m_impl->m_fence.Get(), m_impl->m_currentFence), "ID3D12CommandQueue::Signal Failed (DX12Renderer::FlushCommandQueue).");

	if (m_impl->m_fence->GetCompletedValue() < m_impl->m_currentFence)
	{
		if (!CheckNotNull(m_impl->m_fenceEvent, "Invalid Handle 'm_impl->m_fenceEvent'"))
			return;

		ThrowIfHResultFailed(m_impl->m_fence->SetEventOnCompletion(m_impl->m_currentFence, m_impl->m_fenceEvent), "ID3D12Fence::SetEventOnCompletion Failed (DX12Renderer::FlushCommandQueue).");

		const DWORD waitResult = WaitForSingleObject(m_impl->m_fenceEvent, INFINITE);

		ThrowIfFalse(waitResult != WAIT_FAILED, "WaitForSingleObject Failed.");
	}
}
