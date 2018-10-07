//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Graphics/Renderer/Realtime/RealtimeRenderer.h"
#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct HighlightColorPushArgs
{
	const Scene::BaseScene& scene;
	const Graphics::Model::Mesh* selection;
};

class HighlightColor : public BaseEffect<HighlightColorPushArgs&>
{
public:
	HighlightColor(const DXGI_SAMPLE_DESC& sampleDesc);
	void pushDrawCommands(HighlightColorPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	static constexpr nbInt32 s_nbPasses = 2;

	KRONOS_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
		DirectX::XMFLOAT3 eyePosition;
		FLOAT scale;
	};

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderCB();
	void updateVertexShaderCB(HighlightColorPushArgs& data, nbInt32 frameIndex, nbInt32 passIndex);

	PipelineStatePtr m_PSOs[s_nbPasses];
	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[s_nbPasses][swapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[s_nbPasses][swapChainBufferCount];
};
}}}}}