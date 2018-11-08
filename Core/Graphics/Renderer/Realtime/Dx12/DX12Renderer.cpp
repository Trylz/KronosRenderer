//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#include "stdafx.h"

#include "../CreateTextureException.h"
#include "DX12Renderer.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/CameraConstantBuffer.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/MeshGroupConstantBuffer.h"
#include <d3d12.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
using namespace DirectX; 
using namespace Descriptor;

#define MSAA_SAMPLES 4

nbBool DX12Renderer::init(const InitArgs& args)
{
	m_hwindow = args.hwnd;

	RECT rect;
	NEBULA_ASSERT(GetWindowRect(m_hwindow, &rect) > 0);
	const glm::uvec2 bufferSize(rect.right - rect.left, rect.bottom - rect.top);

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));
	if (FAILED(hr))
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create device factory");
		return false;
	}

	if (!createDevice(m_dxgiFactory))
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create device");
		return false;
	}

	// Create descriptor allocators
	m_cbs_srv_uavAllocator = std::make_unique<CBV_SRV_UAV_DescriptorAllocator>();
	m_rtvAllocator = std::make_unique<RTV_DescriptorAllocator>();
	m_dsvAllocator = std::make_unique<DSV_DescriptorAllocator>();

	if (!createCommandQueues())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create direct command queue");
		return false;
	}

	m_msaaSampleDesc.Count = MSAA_SAMPLES;
	m_simpleSampleDesc.Count = 1;

	if (!createSwapChain(m_dxgiFactory, bufferSize))
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create swap chain");
		return false;
	}

	if (!createRenderTargets(bufferSize))
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create msaa render target");
		return false;
	}

	if (!createDepthStencilBuffers(bufferSize))
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create depth stencil buffer");
		return false;
	}

	setUpViewportAndScissor(bufferSize);

	if (!createPixelReadBuffer())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create the pixel read buffer");
		return false;
	}

	if (!bindSwapChainRenderTargets())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to bind swap shain render targets");
		return false;
	}

	if (!createCommandAllocators())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create command allocators");
		return false;
	}

	if (!createCommandLists())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create command list");
		return false;
	}

	if (!createFencesAndFenceEvent())
	{
		NEBULA_TRACE("DX12Renderer::init - Unable to create fence and fence event");
		return false;
	}

	Realtime::GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS> = (TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this;

	// Create effects
	Effect::CameraConstantBufferSingleton::create();
	Effect::MeshGroupConstantBufferSingleton::create();

	m_effectsReady = false;
	startCommandRecording();

	m_compileEffectThread = std::thread([this]{
		m_cubeMappingEffect = std::make_unique<Effect::CubeMapping>(m_msaaSampleDesc);
		m_highlightColorEffect = std::make_unique<Effect::HighlightColor>(m_msaaSampleDesc);
		m_renderLightsEffect = std::make_unique<Effect::RenderLights>(m_msaaSampleDesc);
		m_moveGizmoEffect = std::make_unique<Effect::RenderMoveGizmo>(m_msaaSampleDesc);
		m_scaleGizmoEffect = std::make_unique<Effect::RenderScaleGizmo>(m_msaaSampleDesc);
		m_rotationGizmoEffect = std::make_unique<Effect::RenderRotationGizmo>(m_msaaSampleDesc);
		m_lightVisualLinesEffect = std::make_unique<Effect::LightVisualLines>(m_msaaSampleDesc);
		m_renderPositionEffect = std::make_unique<Effect::RenderWorldPosition>(m_simpleSampleDesc);
		m_forwardLightningEffect = std::make_unique<Effect::ForwardLighning>(m_msaaSampleDesc);

		m_effectsReady = true;

		endCommandRecording();
	});


	return true;
}

void DX12Renderer::finishTasks()
{
	// wait for the gpu to finish all frames
	for (auto& buffer : m_commandBuffers)
	{
		for (nbInt32 i = 0; i < SwapChainBufferCount; ++i)
		{
			buffer.second.frameIndex = i;
			waitCurrentFrameCommandsFinish(buffer.first);
		}
	}
}

