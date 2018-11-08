//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "BaseEffect.h"
#include "Graphics/Texture/TCubeMap.h"
#include "Scene/BaseScene.h"

#include <DirectXMath.h>

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 {
	
using DX12CubeMap = Graphics::Texture::TCubeMap<DX12_GRAPHIC_ALLOC_PARAMETERS>;

namespace Effect {

struct CubeMappingPushArgs
{
	const Scene::BaseScene& scene;
};

class CubeMapping : public BaseEffect<CubeMappingPushArgs&>
{
public:
	CubeMapping(const DXGI_SAMPLE_DESC& sampleDesc);
	void pushDrawCommands(CubeMappingPushArgs& data, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex) override;

private:
	NEBULA_DX12_ATTRIBUTE_ALIGN struct VertexShaderCB
	{
		DirectX::XMFLOAT3 cubeMapCenter;
	};

	void initRootSignature() override;
	void initPipelineStateObjects() override;
	void initVertexShaderCB();
	void updateVertexShaderCB(CubeMappingPushArgs& data, nbInt32 frameIndex);

	PipelineStatePtr m_PSO;

	CComPtr<ID3D12Resource> m_vertexShaderCBUploadHeaps[SwapChainBufferCount];
	UINT8* m_vertexShaderCBGPUAddress[SwapChainBufferCount];
};
}}}}}