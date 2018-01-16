//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Graphics/Renderer/Realtime/RealtimeRenderer.h"
#include "Graphics/Scene.h"
#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct HighlightColorEffectPushArgs
{
	const Graphics::Scene& scene;
	MeshSelection selection;
};

class HighlightColorEffect : public BaseEffect<HighlightColorEffectPushArgs&>
{
public:
	HighlightColorEffect(const SharedDevicePtr& device, const DXGI_SAMPLE_DESC& sampleDesc);
	void pushDrawCommands(HighlightColorEffectPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex) override;

private:
	static constexpr int s_nbPasses = 2;

	ORION_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
		DirectX::XMFLOAT3 eyePosition;
		FLOAT scale;
	};

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderCB();
	void updateVertexShaderCB(HighlightColorEffectPushArgs& data, int frameIndex, int passIndex);

	// vertex shader constant buffer
	VertexShaderCB m_vertexShaderCB;

	PipelineStatePtr m_PSOs[s_nbPasses];
	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[s_nbPasses][swapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[s_nbPasses][swapChainBufferCount];
};
}}}}}