void DX12Renderer::release()
{
	// Wait for the compile thread to finish
	if (m_compileEffectThread.joinable())
		m_compileEffectThread.join();

	// close the fence event
	for (auto& buffer : m_commandBuffers)
		CloseHandle(buffer.second.fenceEvent);

	// get swapchain out of full screen before exiting
	BOOL fs = false;
	if (m_swapChain->GetFullscreenState(&fs, NULL))
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	releaseSwapChainDynamicResources();

	Effect::CameraConstantBufferSingleton::destroy();
	Effect::MeshGroupConstantBufferSingleton::destroy();

	D3D12Device->Release();
}

nbBool DX12Renderer::createDevice(IDXGIFactory4* m_dxgiFactory)
{
	HRESULT hr;
	nbBool debugLayerEnabled = false;
	
#if defined(_DEBUG)
	//Enable the debug layer
	CComPtr<ID3D12Debug> d3dDebug;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&d3dDebug))))
	{
		d3dDebug->EnableDebugLayer();
		debugLayerEnabled = true;
	}
#endif

	// adapters are the graphics card (this includes the embedded graphics on the motherboard)
	IDXGIAdapter1* adapter;

	// we'll start looking for directx 12 compatible graphics devices starting at index 0
	nbInt32 adapterIndex = 0;

	// set this to true when a good one was found
	nbBool adapterFound = false;


	// find first hardware gpu that supports d3d 12
	while (m_dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// we dont want a software device
			continue;
		}

		// we want a device that is compatible with direct3d 12 (feature level 11 or higher)
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&D3D12Device)
	);

	if (FAILED(hr))
	{
		return false;
	}

	if (debugLayerEnabled)
	{
		// Disable specific warning messages.
		CComPtr<ID3D12InfoQueue> infoQueue;
		if (SUCCEEDED(D3D12Device->QueryInterface(IID_PPV_ARGS(&infoQueue))))
		{
			D3D12_INFO_QUEUE_FILTER filter = {};

			// Direct use of D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE makes visual studio hangs.
			// @See: D3D12_MESSAGE_ID enum in d3d12sdklayers.h
			D3D12_MESSAGE_ID disabledMessage = (D3D12_MESSAGE_ID)((nbInt32)D3D12_MESSAGE_ID_UNKNOWN + 820);

			filter.DenyList.NumIDs = 1;
			filter.DenyList.pIDList = &disabledMessage;

			D3D12_MESSAGE_SEVERITY infoSev = D3D12_MESSAGE_SEVERITY_INFO;
			filter.DenyList.NumSeverities = 1;
			filter.DenyList.pSeverityList = &infoSev;
			infoQueue->PushStorageFilter(&filter);
		}
	}

	return true;
}

nbBool DX12Renderer::createCommandQueues()
{
	// create the direct command queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;// direct means the gpu can directly execute this command queue

	HRESULT hr = D3D12Device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandBuffers[CommandType::Direct].commandQueue));
	if (FAILED(hr))
		return false;

	return true;
}

nbBool DX12Renderer::createSwapChain(IDXGIFactory4* m_dxgiFactory, const glm::uvec2& bufferSize)
{
	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = bufferSize.x; // buffer width
	backBufferDesc.Height = bufferSize.y; // buffer height
	backBufferDesc.Format = NEBULA_SWAP_CHAIN_FORMAT;

	// Describe and create the swap chain.
	m_swapChainDesc.BufferCount = SwapChainBufferCount; // number of buffers we have
	m_swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
	m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
	m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
	m_swapChainDesc.OutputWindow = m_hwindow; // handle to our window
	m_swapChainDesc.SampleDesc = m_simpleSampleDesc;
	m_swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

	IDXGISwapChain* tempSwapChain;

	HRESULT hr = m_dxgiFactory->CreateSwapChain(
		m_commandBuffers[CommandType::Direct].commandQueue, // the queue will be flushed once the swap chain is created
		&m_swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain
	);

	if (FAILED(hr))
	{
		return false;
	}

	m_swapChain.Attach(static_cast<IDXGISwapChain3*>(tempSwapChain));

	for (auto& buffer : m_commandBuffers)
		buffer.second.frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	return true;
}

