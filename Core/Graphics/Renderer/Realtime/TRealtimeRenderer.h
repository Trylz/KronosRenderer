//========================================================================
// Copyright (c) Yann Clotioloman Yeo, 2017
//
//	Author					: Yann Clotioloman Yeo
//	E-Mail					: kronosrenderer@gmail.com
//========================================================================

#pragma once

#include "RealtimeRenderer.h"
#include "TGraphicResourceAllocator.h"

#define TREALTIME_RENDERER_PARAMETER_LIST TGRAPHIC_ALLOC_PARAMETER_LIST, typename InitDataType
#define TREALTIME_RENDERER_PARAMETERS TGRAPHIC_ALLOC_PARAMETERS, InitDataType

namespace Graphics { namespace Renderer { namespace Realtime
{
template <TREALTIME_RENDERER_PARAMETER_LIST>
class TRealtimeRenderer : TGraphicResourceAllocator<TGRAPHIC_ALLOC_PARAMETERS>, public RealtimeRenderer
{
public:
	virtual bool init(const InitDataType& initData) = 0;
};
}}}