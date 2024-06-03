#pragma once

#include "../../Common/ConstantBufferMacros.h"
#include "../../Common/Platforms.h"

#define POSTPROCESS_BLOCKSIZE 8
#define SSR_TILESIZE 32

CONSTANT_BUFFER(ezSSRConstants, 4)
{
  FLOAT1(RoughnessCutoff);
  FLOAT1(Frame);
};
