//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#pragma once

#include "HandleTypes.h"
#include "Descriptor/Allocators.h"
#include "Graphics/Model/TModel.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/CubeMappingEffect.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/ForwardLighningEffect.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/HighlightColorEffect.h"
#include "Graphics/Renderer/Realtime/TRealtimeRenderer.h"
#include "Graphics/Texture/TCubeMap.h"

#include <dxgi1_4.h>
#include <memory>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
struct InitArgs
{
	HWND hwnd;
	float screenAspectRatio;
};

#define DX12_GRAPHIC_ALLOC_PARAMETERS Dx12TextureHandle,\
Dx12VertexBufferHandle,\
Dx12IndexBufferHandle

using DX12Model = Graphics::Model::TModel<DX12_GRAPHIC_ALLOC_PARAMETERS>;
using DX12CubeMap = Graphics::Texture::TCubeMap<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class DX12Renderer : public TRealtimeRenderer<DX12_GRAPHIC_ALLOC_PARAMETERS, InitArgs>
{
public:
	bool init(const InitArgs& args) override;
	void finishTasks() override;
	void release() override;

	Graphics::Model::ModelPtr createModelFromRaw(const std::string& path) const override;
	Graphics::Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const override;

	Graphics::Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const override;
	Graphics::Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const override;

	void drawScene(const Graphics::Scene::BaseScene& scene, const MeshSelection& currentSelection) override;
	void present() override;

	void createRGBATexture2D(const Texture::RGBAImage* image, Dx12TextureHandle& dst) override;
	void createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, Dx12TextureHandle& dst) override;

	bool releaseTexture(const Dx12TextureHandle& textureHandle) const override;

	bool createVertexBuffer(const std::vector<Model::Vertex>& data, Dx12VertexBufferHandle& dst) override;
	bool createVertexBuffer(const std::vector<Graphics::Texture::CubeMap::Vertex>& data, Dx12VertexBufferHandle& dst) override;
	bool releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const override;

	bool createIndexBuffer(const std::vector<uint32_t>& data, Dx12IndexBufferHandle& dst) override;
	bool releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const override;

	void resizeBuffers(const glm::uvec2& newSize) override;
	void onMeshSelectionMaterialChanged(const Graphics::Scene::BaseScene& scene, const MeshSelection& currentSelection) override;

	void startCommandRecording() override;
	void endSceneLoadCommandRecording(const Graphics::Scene::BaseScene* scene) override;
	void endCommandRecording() override;

	static float s_screenAspectRatio;

private:
	void waitCurrentBackBufferCommandsFinish();

	// Dx12 initilization helpers
	// @See: // https://www.braynzarsoft.net/viewtutorial/q16390-03-initializing-directx-12
	bool createDevice(IDXGIFactory4* m_dxgiFactory);
	bool createDirectCommandQueue();
	bool createSwapChain(IDXGIFactory4* m_dxgiFactory, const glm::uvec2& bufferSize);
	bool createDepthStencilBuffer(const glm::uvec2& bufferSize);
	void setUpViewportAndScissor(const glm::uvec2& bufferSize);
	bool createRTVsDescriptor();
	bool createDirectCommandAllocators();
	bool createDirectCommandList();
	bool createFencesAndFenceEvent();
	void releaseSwapChainDynamicResources();

	struct ArrayBufferResource : ResourceBuffer
	{
		uint32_t bufferSize = 0u;
	};

	template<typename VertexDataType>
	bool createVertexBuffer(const std::vector<VertexDataType>& data, Dx12VertexBufferHandle& dst);

	// Create a resource given a vector of data. Used for vertex and index buffers.
	template<typename BufferDataType>
	ArrayBufferResource createArrayBufferRecource(const std::vector<BufferDataType>& data);

	void createRGBATextureArray2D(const std::vector<const Texture::RGBAImage*>& image, uint32_t width, uint32_t height, D3D12_SRV_DIMENSION viewDimension, Dx12TextureHandle& dst);

	// the window
	HWND m_hwindow;

	// direct3d device
	SharedDevicePtr m_device;

	// Descriptor allocators
	std::unique_ptr<Descriptor::CBV_SRV_UAV_DescriptorAllocator> m_cbs_srv_uavAllocator;
	std::unique_ptr<Descriptor::RTV_DescriptorAllocator> m_rtvAllocator;
	std::unique_ptr<Descriptor::DSV_DescriptorAllocator> m_dsvAllocator;

	// the dxgi factory
	CComPtr<IDXGIFactory4> m_dxgiFactory;

	// swapchain used to switch between render targets
	CComPtr<IDXGISwapChain3> m_swapChain;

	// The swap chain description structure
	DXGI_SWAP_CHAIN_DESC m_swapChainDesc = {};

	// container for command lists
	CComPtr<ID3D12CommandQueue> m_commandQueue;

	DescriptorHandle m_rtvDescriptorsHandle;
	DescriptorHandle m_dsvDescriptorsHandle;

	// number of render targets equal to buffer count
	ID3D12Resource* m_renderTargets[swapChainBufferCount];

	// we want enough allocators for each buffer * number of threads (we only have one thread)
	CComPtr<ID3D12CommandAllocator> m_commandAllocator[swapChainBufferCount];

	// a command list we can record commands into, then execute them to render the frame
	CComPtr<ID3D12GraphicsCommandList> m_commandList;

	// an object that is locked while our command list is being executed by the gpu. We need as many 
	CComPtr<ID3D12Fence> m_fence[swapChainBufferCount];

	// a handle to an event when our fence is unlocked by the gpu
	HANDLE m_fenceEvent; 

	// this value is incremented each frame. each fence will have its own value
	UINT64 m_fenceValue[swapChainBufferCount];

	// current rtv we are on
	int m_frameIndex; 

	// area that output from rasterizer will be stretched to.
	D3D12_VIEWPORT m_viewport; 

	// the area to draw in. pixels outside that area will not be drawn onto
	D3D12_RECT m_scissorRect; 

	// This is the memory for our depth buffer.
	ID3D12Resource* m_depthStencilBuffer =  nullptr;

	std::vector<ID3D12Resource*> m_loadingResources;

	std::unique_ptr<Effect::ForwardLighningEffect> m_forwardLightningEffect;
	std::unique_ptr<Effect::CubeMappingEffect> m_cubeMappingEffect;
	std::unique_ptr<Effect::HighlightColorEffect> m_highlightColorEffect;

	// describe our multi-sampling. We are not multi-sampling, so we set the count to 1 (we need at least one sample of course)
	DXGI_SAMPLE_DESC m_sampleDesc = {};
};

