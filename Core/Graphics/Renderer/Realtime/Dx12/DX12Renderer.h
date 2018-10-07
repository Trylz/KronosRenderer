//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "HandleTypes.h"
#include "Descriptor/Allocators.h"
#include "Graphics/Model/TModel.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/CubeMapping.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/ForwardLighning.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/HighlightColor.h"
#include "Graphics/Renderer/Realtime/Dx12/Effect/RenderLights.h"
#include "Graphics/Renderer/Realtime/TRealtimeRenderer.h"

#include <dxgi1_4.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12
{
struct InitArgs
{
	HWND hwnd;
};

using DX12Model = Graphics::Model::TModel<DX12_GRAPHIC_ALLOC_PARAMETERS>;

class DX12Renderer : public TRealtimeRenderer<DX12_GRAPHIC_ALLOC_PARAMETERS, InitArgs>
{
public:
	nbBool init(const InitArgs& args) override;
	void finishTasks() override;
	void release() override;

	Graphics::Model::ModelPtr createModelFromRaw(const std::string& path) const override;
	Graphics::Model::ModelPtr createModelFromNode(const Utilities::Xml::XmlNode& node) const override;

	Graphics::Texture::CubeMapPtr createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const override;
	Graphics::Texture::CubeMapPtr createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const override;

	void drawScene(const Scene::BaseScene& scene) override;
	void present() override;

	void createRGBATexture2D(const Texture::RGBAImage* image, Dx12TextureHandle& dst) override;
	void createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, Dx12TextureHandle& dst) override;

	nbBool releaseTexture(const Dx12TextureHandle& textureHandle) const override;

	nbBool createVertexBuffer(const void* data, Dx12VertexBufferHandle& dst, nbUint32 sizeofElem, nbUint32 count) override;
	nbBool releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const override;

	nbBool createIndexBuffer(const std::vector<nbUint32>& data, Dx12IndexBufferHandle& dst) override;
	nbBool releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const override;

	void resizeBuffers(const glm::uvec2& newSize) override;
	void onMaterialChanged(const Scene::BaseScene& scene, const MaterialIdentifier& matId) override;

	void updateLightBuffers(const Scene::BaseScene& scene) override;
	void updateMaterialBuffers(const Scene::BaseScene& scene) override;

	void startCommandRecording() override;
	void endSceneLoadCommandRecording(const Scene::BaseScene* scene) override;
	void endCommandRecording() override;

private:
	void waitCurrentBackBufferCommandsFinish();

	// Dx12 initilization helpers
	// @See: // https://www.braynzarsoft.net/viewtutorial/q16390-03-initializing-directx-12
	nbBool createDevice(IDXGIFactory4* m_dxgiFactory);
	nbBool createDirectCommandQueue();
	nbBool createSwapChain(IDXGIFactory4* m_dxgiFactory, const glm::uvec2& bufferSize);
	nbBool createMSAARenderTarget(const glm::uvec2& bufferSize);
	nbBool createDepthStencilBuffer(const glm::uvec2& bufferSize);
	void setUpViewportAndScissor(const glm::uvec2& bufferSize);
	nbBool bindSwapChainRenderTargets();
	nbBool createDirectCommandAllocators();
	nbBool createDirectCommandList();
	nbBool createFencesAndFenceEvent();
	void releaseSwapChainDynamicResources();

	struct ArrayBufferResource : ResourceBuffer
	{
		nbUint32 bufferSize = 0u;
	};

	ArrayBufferResource createArrayBufferRecource(const void* data, nbUint32 sizeofElem, nbUint32 count);

	void createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& image, nbUint32 width, nbUint32 height, D3D12_SRV_DIMENSION viewDimension, Dx12TextureHandle& dst);

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

	DescriptorHandle m_msaaRTDescriptorHandle;
	DescriptorHandle m_dsvDescriptorsHandle;

	// number of render targets equal to buffer count
	ID3D12Resource* m_renderTargets[swapChainBufferCount];

	// we want enough allocators for each buffer * number of threads (we only have one thread)
	CComPtr<ID3D12CommandAllocator> m_commandAllocator[swapChainBufferCount];

	// a command list we can record commands into, then execute them to render the frame
	CComPtr<ID3D12GraphicsCommandList> m_commandList;

	// an object that is locked while our command list is being executed by the gpu.
	CComPtr<ID3D12Fence> m_fence[swapChainBufferCount];

	// a handle to an event when our fence is unlocked by the gpu
	HANDLE m_fenceEvent; 

	// this value is incremented each frame. each fence will have its own value
	UINT64 m_fenceValue[swapChainBufferCount];

	// current rtv we are on
	nbInt32 m_frameIndex;

	// area that output from rasterizer will be stretched to.
	D3D12_VIEWPORT m_viewport; 

	// the area to draw in. pixels outside that area will not be drawn onto
	D3D12_RECT m_scissorRect; 

	// The multi-sampling description
	DXGI_SAMPLE_DESC m_sampleDesc = {};

	// This is the memory for our depth buffer.
	ID3D12Resource* m_depthStencilBuffer =  nullptr;

	// The offscreen render targer
	ID3D12Resource* m_msaaRenderTarget = nullptr;

	std::vector<ID3D12Resource*> m_loadingResources;

	std::unique_ptr<Effect::ForwardLighning> m_forwardLightningEffect;
	std::unique_ptr<Effect::CubeMapping> m_cubeMappingEffect;
	std::unique_ptr<Effect::HighlightColor> m_highlightColorEffect;
	std::unique_ptr<Effect::RenderLights> m_renderLightsEffect;
};

