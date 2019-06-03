//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "TSimpleColor.h"
#include "Scene/BaseScene.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12 { namespace Effect
{
struct RenderGizmoPushArgs
{
	const Scene::BaseScene& scene;
};

template <nbUint32 PixelCBElementCount>
class TRenderGizmo : public TSimpleColor<RenderGizmoPushArgs&, PixelCBElementCount>
{
public:
	TRenderGizmo(const DXGI_SAMPLE_DESC& sampleDesc)
	: TSimpleColor<RenderGizmoPushArgs&, PixelCBElementCount>(sampleDesc)
	{
	}

protected:
	static const DirectX::XMFLOAT4 s_xAxisColor;
	static const DirectX::XMFLOAT4 s_yAxisColor;
	static const DirectX::XMFLOAT4 s_zAxisColor;
	static const DirectX::XMFLOAT4 s_hoverColor;
	static const DirectX::XMFLOAT4 s_whiteColor;

	void updateVertexShaderConstantBuffer(const Camera::SharedCameraPtr& camera, const Gizmo::BaseGizmo* gizmo, nbInt32 frameIndex);
};

template <nbUint32 PixelCBElementCount>
void TRenderGizmo<PixelCBElementCount>::updateVertexShaderConstantBuffer(const Camera::SharedCameraPtr& camera, const Gizmo::BaseGizmo* gizmo, nbInt32 frameIndex)
{
	using namespace DirectX;
	const glm::vec3 gizmoPos = gizmo->getPosition();
	const nbFloat32 distance = glm::length(camera->getPosition() - gizmoPos);

	// Compute frustum z-planes that make sure gizmo is always visible.
	static const nbFloat32 delta = 1e4f;
	const nbFloat32 nearZ = std::min(std::max(0.001f, distance - delta), 1e6f);
	const nbFloat32 farZ = distance + delta;

	glm::mat4 glmModelMatrix = glm::translate(glm::mat4(), gizmoPos);

	glmModelMatrix = glm::scale(
		glmModelMatrix,
		glm::vec3(gizmo->getScale())
	);

	const XMMATRIX modelMatrix = XMMATRIX(&glmModelMatrix[0][0]);
	const XMMATRIX projectionMatrix = camera->getDirectXPerspectiveMatrix(nearZ, farZ);
	const XMMATRIX viewMatrix = camera->getDirectXViewMatrix();

	const XMMATRIX mvp = XMMatrixTranspose(modelMatrix * viewMatrix * projectionMatrix);

	VertexShaderCB vertexShaderCB;
	XMStoreFloat4x4(&vertexShaderCB.wvpMat, mvp);
	memcpy(m_vertexShaderCBGPUAddress[frameIndex], &vertexShaderCB, sizeof(VertexShaderCB));
}

template <nbUint32 PixelCBElementCount>
const DirectX::XMFLOAT4 TRenderGizmo<PixelCBElementCount>::s_xAxisColor = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.8f);

template <nbUint32 PixelCBElementCount>
const DirectX::XMFLOAT4 TRenderGizmo<PixelCBElementCount>::s_yAxisColor = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.8f);

template <nbUint32 PixelCBElementCount>
const DirectX::XMFLOAT4 TRenderGizmo<PixelCBElementCount>::s_zAxisColor = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.8f);

template <nbUint32 PixelCBElementCount>
const DirectX::XMFLOAT4 TRenderGizmo<PixelCBElementCount>::s_hoverColor = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.8f);

template <nbUint32 PixelCBElementCount>
const DirectX::XMFLOAT4 TRenderGizmo<PixelCBElementCount>::s_whiteColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.8f);

}}}}}