//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017 - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential
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
struct CubeMappingPushArgs
{
	const Graphics::Scene& scene;
};

class CubeMappingEffect : public BaseEffect<CubeMappingPushArgs&>
{
public:
	CubeMappingEffect(const SharedDevicePtr& device, const DXGI_SAMPLE_DESC& sampleDesc);
	void pushDrawCommands(CubeMappingPushArgs& data, ID3D12GraphicsCommandList* commandList, int frameIndex) override;

private:
	ORION_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 wvpMat;
		DirectX::XMFLOAT3 cubeMapCenter;
	};

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderCB();
	void updateVertexShaderCB(CubeMappingPushArgs& data, int frameIndex);

	PipelineStatePtr m_PSO;

	// vertex shader constant buffer
	VertexShaderCB m_vertexShaderCB;
	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[swapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[swapChainBufferCount];
};
}}}}}