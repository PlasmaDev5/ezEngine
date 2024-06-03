#pragma once

#include "../Common/ConstantBufferMacros.h"

struct EZ_SHADER_STRUCT ezPostprocessTileStatistics
{
  UINT1(Dispatch_Earlyexit_ThreadGroupCountX);
  UINT1(Dispatch_Earlyexit_ThreadGroupCountY);
  UINT1(Dispatch_Earlyexit_ThreadGroupCountZ);

  UINT1(Dispatch_Cheap_ThreadGroupCountX);
  UINT1(Dispatch_Cheap_ThreadGroupCountY);
  UINT1(Dispatch_Cheap_ThreadGroupCountZ);

  UINT1(Dispatch_Expensive_ThreadGroupCountX);
  UINT1(Dispatch_Expensive_ThreadGroupCountY);
  UINT1(Dispatch_Expensive_ThreadGroupCountZ);
};

#if EZ_DISABLED(PLATFORM_SHADER)
  EZ_DEFINE_AS_POD_TYPE(ezPostprocessTileStatistics);
#endif

CONSTANT_BUFFER(ezPostProcessingConstants, 3)
{
  FLOAT4(resolution);
  FLOAT4(params0);
  FLOAT4(params1);
};
