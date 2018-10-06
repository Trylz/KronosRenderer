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

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct ForwardLightningPushArgs
{
	const Scene::BaseScene& scene;
};

class ForwardLighning : public BaseEffect<ForwardLightningPushArgs&>
{
public:
	ForwardLighning(const DXGI_SAMPLE_DESC& sampleDesc);
	~ForwardLighning();

	void pushDrawCommands(ForwardLightningPushArgs& data, ID3D12GraphicsCommandList* commandList, kInt32 frameIndex) override;

	void onNewScene(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void updateMaterialBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void onUpdateMaterial(const Scene::BaseScene& scene, const MaterialIdentifier& matId, ID3D12GraphicsCommandList* commandList);

private:
	KRONOS_DX12_ATTRIBUTE_ALIGN struct DX12Light
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 direction;
		DirectX::XMFLOAT4 color;

		FLOAT range;
		INT type;
	};

	KRONOS_DX12_ATTRIBUTE_ALIGN struct DX12Material
	{
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 specular;
		DirectX::XMFLOAT4 emissive;

		FLOAT opacity;
		FLOAT shininess;
		FLOAT roughness;

		FLOAT fresnel0;
		INT type;

		BOOL hasDiffuseTex;
		BOOL hasSpecularTex;
		BOOL hasNormalTex;
	};

	struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
	};

	struct PixelShaderEnvironmentCb
	{
		DirectX::XMFLOAT4 mediaInfo;
		DirectX::XMFLOAT4 eyePosition;
		DirectX::XMFLOAT4 sceneAmbient;

		INT nbLights;
		BOOL padding[3];

		DX12Light lights[Graphics::Light::MaxLightsPerScene];
	};

	struct PixelShaderMaterialCB
	{
		DX12Material material;
	};

	kUint32 PixelShaderLightCBAlignedSize = (sizeof(PixelShaderEnvironmentCb) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	kUint32 PixelShaderMaterialCBAlignedSize = (sizeof(PixelShaderMaterialCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initStaticConstantBuffers();
	void initDynamicMaterialConstantBuffer(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void fromMaterialUploadHeapToDefaulHeap(ID3D12GraphicsCommandList* commandList);
	void updateMaterial(const Scene::BaseScene& scene, const MaterialIdentifier& matId, ID3D12GraphicsCommandList* commandList);

	void updateVertexShaderCB(ForwardLightningPushArgs& data, kInt32 frameIndex);
	void updatePixelShaderLightsCB(ForwardLightningPushArgs& data, kInt32 frameIndex);

	PipelineStatePtr m_solidPSO;
	PipelineStatePtr m_wireframePSO;

	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[swapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[swapChainBufferCount];

	// pixel shader lights constant buffer
	PixelShaderEnvironmentCb m_pixelShaderLightsCB;
	CComPtr<ID3D12Resource> m_pixelShaderLightsCBUploadHeaps[swapChainBufferCount];
	UINT8* m_pixelShaderLightsCBGPUAddress[swapChainBufferCount];

	// pixel shader material constant buffer
	UINT8* m_pixelShaderMaterialCBGPUAddress[swapChainBufferCount];
	ID3D12Resource* m_pixelShaderMaterialCBUploadHeaps[swapChainBufferCount];
	ID3D12Resource* m_pixelShaderMaterialCBDefaultHeaps[swapChainBufferCount];

	kUint32 m_materialBufferSize = 0u;
};

inline void ForwardLighning::onNewScene(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	m_materialBufferSize = 0u;
	initDynamicMaterialConstantBuffer(scene, commandList);
}

inline void ForwardLighning::updateMaterialBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	initDynamicMaterialConstantBuffer(scene, commandList);
}

}}}}}