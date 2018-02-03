//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: orionrenderer@gmail.com
//========================================================================

#include "stdafx.h"
#include "BaseEffect.h"

namespace Graphics { namespace Renderer { namespace Realtime { namespace Dx12{ namespace Effect
{
	SharedDevicePtr D3d12Device;
	TGraphicResourceAllocator<DX12_GRAPHIC_ALLOC_PARAMETERS>* D3d12ResAllocator;
}}}}}