nbBool DX12Renderer::bindSwapChainRenderTargets()
{
	// Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
	for (nbInt32 i = 0; i < SwapChainBufferCount; i++)
	{
		// first we get the n'th buffer in the swap chain and store it in the n'th
		// position of our ID3D12Resource array
		HRESULT hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

nbBool DX12Renderer::createCommandAllocators()
{
	for (nbInt32 i = 0; i < SwapChainBufferCount; i++)
	{
		HRESULT hr = D3D12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandBuffers[CommandType::Direct].commandAllocator[i]));
		if (FAILED(hr))
			return false;
	}

	return true;
}

nbBool DX12Renderer::createCommandLists()
{
	auto& directCommandBuffers = m_commandBuffers[CommandType::Direct];

	HRESULT hr = D3D12Device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		directCommandBuffers.commandAllocator[directCommandBuffers.frameIndex],
		NULL,
		IID_PPV_ARGS(&directCommandBuffers.commandList)
	);

	if (FAILED(hr))
		return false;

	directCommandBuffers.commandList->SetName(L"Direct command list");
	directCommandBuffers.commandList->Close();

	return true;
}

nbBool DX12Renderer::createFencesAndFenceEvent()
{
	HRESULT hr;

	for (auto& buffer : m_commandBuffers)
	{
		for (nbInt32 i = 0; i < SwapChainBufferCount; i++)
		{
			hr = D3D12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&buffer.second.fences[i]));
			if (FAILED(hr))
			{
				return false;
			}
			
			// set the initial fence value to 0
			buffer.second.fenceValue[i] = 0;
		}

		// create a handle to a fence event
		buffer.second.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (buffer.second.fenceEvent == nullptr)
		{
			return false;
		}
	}

	return true;
}

nbBool DX12Renderer::createPixelReadBuffer()
{
	D3D12_HEAP_PROPERTIES const heapProperties =
	{
		/*Type*/                    D3D12_HEAP_TYPE_CUSTOM
		/*CPUPageProperty*/         ,D3D12_CPU_PAGE_PROPERTY_WRITE_BACK
		/*MemoryPoolPreference*/    ,D3D12_MEMORY_POOL_L0
		/*CreationNodeMask*/        ,0
		/*VisibleNodeMask*/         ,0
	};

	HRESULT hr = D3D12Device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(NEBULA_WORLD_POSITION_RT_FORMAT, 1, 1, 1, 1, 1),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_pixelReadBuffer)
	);

	return SUCCEEDED(hr);
}

nbBool DX12Renderer::createRenderTargets(const glm::uvec2& bufferSize)
{
	{
		// The msaa render target
		D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Tex2D(NEBULA_SWAP_CHAIN_FORMAT, bufferSize.x, bufferSize.y, 1, 1, MSAA_SAMPLES);
		resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resDesc,
			D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
			nullptr,
			IID_PPV_ARGS(&m_msaaRenderTarget)
		);

		if (FAILED(hr))
			return false;

		m_msaaRTDescriptorHandle = m_rtvAllocator->allocate({ m_msaaRenderTarget });
	}

	{
		// The world position render target
		m_positionRenderTargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(NEBULA_WORLD_POSITION_RT_FORMAT, bufferSize.x, bufferSize.y, 1, 1, 1);
		m_positionRenderTargetDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Format = NEBULA_WORLD_POSITION_RT_FORMAT;

		const nbFloat32 clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		memcpy(optimizedClearValue.Color, clearColor, sizeof(nbFloat32) * 4);

		HRESULT hr = D3D12Device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&m_positionRenderTargetDesc,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			&optimizedClearValue,
			IID_PPV_ARGS(&m_positionRenderTarget)
		);

		if (FAILED(hr))
			return false;

		m_positionRTDescriptorHandle = m_rtvAllocator->allocate({ m_positionRenderTarget });
	}

	return true;
}

