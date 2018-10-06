//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2018
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: nebularender@gmail.com
//========================================================================

#pragma once

#include "Graphics/Texture/Image.h"
#include "Graphics/Vertex.h"
#include <vector>

#define TGRAPHIC_ALLOC_PARAMETER_LIST typename TextureHandle,\
									  typename VertexBufferHandle,\
									  typename IndexBufferHandle

#define TGRAPHIC_ALLOC_PARAMETERS TextureHandle,  \
								  VertexBufferHandle, \
								  IndexBufferHandle

namespace Graphics { namespace Renderer { namespace Realtime
{
template <TGRAPHIC_ALLOC_PARAMETER_LIST>
class TGraphicResourceAllocator
{
public:
	virtual void createRGBATexture2D(const Texture::RGBAImage* image, TextureHandle& dst) = 0;
	virtual void createRGBATexture2DArray(const std::vector<const Texture::RGBAImage*>& images, TextureHandle& dst) = 0;
	virtual kBool releaseTexture(const TextureHandle& textureHandle) const = 0;

	virtual kBool createVertexBuffer(const void* data, VertexBufferHandle& dst, kUint32 sizeofElem, kUint32 count) = 0;;
	virtual kBool releaseVertexBuffer(const VertexBufferHandle& arrayBufferHandle) const = 0;

	virtual kBool createIndexBuffer(const std::vector<kUint32>& data, IndexBufferHandle& dst) = 0;
	virtual kBool releaseIndexBuffer(const IndexBufferHandle& arrayBufferHandle) const = 0;

	template<typename T>
	kBool createVertexBuffer(const std::vector<T>& data, VertexBufferHandle& dst)
	{
		return createVertexBuffer(data.data(), dst, sizeof(T), (kUint32)data.size());
	}
};

template <TGRAPHIC_ALLOC_PARAMETER_LIST>
extern TGraphicResourceAllocator<TGRAPHIC_ALLOC_PARAMETER_LIST>* GraphicResourceAllocatorPtr = nullptr;
}}}