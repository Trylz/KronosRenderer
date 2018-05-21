//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#include "stdafx.h"

#include "../CreateTextureException.h"
#include "DX12Renderer.h"
#include <d3d12.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
using namespace DirectX; 
using namespace Descriptor;

bool DX12Renderer::init(const InitArgs& args)
{
	m_hwindow = args.hwnd;

	RECT rect;
	KRONOS_ASSERT(GetWindowRect(m_hwindow, &rect) > 0);
	const glm::uvec2 bufferSize(rect.right - rect.left, rect.bottom - rect.top);

	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory));
	if (FAILED(hr))
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create device factory");
		return false;
	}

	if (!createDevice(m_dxgiFactory))
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create device");
		return false;
	}

	// Create descriptor allocators
	m_cbs_srv_uavAllocator = std::make_unique<CBV_SRV_UAV_DescriptorAllocator>(m_device);
	m_rtvAllocator = std::make_unique<RTV_DescriptorAllocator>(m_device);
	m_dsvAllocator = std::make_unique<DSV_DescriptorAllocator>(m_device);

	if (!createDirectCommandQueue())
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create direct command queue");
		return false;
	}

	m_sampleDesc.Count = 1;

	if (!createSwapChain(m_dxgiFactory, bufferSize))
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create swap chain");
		return false;
	}

	if (!createDepthStencilBuffer(bufferSize))
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create depth stencil buffer");
		return false;
	}

	setUpViewportAndScissor(bufferSize);

	if (!createRTVsDescriptor())
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create render target descriptor views");
		return false;
	}

	if (!createDirectCommandAllocators())
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create command allocators");
		return false;
	}

	if (!createDirectCommandList())
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create command list");
		return false;
	}

	if (!createFencesAndFenceEvent())
	{
		KRONOS_TRACE("DX12Renderer::init - Unable to create fence and fence event");
		return false;
	}

	Realtime::GraphicResourceAllocatorPtr<DX12_GRAPHIC_ALLOC_PARAMETERS> = (TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this;

	// Create effects
	Effect::D3d12Device = m_device;

	startCommandRecording();

	m_forwardLightningEffect = std::make_unique<Effect::ForwardLighning>(m_sampleDesc);
	m_cubeMappingEffect = std::make_unique<Effect::CubeMapping>(m_sampleDesc);
	m_highlightColorEffect = std::make_unique<Effect::HighlightColor>(m_sampleDesc);
	m_renderLightsEffect = std::make_unique<Effect::RenderLights>(m_sampleDesc);

	endCommandRecording();

	return true;
}

void DX12Renderer::finishTasks()
{
	// wait for the gpu to finish all frames
	for (int i = 0; i < swapChainBufferCount; ++i)
	{
		m_frameIndex = i;
		waitCurrentBackBufferCommandsFinish();
	}
}

void DX12Renderer::release()
{
	// close the fence event
	CloseHandle(m_fenceEvent);

	// get swapchain out of full screen before exiting
	BOOL fs = false;
	if (m_swapChain->GetFullscreenState(&fs, NULL))
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	releaseSwapChainDynamicResources();
}

bool DX12Renderer::createDevice(IDXGIFactory4* m_dxgiFactory)
{
#if defined(_DEBUG)
	//Enable the debug layer
	CComPtr<ID3D12Debug> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
#endif

	HRESULT hr;

	// adapters are the graphics card (this includes the embedded graphics on the motherboard)
	IDXGIAdapter1* adapter;

	// we'll start looking for directx 12 compatible graphics devices starting at index 0
	int adapterIndex = 0;

	// set this to true when a good one was found
	bool adapterFound = false;

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
		IID_PPV_ARGS(&m_device)
	);

	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool DX12Renderer::createDirectCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	cqDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cqDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;// direct means the gpu can directly execute this command queue

	// create the command queue
	auto hr = m_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool DX12Renderer::createSwapChain(IDXGIFactory4* m_dxgiFactory, const glm::uvec2& bufferSize)
{
	DXGI_MODE_DESC backBufferDesc = {}; // this is to describe our display mode
	backBufferDesc.Width = bufferSize.x; // buffer width
	backBufferDesc.Height = bufferSize.y; // buffer height
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // format of the buffer (rgba 32 bits, 8 bits for each chanel)

	// Describe and create the swap chain.
	m_swapChainDesc.BufferCount = swapChainBufferCount; // number of buffers we have
	m_swapChainDesc.BufferDesc = backBufferDesc; // our back buffer description
	m_swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // this says the pipeline will render to this swap chain
	m_swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // dxgi will discard the buffer (data) after we call present
	m_swapChainDesc.OutputWindow = m_hwindow; // handle to our window
	m_swapChainDesc.SampleDesc = m_sampleDesc;
	m_swapChainDesc.Windowed = true; // set to true, then if in fullscreen must call SetFullScreenState with true for full screen to get uncapped fps

	IDXGISwapChain* tempSwapChain;

	HRESULT hr = m_dxgiFactory->CreateSwapChain(
		m_commandQueue, // the queue will be flushed once the swap chain is created
		&m_swapChainDesc, // give it the swap chain description we created above
		&tempSwapChain
	);

	if (FAILED(hr))
	{
		return false;
	}

	m_swapChain.Attach(static_cast<IDXGISwapChain3*>(tempSwapChain));

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	return true;
}