nbBool DX12Renderer::createDepthStencilBuffers(const glm::uvec2& bufferSize)
{
	const DXGI_FORMAT format = Effect::defaultDSFormat;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = format;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	HRESULT hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(format, bufferSize.x, bufferSize.y, 1, 1, MSAA_SAMPLES, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);

	if (FAILED(hr))
		return false;
	
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = format;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_dsvDescriptorsHandle = m_dsvAllocator->allocate({ m_depthStencilBuffer }, depthStencilDesc);

	// The world position render depth
	hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(format, bufferSize.x, bufferSize.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_positionDepthStencilBuffer)
	);

	if (FAILED(hr))
		return false;

	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	m_positionDsvDescriptorsHandle = m_dsvAllocator->allocate({ m_positionDepthStencilBuffer }, depthStencilDesc);

	return true;
}

void DX12Renderer::setUpViewportAndScissor(const glm::uvec2& bufferSize)
{
	// Fill out the Viewport
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = (FLOAT)bufferSize.x;
	m_viewport.Height = (FLOAT)bufferSize.y;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	// Fill out a scissor rect
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = (LONG)bufferSize.x;
	m_scissorRect.bottom = (LONG)bufferSize.y;
}

void DX12Renderer::releaseSwapChainDynamicResources()
{
	NEBULA_ASSERT(m_depthStencilBuffer);
	m_depthStencilBuffer->Release();

	NEBULA_ASSERT(m_positionDepthStencilBuffer);
	m_positionDepthStencilBuffer->Release();

	NEBULA_ASSERT(m_msaaRenderTarget);
	m_msaaRenderTarget->Release();

	NEBULA_ASSERT(m_positionRenderTarget);
	m_positionRenderTarget->Release();

	for (nbInt32 i = 0; i < SwapChainBufferCount; i++)
	{
		m_renderTargets[i]->Release();
	}

	m_rtvAllocator->release(m_msaaRTDescriptorHandle);
	m_rtvAllocator->release(m_positionRTDescriptorHandle);
	m_dsvAllocator->release(m_dsvDescriptorsHandle);
	m_dsvAllocator->release(m_positionDsvDescriptorsHandle);
}

void DX12Renderer::resizeBuffers(const glm::uvec2& newSize)
{
	releaseSwapChainDynamicResources();

	HRESULT hr = m_swapChain->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, m_swapChainDesc.Flags);
	NEBULA_ASSERT(SUCCEEDED(hr));

	NEBULA_ASSERT(createRenderTargets(newSize));

	NEBULA_ASSERT(createDepthStencilBuffers(newSize));

	NEBULA_ASSERT(bindSwapChainRenderTargets());

	setUpViewportAndScissor(newSize);
}

void DX12Renderer::createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, Dx12TextureHandle& dst)
{
	const nbUint32 width = images[0]->getWidth();
	const nbUint32 height = images[0]->getHeight();

	if (width != height)
		throw CreateTextureException("Images width and height must be the same.");

	if (width > D3D12_REQ_TEXTURECUBE_DIMENSION)
		throw CreateTextureException("Images width is too high.");
	if (height > D3D12_REQ_TEXTURECUBE_DIMENSION)
		throw CreateTextureException("Images height is too high");

	const Texture::ImageFormat nebulaFormat = images[0]->getFormat();

	for (int i = 1; i < images.size(); ++i)
	{
		if (images[i]->getWidth() != width)
			throw CreateTextureException("All images must have the same width. It is not the case for: " + images[i]->getPath());

		if (images[i]->getHeight() != height)
			throw CreateTextureException("All images must have the same height. It is not the case for: " + images[i]->getPath());

		if (images[i]->getFormat() != nebulaFormat)
			throw CreateTextureException("Images must have the same format");
	}

	createRGBATexture2DArray(images, width, height, D3D12_SRV_DIMENSION_TEXTURECUBE, dst);
}