inline Graphics::Model::ModelPtr DX12Renderer::createModelFromRaw(const std::string& path) const
{
	return std::make_unique<DX12Model>(path);
}

inline Graphics::Model::ModelPtr DX12Renderer::createModelFromNode(const Utilities::Xml::XmlNode& node)const
{
	return std::make_unique<DX12Model>(node);
}

inline Graphics::Texture::CubeMapPtr DX12Renderer::createCubeMapFromRaw(Graphics::Texture::CubeMapConstructRawArgs& args) const
{
	return std::make_unique<DX12CubeMap>(args);
}

inline Graphics::Texture::CubeMapPtr DX12Renderer::createCubeMapFromNode(const Graphics::Texture::CubeMapConstructNodeArgs& args) const
{
	return std::make_unique<DX12CubeMap>(args);
}

inline nbBool DX12Renderer::createVertexBuffer(const void* data, Dx12VertexBufferHandle& dst, nbUint32 sizeofElem, nbUint32 count)
{
	ArrayBufferResource resourceData = createArrayBufferRecource(data, sizeofElem, count);
	if (resourceData.bufferSize == 0u || resourceData.buffer == nullptr)
	{
		return false;
	}

	dst.buffer = resourceData.buffer;
	dst.bufferView.BufferLocation = dst.buffer->GetGPUVirtualAddress();
	dst.bufferView.StrideInBytes = sizeofElem;
	dst.bufferView.SizeInBytes = resourceData.bufferSize;

	return true;
}

inline void DX12Renderer::onMaterialChanged(const Scene::BaseScene& scene, const MaterialIdentifier& matId)
{
	m_forwardLightningEffect->onUpdateMaterial(scene, matId, m_commandList);
}

inline void DX12Renderer::updateMaterialBuffers(const Scene::BaseScene& scene)
{
	m_forwardLightningEffect->updateMaterialBuffers(scene, m_commandList);
}

inline void DX12Renderer::updateLightBuffers(const Scene::BaseScene& scene)
{
	m_renderLightsEffect->updateLightBuffers(scene);
}

inline void DX12Renderer::createRGBATexture2D(const Texture::RGBAImage* image, Dx12TextureHandle& dst)
{
	createRGBATexture2DArray({ image }, image->getWidth(), image->getHeight(), D3D12_SRV_DIMENSION_TEXTURE2D, dst);
}

inline nbBool DX12Renderer::releaseTexture(const Dx12TextureHandle& textureHandle) const
{
	m_cbs_srv_uavAllocator->release(textureHandle.descriptorHandle);
	return textureHandle.buffer->Release();
}

inline nbBool DX12Renderer::releaseVertexBuffer(const Dx12VertexBufferHandle& arrayBufferHandle) const
{
	return arrayBufferHandle.buffer->Release();
}

inline nbBool DX12Renderer::releaseIndexBuffer(const Dx12IndexBufferHandle& arrayBufferHandle) const
{
	return arrayBufferHandle.buffer->Release();
}

}}}}