bool DX12Renderer::createRTVsDescriptor()
{
	HRESULT hr;
	ResourceArray resourceArray;
	
	// Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
	for (int i = 0; i < swapChainBufferCount; i++)
	{
		// first we get the n'th buffer in the swap chain and store it in the n'th
		// position of our ID3D12Resource array
		hr = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
		if (FAILED(hr))
		{
			return false;
		}

		resourceArray.push_back(m_renderTargets[i]);
	}

	m_rtvDescriptorsHandle = m_rtvAllocator->allocate(resourceArray);

	return true;
}

bool DX12Renderer::createDirectCommandAllocators()
{
	for (int i = 0; i < swapChainBufferCount; i++)
	{
		HRESULT hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	return true;
}

bool DX12Renderer::createDirectCommandList()
{
	// create the command list with the first allocator
	HRESULT hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[m_frameIndex], NULL, IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		return false;
	}

	m_commandList->SetName(L"Command list");
	m_commandList->Close();

	return true;
}

bool DX12Renderer::createFencesAndFenceEvent()
{
	HRESULT hr;

	// create the fences
	for (int i = 0; i < swapChainBufferCount; i++)
	{
		hr = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
		if (FAILED(hr))
		{
			return false;
		}

		// set the initial fence value to 0
		m_fenceValue[i] = 0;
	}

	// create a handle to a fence event
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		return false;
	}

	return true;
}

bool DX12Renderer::createDepthStencilBuffer(const glm::uvec2& bufferSize)
{
	const DXGI_FORMAT format = Effect::defaultDSFormat;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = format;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	HRESULT hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(format, bufferSize.x, bufferSize.y, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencilBuffer)
	);

	if (FAILED(hr))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = format;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	m_dsvDescriptorsHandle = m_dsvAllocator->allocate({ m_depthStencilBuffer }, depthStencilDesc);

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
	KRONOS_ASSERT(m_depthStencilBuffer);
	m_depthStencilBuffer->Release();

	for (int i = 0; i < swapChainBufferCount; i++)
	{
		m_renderTargets[i]->Release();
	}

	m_rtvAllocator->release(m_rtvDescriptorsHandle);
	m_dsvAllocator->release(m_dsvDescriptorsHandle);
}

void DX12Renderer::resizeBuffers(const glm::uvec2& newSize)
{
	releaseSwapChainDynamicResources();

	HRESULT hr = m_swapChain->ResizeBuffers(0, newSize.x, newSize.y, DXGI_FORMAT_UNKNOWN, m_swapChainDesc.Flags);
	KRONOS_ASSERT(SUCCEEDED(hr));

	KRONOS_ASSERT(createDepthStencilBuffer(newSize));

	KRONOS_ASSERT(createRTVsDescriptor());

	setUpViewportAndScissor(newSize);
}

void DX12Renderer::createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, Dx12TextureHandle& dst)
{
	uint32_t width = images[0]->getWidth();
	uint32_t height = images[0]->getHeight();

	if (width > D3D12_REQ_TEXTURECUBE_DIMENSION)
		throw CreateTextureException("Images width is too high.");
	if (height > D3D12_REQ_TEXTURECUBE_DIMENSION)
		throw CreateTextureException("Images height is too high");

	for (auto im : images)
	{
		if (im->getWidth() != width)
			throw CreateTextureException("All images must have the same width. It is not the case for: " + im->getPath());

		if (im->getHeight() != height)
			throw CreateTextureException("All images must have the same height. It is not the case for: " + im->getPath());
	}

	createRGBATexture2DArray(images, width, height, D3D12_SRV_DIMENSION_TEXTURECUBE, dst);
}

