//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Scene/BaseScene.h"
#include <DirectXMath.h>
#include <unordered_map>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct RenderLightsPushArgs
{
	const Scene::BaseScene& scene;
};

class RenderLights : public BaseEffect<RenderLightsPushArgs&>
{
public:
	RenderLights(const DXGI_SAMPLE_DESC& sampleDesc);
	~RenderLights();

	void onNewScene(const Scene::BaseScene& scene);
	void updateLightBuffers(const Scene::BaseScene& scene);

	void pushDrawCommands(RenderLightsPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	NEBULA_DX12_ATTRIBUTE_ALIGN struct VertexShaderSharedCB
	{
		DirectX::XMFLOAT4X4 projMatrix;
		FLOAT billboardScale;
	};

	NEBULA_DX12_ATTRIBUTE_ALIGN struct VertexShaderCenterCB
	{
		DirectX::XMFLOAT4 centerCameraSpace;
	};

	nbUint32 VtxShaderCenterCBAlignedSize = (sizeof(VertexShaderCenterCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderSharedCB();
	void initVertexShaderCenterCB(const Scene::BaseScene& scene);

	void initVertexAndIndexBuffer();
	void initTextures();

	void updateVertexShaderSharedCB(RenderLightsPushArgs& data, nbInt32 frameIndex);
	void updateVertexShaderCenterCB(RenderLightsPushArgs& data, nbInt32 frameIndex);

	PipelineStatePtr m_PSO;

	std::unordered_map<Graphics::Light::LightType, Dx12TextureHandle> m_textures;
	std::unordered_map<Graphics::Light::LightType, Graphics::Texture::RGBAImage*> m_images;

	Dx12VertexBufferHandle m_vtxBuffer;
	Dx12IndexBufferHandle m_idxBuffer;

	CComPtr<ID3D12Resource> m_vShaderSharedCBUploadHeaps[SwapChainBufferCount];
	UINT8* m_vShaderSharedCBGPUAddress[SwapChainBufferCount];

	ID3D12Resource* m_vShaderCenterCBUploadHeaps[SwapChainBufferCount];
	UINT8* m_vShaderCenterCBGPUAddress[SwapChainBufferCount];
};

inline void RenderLights::onNewScene(const Scene::BaseScene& scene)
{
	initVertexShaderCenterCB(scene);
}

inline void RenderLights::updateLightBuffers(const Scene::BaseScene& scene)
{
	initVertexShaderCenterCB(scene);
}
}}}}}
