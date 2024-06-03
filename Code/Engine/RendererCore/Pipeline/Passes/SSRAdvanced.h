#pragma once

#include <Foundation/Reflection/Reflection.h>

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

struct ezPostprocessTileStatistics;

class EZ_RENDERERCORE_DLL ezSSRAdvancedPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSSRAdvancedPass, ezRenderPipelinePass);

public:
  ezSSRAdvancedPass();
  ~ezSSRAdvancedPass() override;

  bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;


protected:
  void UpdateSSRConstantBuffer() const;
  void UpdateBlurConstantBuffer() const;

  float m_fMaxDist;
  float m_fResolution;
  float m_fThickness;
  int m_iSteps;

  ezRenderPipelineNodePassThrougPin m_PinSceneColor;
  ezRenderPipelineNodeInputPin m_PinInputDepth;
  ezRenderPipelineNodeInputPin m_PinInputMaterial;
  ezRenderPipelineNodeInputPin m_PinInputVelocity;

  ezConstantBufferStorageHandle m_hSSRConstantBuffer;
  ezConstantBufferStorageHandle m_hPostProcessConstantBuffer;
  ezShaderResourceHandle m_hShaderTileMinMaxRoughnessHorizontal;
  ezShaderResourceHandle m_hShaderTileMinMaxRoughnessVerticle;
  ezShaderResourceHandle m_hShaderDepthHierarchy;
  ezShaderResourceHandle m_hShaderSSRTrace;

  ezGALTextureHandle m_hTextureTileMinMaxRoughnessHorizontal;
  ezGALTextureHandle m_hTileMinMaxRoughness;
  ezGALTextureHandle m_hDepthHierarchy;
  ezGALTextureHandle m_hDepthHierarchyTmp;
  ezGALTextureHandle m_hIndirectSpecular;
  ezGALTextureHandle m_hDirectionPDF;
  ezGALTextureHandle m_hRayLength;

  ezTexture2DResourceHandle m_hBlueNoiseTexture;

  ezGALBufferHandle m_hTileStatisticsBuffer;
  ezArrayPtr<ezPostprocessTileStatistics> m_TileStatistics;

  ezGALBufferHandle m_hTilesTracingEarlyexitBuffer;
  ezGALBufferHandle m_hTilesTracingCheapBuffer;
  ezGALBufferHandle m_hTilesTracingExpensiveBuffer;
  ezArrayPtr<ezUInt32> m_TilesTracingEarlyexit;
  ezArrayPtr<ezUInt32> m_TilesTracingCheap;
  ezArrayPtr<ezUInt32> m_TilesTracingExpensive;

  float m_MinMaxRoughnessWidth;
  float m_MinMaxRoughnessHeight;
};