void DX12Renderer::createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, uint32_t width, uint32_t height, D3D12_SRV_DIMENSION viewDimension, Dx12TextureHandle& dst)
{
	const uint32_t arraySize = (uint32_t)images.size();
	KRONOS_ASSERT(arraySize < D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Alignment = 0; // may be 0, 4KB, 64KB, or 4MB. 0 will let runtime decide between 64KB and 4MB (4MB for multi-sampled textures)
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.DepthOrArraySize = static_cast<std::uint16_t>(arraySize); // if 3d image, depth of 3d image. Otherwise an array of 1D or 2D textures (we only have one image, so we set 1)
	textureDesc.MipLevels = 1; // Number of mipmaps. We are not generating mipmaps for this texture, so we have only one level

	// Set is the dxgi format of the image (format of the pixels)
#if KRONOS_IMAGE_COLORORDER == KRONOS_IMAGE_COLORORDER_BGR
	textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
#else
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
#endif

	textureDesc.SampleDesc.Count = 1; // This is the number of samples per pixel, we just want 1 sample
	textureDesc.SampleDesc.Quality = 0; // The quality level of the samples. Higher is better quality, but worse performance
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN; // The arrangement of the pixels. Setting to unknown lets the driver choose the most efficient one
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE; // no flags

	// create a default heap.
	HRESULT hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), // a default heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&textureDesc, // the description of our texture
		D3D12_RESOURCE_STATE_COPY_DEST, // We will copy the texture from the upload heap to here, so we start it out in a copy dest state
		nullptr, // used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&dst.buffer));

	KRONOS_ASSERT(SUCCEEDED(hr));

	dst.format = textureDesc.Format;

	std::string bufferDebugString = "Texture Resource Heap images first = " + images[0]->getPath() + ", size = " + std::to_string(arraySize);
	std::wstring bufferDebugStringW(bufferDebugString.begin(), bufferDebugString.end());
	dst.buffer->SetName(bufferDebugStringW.c_str());

	UINT64 textureUploadBufferSize;
	m_device->GetCopyableFootprints(&textureDesc, 0, arraySize, 0, nullptr, nullptr, nullptr, &textureUploadBufferSize);

	// now we create an upload heap to upload our texture to the GPU
	ID3D12Resource* textureBufferUploadHeap;

	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(textureUploadBufferSize), // resource description for a buffer (storing the image data in this heap just to copy to the default heap)
		D3D12_RESOURCE_STATE_GENERIC_READ, // We will copy the contents from this heap to the default heap above
		nullptr,
		IID_PPV_ARGS(&textureBufferUploadHeap));

	KRONOS_ASSERT(SUCCEEDED(hr));

	std::string uploadDebugString = "Texture Upload Resource Heap images first = " + images[0]->getPath() + ", size = " + std::to_string(arraySize);
	std::wstring uploadDebugStringW(uploadDebugString.begin(), uploadDebugString.end());

	textureBufferUploadHeap->SetName(uploadDebugStringW.c_str());

	// Initialize subresources
	std::vector<D3D12_SUBRESOURCE_DATA> subResourceData(arraySize);
	const int imageBytesPerRow = width * 4;

	for (uint32_t i = 0u; i < arraySize; ++i)
	{
		subResourceData[i].pData = images[i]->getRawData();
		subResourceData[i].RowPitch = imageBytesPerRow;
		subResourceData[i].SlicePitch = imageBytesPerRow * height;
	}

	// Now we copy the upload buffer contents to the default heap
	UpdateSubresources(m_commandList, dst.buffer, textureBufferUploadHeap, 0, 0, arraySize, &subResourceData[0]);

	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(dst.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON));

	m_loadingResources.push_back(textureBufferUploadHeap);

	// Init descriptor
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = viewDimension;
	srvDesc.Format = dst.format;
	srvDesc.Texture2D.MipLevels = 1;
	dst.descriptorHandle = m_cbs_srv_uavAllocator->allocate({ dst.buffer }, srvDesc);
}

bool DX12Renderer::createIndexBuffer(const std::vector<uint32_t>& data, Dx12IndexBufferHandle& dst)
{
	ArrayBufferResource resourceData = createArrayBufferRecource(data.data(), sizeof(uint32_t), (uint32_t)data.size());
	if (resourceData.bufferSize == 0u || resourceData.buffer == nullptr)
	{
		return false;
	}

	dst.buffer = resourceData.buffer;
	dst.bufferView.BufferLocation = dst.buffer->GetGPUVirtualAddress();
	dst.bufferView.Format = DXGI_FORMAT_R32_UINT;
	dst.bufferView.SizeInBytes = resourceData.bufferSize;

	return true;
}

