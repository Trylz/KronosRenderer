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
class MeshGroupConstantBuffer
{
public:
	MeshGroupConstantBuffer();
	~MeshGroupConstantBuffer();

	void onNewScene(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void onUpdateMeshGroup(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList);

	struct VertexShaderGroupCB
	{
		DirectX::XMFLOAT4X4 modelMat;
		UINT groupId;
	};

	ID3D12Resource* getDefaultHeap();

	static const nbUint32 VertexShaderCBAlignedSize = NEBULA_DX12_ALIGN_SIZE(VertexShaderGroupCB);

private:
	void fromVertexShaderUploadToDefaulHeap(ID3D12GraphicsCommandList* commandList);
	void updateMeshGroup(const Scene::BaseScene& scene, const Model::MeshGroupId& groupId, ID3D12GraphicsCommandList* commandList);
	void initVertexShaderConstantBuffer(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);

	// vertex shader constant buffer
	UINT8* m_vertexShaderCBGPUAddress;
	ID3D12Resource* m_vertexShaderCBUploadHeap;
	ID3D12Resource* m_vertexShaderCBDefaultHeap;
};

inline void MeshGroupConstantBuffer::onNewScene(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	initVertexShaderConstantBuffer(scene, commandList);
}

inline ID3D12Resource* MeshGroupConstantBuffer::getDefaultHeap()
{
	return m_vertexShaderCBDefaultHeap;
}

using MeshGroupConstantBufferSingleton = Utilities::Singleton<MeshGroupConstantBuffer>;
}}}}}
