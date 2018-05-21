//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Scene/BaseScene.h"
#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct ForwardLightningPushArgs
{
	const Graphics::Scene::BaseScene& scene;
};

class ForwardLighning : public BaseEffect<ForwardLightningPushArgs&>
{
public:
	ForwardLighning(const DXGI_SAMPLE_DESC& sampleDesc);
	~ForwardLighning();

	void pushDrawCommands(ForwardLightningPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex) override;
	void onNewScene(const Graphics::Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void onUpdateGroupMaterial(const Graphics::Scene::BaseScene& scene, const Graphics::Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList);

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

		FLOAT fresnel0;
		INT type;

		BOOL hasDiffuseTex;
		BOOL hasSpecularTex;
	};

	struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
	};

	struct PixelShaderLightsCB
	{
		DirectX::XMFLOAT4 eyePosition;
		DirectX::XMFLOAT4 sceneAmbient;

		INT nbLights;
		BOOL padding[3];

		DX12Light lights[Graphics::Light::maxLightsPerScene];
	};

	struct PixelShaderMaterialCB
	{
		DX12Material material;
	};

	uint32_t PixelShaderLightCBAlignedSize = (sizeof(PixelShaderLightsCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
	uint32_t PixelShaderMaterialCBAlignedSize = (sizeof(PixelShaderMaterialCB) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initStaticConstantBuffers();
	void initDynamicMaterialConstantBuffer(const Graphics::Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void fromMaterialUploadHeapToDefaulHeap(ID3D12GraphicsCommandList* commandList);
	void updateGroupMaterial(const Graphics::Scene::BaseScene& scene, const Graphics::Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList);

	void updateVertexShaderCB(ForwardLightningPushArgs& data, int frameIndex);
	void updatePixelShaderLightsCB(ForwardLightningPushArgs& data, int frameIndex);

	PipelineStatePtr m_mainPSO;

	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[swapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[swapChainBufferCount];

	// pixel shader lights constant buffer
	PixelShaderLightsCB m_pixelShaderLightsCB;
	CComPtr<ID3D12Resource> m_pixelShaderLightsCBUploadHeaps[swapChainBufferCount];
	UINT8* m_pixelShaderLightsCBGPUAddress[swapChainBufferCount];

	// pixel shader material constant buffer
	UINT8* m_pixelShaderMaterialCBGPUAddress[swapChainBufferCount];
	ID3D12Resource* m_pixelShaderMaterialCBUploadHeaps[swapChainBufferCount];
	ID3D12Resource* m_pixelShaderMaterialCBDefaultHeaps[swapChainBufferCount];
};
}}}}}