//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Graphics/Scene/BaseScene.h"
#include <DirectXMath.h>
#include <unordered_map>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct RenderLightsPushArgs
{
	const Graphics::Scene::BaseScene& scene;
};

class RenderLights : public BaseEffect<RenderLightsPushArgs&>
{
public:
	RenderLights(const DXGI_SAMPLE_DESC& sampleDesc);
	~RenderLights();

	void onNewScene(const Graphics::Scene::BaseScene& scene);
	void onNewlightAdded(const Graphics::Scene::BaseScene& scene);

	void pushDrawCommands(RenderLightsPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex) override;

private:
	KRONOS_DX12_ATTRIBUTE_ALIGN struct VertexShaderSharedCB
	{
		DirectX::XMFLOAT4X4 projMatrix;
		FLOAT billboardScale;
	};

	KRONOS_DX12_ATTRIBUTE_ALIGN struct VertexShaderCenterCB
	{
		DirectX::XMFLOAT4 centerCameraSpace;
	};

	uint32_t VtxShaderCenterCBAlignedSize = (sizeof(VertexShaderCenterCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderSharedCB();
	void initVertexShaderCenterCB(const Graphics::Scene::BaseScene& scene);

	void initVertexAndIndexBuffer();
	void initTextures();

	void updateVertexShaderSharedCB(RenderLightsPushArgs& data, int frameIndex);
	void updateVertexShaderCenterCB(RenderLightsPushArgs& data, int frameIndex);

	PipelineStatePtr m_PSO;

	std::unordered_map<Graphics::Light::LightType, Dx12TextureHandle> m_textures;
	std::unordered_map<Graphics::Light::LightType, Graphics::Texture::RGBAImage*> m_images;

	Dx12VertexBufferHandle m_vtxBuffer;
	Dx12IndexBufferHandle m_idxBuffer;

	CComPtr<ID3D12Resource> m_vShaderSharedCBUploadHeaps[swapChainBufferCount];
	UINT8* m_vShaderSharedCBGPUAddress[swapChainBufferCount];

	ID3D12Resource* m_vShaderCenterCBUploadHeaps[swapChainBufferCount];
	UINT8* m_vShaderCenterCBGPUAddress[swapChainBufferCount];
};

inline void RenderLights::onNewScene(const Graphics::Scene::BaseScene& scene)
{
	initVertexShaderCenterCB(scene);
}

inline void RenderLights::onNewlightAdded(const Graphics::Scene::BaseScene& scene)
{
	initVertexShaderCenterCB(scene);
}
}}}}}