void DX12Renderer::createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, nbUint32 width, nbUint32 height, D3D12_SRV_DIMENSION viewDimension, Dx12TextureHandle& dst)
{
	const nbUint32 arraySize = (nbUint32)images.size();
	NEBULA_ASSERT(arraySize < D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = static_cast<std::uint16_t>(arraySize); // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	textureDesc.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level

	const Texture::ImageFormat nebulaFormat = images[0]->getFormat();
	if (nebulaFormat == Texture::ImageFormat::RGBA8)
	{
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else
	{
		NEBULA_ASSERT(nebulaFormat == Texture::ImageFormat::RGBA32F);
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	}

	textureDesc.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	textureDesc.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	// create a default heap.
	HRESULT hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&textureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&dst.buffer));

	NEBULA_ASSERT(SUCCEEDED(hr));

	dst.format = textureDesc.Format;

	std::string bufferDebugString = "Texture Resource Heap images first = " + images[0]->getPath() + ", size = " + std::to_string(arraySize);
	std::wstring bufferDebugStringW(bufferDebugString.begin(), bufferDebugString.end());
	dst.buffer->SetName(bufferDebugStringW.c_str());

	UINT64 textureUploadBufferSize;
	D3D12Device->GetCopyableFootprints(&textureDesc, 0, arraySize, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// now we create an upload heap to upload our texture to the GPU
	ID3D12Resource* textureBufferUploadHeap;

	hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));

	NEBULA_ASSERT(SUCCEEDED(hr));

	std::string uploadDebugString = "Texture Upload Resource Heap images first = " + images[0]->getPath() + ", size = " + std::to_string(arraySize);
	std::wstring uploadDebugStringW(uploadDebugString.begin(), uploadDebugString.end());

	textureBufferUploadHeap->SetName(uploadDebugStringW.c_str());

	// Initialize subresources
	std::vector<D3D12_SUBRESOURCE_DATA> subResourceData(arraySize);
	for (nbUint32 i = 0u; i < arraySize; ++i)
	{
		subResourceData[i].pData = images[i]->getRawData();
		subResourceData[i].RowPitch = images[i]->getBytesPerRow();
		subResourceData[i].SlicePitch = images[i]->getBytesPerRow() * height;
	}

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(m_commandBuffers[CommandType::Direct].commandList, dst.buffer, textureBufferUploadHeap, 0, 0, arraySize, &subResourceData[0]);

	m_commandBuffers[CommandType::Direct].commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));

	m_loadingResources.push_back(textureBufferUploadHeap);

	// Init descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Format = dst.format;
	srvDesc.Texture2D.MipLevels = 1;
	dst.descriptorHandle = m_cbs_srv_uavAllocator->allocate({ dst.buffer }, srvDesc);
}

nbBool DX12Renderer::createIndexBuffer(const std::vector<nbUint32>& data, Dx12IndexBufferHandle& dst)
{
	ArrayBufferResource resourceData = createArrayBufferRecource(data.data(), sizeof(nbUint32), (nbUint32)data.size());
	if (resourceData.bufferSize == 0u || resourceData.buffer == nullptr)
	{
		return false;
	}

	dst.buffer = resourceData.buffer;
	dst.count = (nbUint32)data.size();
	dst.bufferView.BufferLocation = dst.buffer->GetGPUVirtualAddress();
	dst.bufferView.Format = DXGI_FORMAT_R32_UINT;
	dst.bufferView.SizeInBytes = resourceData.bufferSize;

	return true;
}

