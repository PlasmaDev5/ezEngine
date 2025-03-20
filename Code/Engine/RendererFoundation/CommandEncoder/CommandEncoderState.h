
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct ezGALVertexBufferBinding
{
  ezGALBufferHandle m_hBuffer;
  ezUInt32 m_uiOffsetInBytes = 0;
  ezGALVertexBufferStepMode::Enum m_StepMode = ezGALVertexBufferStepMode::PerVertex;
};

struct ezGALIndexBufferBinding
{
  ezGALBufferHandle m_hBuffer;
  ezUInt32 m_uiOffsetInBytes = 0;
};

struct EZ_RENDERERFOUNDATION_DLL ezGALCommandEncoderRenderState final
{
  void InvalidateState() { *this = {}; }

  ezGALShaderHandle m_hShader;

  ezGALVertexBufferBinding m_VertexBufferBindings[EZ_GAL_MAX_VERTEX_BUFFER_COUNT];
  ezGALIndexBufferBinding m_IndexBufferBinding;  

  ezGALPrimitiveTopology::Enum m_Topology = ezGALPrimitiveTopology::ENUM_COUNT;

  ezGALBlendStateHandle m_hBlendState;
  ezColor m_BlendFactor = ezColor(0, 0, 0, 0);
  ezUInt32 m_uiSampleMask = 0;

  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezUInt8 m_uiStencilRefValue = 0;

  ezGALRasterizerStateHandle m_hRasterizerState;

  ezRectU32 m_ScissorRect = ezRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  ezRectFloat m_ViewPortRect = ezRectFloat(ezMath::MaxValue<float>(), ezMath::MaxValue<float>(), 0.0f, 0.0f);
  float m_fViewPortMinDepth = ezMath::MaxValue<float>();
  float m_fViewPortMaxDepth = -ezMath::MaxValue<float>();
};