inline Graphics::Model::ModelPtr DX12Renderer::createModelFromRaw(const std::string& path) const
{
	return std::make_unique<DX12Model>((TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this, path);
}

inline Graphics::Model::ModelPtr DX12Renderer::createModelFromNode(const Utilities::Xml::XmlNode& node)const
{
	return std::make_unique<DX12Model>((TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this, node);
}

inline Graphics::Texture::CubeMapPtr DX12Renderer::createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const
{
	return std::make_unique<DX12CubeMap>((TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this, args);
}

inline Graphics::Texture::CubeMapPtr DX12Renderer::createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const
{
	return std::make_unique<DX12CubeMap>((TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>*) this, args);
}

inline void DX12Renderer::onMeshSelectionMaterialChanged(const Graphics::Scene::BaseScene& scene, const MeshSelection& currentSelection)
{
	m_forwardLightningEffect->onUpdateGroupMaterial(scene, currentSelection, m_commandList);
}

inline void DX12Renderer::createRGBATexture2D(const Texture::RGBAImage* image, Dx12TextureHandle& dst)
{
	createRGBATextureArray2D({ image }, image->getWidth(), image->getHeight(), D3D12_SRV_DIMENSION_TEXTURE2D, dst);
}

inline bool DX12Renderer::releaseTexture(const Dx12TextureHandle& textureHandle) const
{
	m_cbs_srv_uavAllocator->release(textureHandle.descriptorHandle);
	return textureHandle.buffer->Release();
}

inline bool DX12Renderer::releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const
{
	return arrayBufferHandle.buffer->Release();
}

inline bool DX12Renderer::releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const
{
	return arrayBufferHandle.buffer->Release();
}

template<typename VertexDataType>
bool DX12Renderer::createVertexBuffer(const std::vector<VertexDataType>& data, Dx12VertexBufferHandle& dst)
{
	ArrayBufferResource resourceData = createArrayBufferRecource(data);
	if (resourceData.bufferSize == 0u || resourceData.buffer == nullptr)
	{
		return false;
	}

	dst.buffer = resourceData.buffer;
	dst.bufferView.BufferLocation = dst.buffer->GetGPUVirtualAddress();
	dst.bufferView.StrideInBytes = sizeof(VertexDataType);
	dst.bufferView.SizeInBytes = resourceData.bufferSize;

	return true;
}

template<typename BufferDataType>
DX12Renderer::ArrayBufferResource DX12Renderer::createArrayBufferRecource(const std::vector<BufferDataType>& data)
{
	ArrayBufferResource retData;

	BufferDataType* dataArray = const_cast<BufferDataType*>(&data[0]);
	uint32_t arraySize = (uint32_t)data.size();

	retData.bufferSize = sizeof(BufferDataType) * arraySize;

	HRESULT hr;

	// create default heap
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST, // we will start this heap in the copy destination state since we will copy data from the upload heap to this heap
		nullptr, // optimized clear value must be null for this type of resource. used for render targets and depth/stencil buffers
		IID_PPV_ARGS(&retData.buffer));

	ORION_ASSERT(SUCCEEDED(hr));

	// create upload heap
	ID3D12Resource* vBufferUploadHeap;
	hr = m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(retData.bufferSize), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap));

	ORION_ASSERT(SUCCEEDED(hr));

	// store buffer data in upload heap
	D3D12_SUBRESOURCE_DATA bufferData = {};
	bufferData.pData = reinterpret_cast<BYTE*>(dataArray);
	bufferData.RowPitch = retData.bufferSize;
	bufferData.SlicePitch = retData.bufferSize;

	// we are now creating a command with the command list to copy the data from the upload heap to the default heap
	UpdateSubresources(m_commandList, retData.buffer, vBufferUploadHeap, 0, 0, 1, &bufferData);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(retData.buffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_loadingResources.push_back(vBufferUploadHeap);

	return retData;
}
}}}}