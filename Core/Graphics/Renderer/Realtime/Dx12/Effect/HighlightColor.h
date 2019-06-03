//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct HighlightColorPushArgs
{
	const Scene::BaseScene& scene;
};

class HighlightColor : public BaseEffect<HighlightColorPushArgs&>
{
public:
	HighlightColor(const DXGI_SAMPLE_DESC& sampleDesc, ResourceArray& loadingResources, ID3D12GraphicsCommandList* commandList);
	~HighlightColor();

	void pushDrawCommands(HighlightColorPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;
	void resetBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);

private:
	static constexpr nbUint32 s_nbPSO = 2;
	static constexpr nbUint32 s_nbPasses = 3;
	static constexpr nbUint32 s_maxColor = 2;

	NEBULA_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT3 eyePosition;
		FLOAT scale;
		UINT passIdx;
	};

	struct PixelShaderCB
	{
		DirectX::XMFLOAT4 color;
	};

	nbUint32 VertexShaderCBAlignedSize = NEBULA_DX12_ALIGN_SIZE(VertexShaderCB);
	nbUint32 PixelShaderCBAlignedSize = NEBULA_DX12_ALIGN_SIZE(PixelShaderCB);
	
	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initPixelShaderCB(ResourceArray& loadingResources, ID3D12GraphicsCommandList* commandList);
	void pushDrawCommandsInternal(HighlightColorPushArgs& data, const EntityIdentifier& entityId, ID3D12GraphicsCommandList* commandList, nbInt32 colorIndex, nbInt32 frameIndex);

	void freeVertexShaderCBs();

	void updateVertexShaderSharedCB(HighlightColorPushArgs& data,
		ID3D12GraphicsCommandList* commandList,
		const Model::DatabaseMeshGroupPtr& meshGroup,
		nbUint32 objectIdx,
		nbUint32 frameIndex,
		nbUint32 passIndex);

	PipelineStatePtr m_PSOs[s_nbPSO];

	ID3D12Resource* m_vertexShaderCBUploadHeaps[s_maxColor][s_nbPasses][SwapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[s_maxColor][s_nbPasses][SwapChainBufferCount];

	std::unordered_map<EntityIdentifier, nbUint32> m_groupVertexShaderCBPositions;

	CComPtr<ID3D12Resource> m_pixelShaderCBDefaultHeaps[s_maxColor];
};
}}}}}