DX12Renderer::ArrayBufferResource DX12Renderer::createArrayBufferRecource(const void* data, uint32_t sizeofElem, uint32_t count)
{
	ArrayBufferResource retData;
	retData.bufferSize = sizeofElem * count;

	HRESULT hr;

	// create default heap
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&retData.buffer));

	KRONOS_ASSERT(SUCCEEDED(hr));

	// create upload heap
	ID3D12Resource* vBufferUploadHeap;
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));

	KRONOS_ASSERT(SUCCEEDED(hr));

	// store buffer data in upload heap
	D3D12_SUBRESOURCE_DATA bufferData = {};
	bufferData.pData = reinterpret_cast<BYTE*>(const_cast<void*>(data));
	bufferData.RowPitch = retData.bufferSize;
	bufferData.SlicePitch = retData.bufferSize;

	// we are now creating a command with the command list to copy the data from the upload heap to the default heap
	UpdateSubresources(m_commandList, retData.buffer, vBufferUploadHeap, 0, 0, 1, &bufferData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(retData.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_loadingResources.push_back(vBufferUploadHeap);

	return retData;
}

void DX12Renderer::startCommandRecording()
{
	waitCurrentBackBufferCommandsFinish();

	// we can only reset an allocator once the gpu is done with it
	// resetting an allocator frees the memory that the command list was stored in
	HRESULT hr = m_commandAllocator[m_frameIndex]->Reset();
	if (FAILED(hr))
	{
		KRONOS_TRACE("DX12Renderer::startCommandRecording - Failed to reset command allocator index = " << m_frameIndex);
		return;
	}

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter.
	hr = m_commandList->Reset(m_commandAllocator[m_frameIndex], nullptr);
	KRONOS_ASSERT(SUCCEEDED(hr));
}

void DX12Renderer::endCommandRecording()
{
	HRESULT hr = m_commandList->Close();
	KRONOS_ASSERT(SUCCEEDED(hr));

	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { m_commandList };

	// execute the array of command lists
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	if (m_loadingResources.size())
	{
		m_fenceValue[m_frameIndex]++;
		waitCurrentBackBufferCommandsFinish();

		for (auto resource : m_loadingResources)
			resource->Release();

		m_loadingResources.clear();
	}
}

void DX12Renderer::endSceneLoadCommandRecording(const Scene::BaseScene* scene)
{
	if (scene)
	{
		m_forwardLightningEffect->onNewScene(*scene, m_commandList);
		m_renderLightsEffect->onNewScene(*scene);
	}

	endCommandRecording();
}

void DX12Renderer::drawScene(const Scene::BaseScene& scene)
{
	// transition the "frameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// here we again get the handle to our current render target view so we can set it as the render target in the output merger stage of the pipeline
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorsHandle.getCpuHandle(),
		m_frameIndex,
		m_rtvAllocator->getDescriptorSize());

	// set the render target for the output merger stage (the output of the pipeline)
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &m_dsvDescriptorsHandle.getCpuHandle());

	// Clear the render target
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// clear the depth/stencil buffer
	m_commandList->ClearDepthStencilView(m_dsvDescriptorsHandle.getCpuHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// set the viewport
	m_commandList->RSSetViewports(1, &m_viewport);

	// set the scissor rect
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	// push draw commands
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_cbs_srv_uavAllocator->getHeap() };
	m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// Draw cubemap first so it can't override selection highlights
	if (scene.hasCubeMap())
	{
		Effect::CubeMappingPushArgs args = { scene };
		m_cubeMappingEffect->pushDrawCommands(args, m_commandList, m_frameIndex);
	}

	// Selection hightlight
	const Model::Mesh* meshSelection = Model::Mesh::fromIselectable(scene.getCurrentSelection());
	if (meshSelection)
	{
		Effect::HighlightColorPushArgs args = { scene, meshSelection };
		m_highlightColorEffect->pushDrawCommands(args, m_commandList, m_frameIndex);
	}

	// Now lights
	if (scene.hasLights())
	{
		Effect::RenderLightsPushArgs lightPassArgs = { scene };
		m_renderLightsEffect->pushDrawCommands(lightPassArgs, m_commandList, m_frameIndex);
	}

	// Draw scene.
	m_commandList->OMSetStencilRef(1);
	Effect::ForwardLightningPushArgs args = { scene };
	m_forwardLightningEffect->pushDrawCommands(args, m_commandList, m_frameIndex);

	// transition the "frameIndex" render target from the render target state to the present state.
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
}

void DX12Renderer::present()
{
	// present the current backbuffer
	HRESULT	hr = m_swapChain->Present(KRONOS_REALTIME_VSYNC_ENABLED, 0);
	if (FAILED(hr))
	{
		KRONOS_TRACE("DX12Renderer::render - Failed to present");
	}
}

void DX12Renderer::waitCurrentBackBufferCommandsFinish()
{
	HRESULT hr;
	// swap the current rtv buffer index so we draw on the correct buffer
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	const UINT64 fence = m_fenceValue[m_frameIndex];
	hr = m_commandQueue->Signal(m_fence[m_frameIndex], fence);

	// Wait until GPU finish with command queue.
	if (m_fence[m_frameIndex]->GetCompletedValue() < fence)
	{
		hr = m_fence[m_frameIndex]->SetEventOnCompletion(fence, m_fenceEvent);
		KRONOS_ASSERT(SUCCEEDED(hr));

		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	KRONOS_ASSERT(SUCCEEDED(hr));
	m_fenceValue[m_frameIndex]++;
}
}}}}