DX12Renderer::ArrayBufferResource DX12Renderer::createArrayBufferRecource(const void* data, nbUint32 sizeofElem, nbUint32 count)
{
	ArrayBufferResource retData;

	retData.bufferSize = sizeofElem * count;

	HRESULT hr;

	// create default heap
	hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&retData.buffer));

	NEBULA_ASSERT(SUCCEEDED(hr));

	// create upload heap
	ID3D12Resource* vBufferUploadHeap;
	hr = D3D12Device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));

	NEBULA_ASSERT(SUCCEEDED(hr));

	// store buffer data in upload heap
	D3D12_SUBRESOURCE_DATA bufferData = {};
	bufferData.pData = reinterpret_cast<BYTE*>(const_cast<void*>(data));
	bufferData.RowPitch = retData.bufferSize;
	bufferData.SlicePitch = retData.bufferSize;

	// we are now creating a command with the command list to copy the data from the upload heap to the default heap
	UpdateSubresources(m_commandBuffers[CommandType::Direct].commandList, retData.buffer, vBufferUploadHeap, 0, 0, 1, &bufferData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_commandBuffers[CommandType::Direct].commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(retData.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_loadingResources.push_back(vBufferUploadHeap);

	return retData;
}

void DX12Renderer::tryWaitEffectCompilationDone()
{
	if (!m_effectsReady)
	{
		if (m_compileEffectThread.joinable())
			m_compileEffectThread.join();
	}
}

Scene::GizmoMap DX12Renderer::createGizmos() const
{
	Scene::GizmoMap gizmos;

	gizmos[Gizmo::GizmoType::Move] = std::make_unique<Effect::DX12MoveGizmo>();
	gizmos[Gizmo::GizmoType::Rotation] = std::make_unique<Effect::DX12RotationGizmo>();
	gizmos[Gizmo::GizmoType::Scale] = std::make_unique<Effect::DX12ScaleGizmo>();

	return gizmos;
}

void DX12Renderer::startCommandRecording()
{
	tryWaitEffectCompilationDone();
	startCommandRecording(CommandType::Direct);
}

void DX12Renderer::endCommandRecording()
{
	endCommandRecording(CommandType::Direct);

	if (m_loadingResources.size())
	{
		auto& commandBuffers = m_commandBuffers[CommandType::Direct];
		commandBuffers.fenceValue[commandBuffers.frameIndex]++;

		waitCurrentFrameCommandsFinish(CommandType::Direct);

		for (auto resource : m_loadingResources)
			resource->Release();

		m_loadingResources.clear();
	}
}

void DX12Renderer::endSceneLoadCommandRecording(const Scene::BaseScene* scene)
{
	if (scene)
	{
		Effect::MeshGroupConstantBufferSingleton::instance()->onNewScene(*scene, m_commandBuffers[CommandType::Direct].commandList);
		m_forwardLightningEffect->onNewScene(*scene, m_commandBuffers[CommandType::Direct].commandList);
		m_renderLightsEffect->onNewScene(*scene);
	}

	endCommandRecording();
}

void DX12Renderer::startCommandRecording(CommandType commandType)
{
	waitCurrentFrameCommandsFinish(commandType);

	auto& commandBuffers = m_commandBuffers[commandType];

	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	const nbInt32 frameIdx = commandBuffers.frameIndex;

	HRESULT hr = commandBuffers.commandAllocator[frameIdx]->Reset();
	if (FAILED(hr))
	{
		NEBULA_TRACE("DX12Renderer::startCommandRecording - Failed to reset command allocator index = " << frameIdx);
		return;
	}

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter.
	hr = commandBuffers.commandList->Reset(commandBuffers.commandAllocator[frameIdx], nullptr);
	NEBULA_ASSERT(SUCCEEDED(hr));
}

void DX12Renderer::endCommandRecording(CommandType commandType)
{
	auto& commandBuffers = m_commandBuffers[commandType];

	HRESULT hr = commandBuffers.commandList->Close();
	NEBULA_ASSERT(SUCCEEDED(hr));

	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { commandBuffers.commandList };

	// execute the array of command lists
	commandBuffers.commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void DX12Renderer::waitCurrentFrameCommandsFinish(CommandType commandType)
{
	auto& commandBuffers = m_commandBuffers[commandType];

	const nbInt32 frameIdx = (commandType == CommandType::Direct) ? m_swapChain->GetCurrentBackBufferIndex()
		: ++commandBuffers.frameIndex % SwapChainBufferCount;

	commandBuffers.frameIndex = frameIdx;

	waitCommandsFinish(commandType, frameIdx);
}

void DX12Renderer::waitCommandsFinish(CommandType commandType, nbInt32 frameIdx)
{
	auto& commandBuffers = m_commandBuffers[commandType];

	const UINT64 fence = commandBuffers.fenceValue[frameIdx];
	HRESULT hr = commandBuffers.commandQueue->Signal(commandBuffers.fences[frameIdx], fence);

	// Wait until GPU finish with command queue.
	if (commandBuffers.fences[frameIdx]->GetCompletedValue() < fence)
	{
		hr = commandBuffers.fences[frameIdx]->SetEventOnCompletion(fence, commandBuffers.fenceEvent);
		NEBULA_ASSERT(SUCCEEDED(hr));

		WaitForSingleObject(commandBuffers.fenceEvent, INFINITE);
	}

	NEBULA_ASSERT(SUCCEEDED(hr));
	commandBuffers.fenceValue[frameIdx]++;
}

IntersectionInfo DX12Renderer::queryIntersection(const Scene::BaseScene& scene, const glm::uvec2& pos)
{
	{
		// Start direct command recording.
		// Will wait for effect compilation to be done if required.
		startCommandRecording();

		auto& commandList = m_commandBuffers[CommandType::Direct].commandList;
		const nbInt32 frameIdx = m_commandBuffers[CommandType::Direct].frameIndex;

		// Update the camera constant buffer
		Effect::CameraConstantBufferSingleton::instance()->update(scene, commandList, frameIdx);

		// First render the positions
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_positionRenderTarget, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

		const nbFloat32 clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		commandList->ClearRenderTargetView(m_positionRTDescriptorHandle.getCpuHandle(), clearColor, 0, nullptr);
		commandList->ClearDepthStencilView(m_positionDsvDescriptorsHandle.getCpuHandle(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		commandList->OMSetRenderTargets(1, &m_positionRTDescriptorHandle.getCpuHandle(), FALSE, &m_positionDsvDescriptorsHandle.getCpuHandle());
		commandList->RSSetViewports(1, &m_viewport);
		commandList->RSSetScissorRects(1, &m_scissorRect);

		ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbs_srv_uavAllocator->getHeap() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		{
			Effect::RenderWorldPositionPushArgs args = { scene };
			m_renderPositionEffect->pushDrawCommands(args, commandList, frameIdx);
		}

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_positionRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));

		// Copy texture pixel
		CD3DX12_TEXTURE_COPY_LOCATION dest(m_pixelReadBuffer, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX);
		CD3DX12_TEXTURE_COPY_LOCATION src(m_positionRenderTarget, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX);

		CD3DX12_BOX box(pos.x, pos.y, pos.x + 1, pos.y + 1);
		commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, &box);

		endCommandRecording();

		waitCommandsFinish(CommandType::Direct, frameIdx);
	}

	// Now read
	float rawData[4];
	m_pixelReadBuffer->ReadFromSubresource(reinterpret_cast<void*>(&rawData), 0, 0, 0, nullptr);

	nbUint32 flatMeshIdx = (nbUint32)rawData[3];

	if (flatMeshIdx == 0)
		return IntersectionInfo{};

	--flatMeshIdx;

	auto& flatMeshes = scene.getModel()->getFlatMeshes();
	if (flatMeshIdx >= flatMeshes.size())
		return IntersectionInfo{};

	IntersectionInfo isectInfo;
	isectInfo.object = flatMeshes[flatMeshIdx];
	isectInfo.worldPos = glm::vec3(rawData[0], rawData[1], rawData[2]);

	return isectInfo;
}

void DX12Renderer::drawScene(const Scene::BaseScene& scene)
{
	auto& commandList = m_commandBuffers[CommandType::Direct].commandList;
	const nbInt32 frameIdx = m_commandBuffers[CommandType::Direct].frameIndex;

	// Update the camera constant buffer
	Effect::CameraConstantBufferSingleton::instance()->update(scene, commandList, frameIdx);

	// transition the MSAA RT from the resolve source to the render target state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_msaaRenderTarget, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the render target
	const RGBColor sceneAmbient = scene.getAmbientColor();
	const nbFloat32 clearColor[] = { sceneAmbient.x, sceneAmbient.y, sceneAmbient.z, 1.0f };
	commandList->ClearRenderTargetView(m_msaaRTDescriptorHandle.getCpuHandle(), clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	commandList->ClearDepthStencilView(m_dsvDescriptorsHandle.getCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set OM stage render target
	commandList->OMSetRenderTargets(1, &m_msaaRTDescriptorHandle.getCpuHandle(), FALSE, &m_dsvDescriptorsHandle.getCpuHandle());

	// set the viewport
	commandList->RSSetViewports(1, &m_viewport);

	// set the scissor rect
	commandList->RSSetScissorRects(1, &m_scissorRect);

	// push draw commands
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbs_srv_uavAllocator->getHeap() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Draw cubemap first so it can't override selection highlights
	if (scene.hasCubeMap())
	{
		Effect::CubeMappingPushArgs args = { scene };
		m_cubeMappingEffect->pushDrawCommands(args, commandList, frameIdx);
	}

	// Selection hightlight
	const Model::Mesh* meshSelection = Model::Mesh::fromIselectable(scene.getCurrentSelection());
	if (meshSelection)
	{
		commandList->OMSetStencilRef(0);

		Effect::HighlightColorPushArgs args = { scene, meshSelection };
		m_highlightColorEffect->pushDrawCommands(args, commandList, frameIdx);
	}

	// Now lights
	if (scene.hasLights())
	{
		Effect::RenderLightsPushArgs lightPassArgs = { scene };
		m_renderLightsEffect->pushDrawCommands(lightPassArgs, commandList, frameIdx);
	}

	// Draw scene.
	commandList->OMSetStencilRef(1);
	{
		Effect::ForwardLightningPushArgs args = { scene };
		m_forwardLightningEffect->pushDrawCommands(args, commandList, frameIdx);
	}

	// Light visual lines
	const Light::BaseLight* lightSelection = Light::BaseLight::fromIselectable(scene.getCurrentSelection());
	if (lightSelection)
	{
		Effect::LightVisualLinesPushArgs linesArgs = { scene.getCamera(), lightSelection, scene.getModel()->getBoundsSizeLength()};
		m_lightVisualLinesEffect->pushDrawCommands(linesArgs, commandList, frameIdx);
	}
	
	// Finish with gizmo.
	if (lightSelection || meshSelection)
	{
		const Gizmo::BaseGizmo* gizmo = scene.getCurrentGizmo();
		NEBULA_ASSERT(gizmo);
		const Gizmo::GizmoType gizmoType = gizmo->getType();
		Effect::RenderGizmoPushArgs gizmoArgs = { scene };

		if (gizmoType == Gizmo::GizmoType::Move)
		{
			m_moveGizmoEffect->pushDrawCommands(gizmoArgs, commandList, frameIdx);
		}
		else if (gizmoType == Gizmo::GizmoType::Scale)
		{
			m_scaleGizmoEffect->pushDrawCommands(gizmoArgs, commandList, frameIdx);
		}
		else if (gizmoType == Gizmo::GizmoType::Rotation)
		{
			m_rotationGizmoEffect->pushDrawCommands(gizmoArgs, commandList, frameIdx);
		}
	}

	// Resolve msaa to swap chain render target
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIdx], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RESOLVE_DEST));
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_msaaRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
	commandList->ResolveSubresource(m_renderTargets[frameIdx], 0, m_msaaRenderTarget, 0, NEBULA_SWAP_CHAIN_FORMAT);

	// Transition render target to the present state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[frameIdx], D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_PRESENT));
}

void DX12Renderer::present()
{
	// present the current backbuffer
	HRESULT	hr = m_swapChain->Present(0, 0);
	if (FAILED(hr))
	{
		NEBULA_TRACE("DX12Renderer::render - Failed to present");
	}
}

}}}}
