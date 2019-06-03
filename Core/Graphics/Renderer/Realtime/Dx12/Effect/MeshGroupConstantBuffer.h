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

	void resetBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);
	void onUpdateMeshGroup(const Scene::BaseScene& scene, const EntityIdentifier& groupId, ID3D12GraphicsCommandList* commandList);

	struct VertexShaderGroupCB
	{
		DirectX::XMFLOAT4X4 modelMat;
		UINT groupId;
	};

	D3D12_GPU_VIRTUAL_ADDRESS getMeshGroupGPUVirtualAddress(const EntityIdentifier& entityId);

	static const nbUint32 VertexShaderCBAlignedSize = NEBULA_DX12_ALIGN_SIZE(VertexShaderGroupCB);

private:
	void fromVertexShaderUploadToDefaulHeap(ID3D12GraphicsCommandList* commandList);
	void updateMeshGroup(const Scene::BaseScene& scene, const EntityIdentifier& groupId, ID3D12GraphicsCommandList* commandList);
	void initVertexShaderConstantBuffer(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList);

	// vertex shader constant buffer
	UINT8* m_vertexShaderCBGPUAddress;
	ID3D12Resource* m_vertexShaderCBUploadHeap;
	ID3D12Resource* m_vertexShaderCBDefaultHeap;

	std::unordered_map<EntityIdentifier, nbUint32> m_groupCBPositions;
};

inline void MeshGroupConstantBuffer::resetBuffers(const Scene::BaseScene& scene, ID3D12GraphicsCommandList* commandList)
{
	initVertexShaderConstantBuffer(scene, commandList);
}

inline D3D12_GPU_VIRTUAL_ADDRESS MeshGroupConstantBuffer::getMeshGroupGPUVirtualAddress(const EntityIdentifier& entityId)
{
	return m_vertexShaderCBDefaultHeap->GetGPUVirtualAddress() + m_groupCBPositions[entityId];
}

using MeshGroupConstantBufferSingleton = Utilities::Singleton<MeshGroupConstantBuffer>;
}}}}}
