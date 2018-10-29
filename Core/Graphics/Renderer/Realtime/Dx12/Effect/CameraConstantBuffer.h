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
#include "Utilities/Singleton.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
class CameraConstantBuffer
{
public:
	CameraConstantBuffer();
	~CameraConstantBuffer();

	struct VertexShaderCB
	{
		DirectX::XMFLOAT4X4 vpMat;
	};

	void update(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList, nbInt32 frameIndex);
	
	inline ID3D12Resource** getUploadtHeaps() { return m_vertexShaderCBUploadHeaps; };

	static const nbUint32 VertexShaderSharedCBAlignedSize = NEBULA_DX12_ALIGN_SIZE(VertexShaderCB);

private:
	UINT8* m_vertexShaderCBGPUAddress[swapChainBufferCount];
	ID3D12Resource* m_vertexShaderCBUploadHeaps[swapChainBufferCount];
};

using CameraConstantBufferSingleton = Utilities::Singleton<CameraConstantBuffer>;
}}}}}
