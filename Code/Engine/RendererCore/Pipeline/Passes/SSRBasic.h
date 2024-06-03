#pragma once

#include <Foundation/Reflection/Reflection.h>

#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererCore/Shader/ShaderResource.h>

class EZ_RENDERERCORE_DLL ezSSRBasicPass : public ezRenderPipelinePass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSSRBasicPass, ezRenderPipelinePass);

public:
  ezSSRBasicPass();
  ~ezSSRBasicPass() override;

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  bool GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs) override;

  void Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;
  void ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) override;

protected:
  void UpdateSSRConstantBuffer() const;
  void UpdateBlurConstantBuffer() const;

  float m_fRoughnessCutoff;
  float m_fBlurStrength;

  ezRenderPipelineNodePassThrougPin m_PinSceneColor;
  ezRenderPipelineNodeInputPin m_PinInputDepth;
  ezRenderPipelineNodeInputPin m_PinInputMaterial;
  ezRenderPipelineNodeInputPin m_PinInputVelocity;

  ezGALTextureHandle m_hTextureSSR;
  ezGALTextureHandle m_hTextureSSR2;
  ezGALTextureHandle m_hTextureSSRBlur;
  ezGALTextureHandle m_hTextureRoughness;
  ezGALTextureHandle m_hTextureOutput;

  ezTexture2DResourceHandle  m_hBlueNoiseTexture;

  ezShaderResourceHandle m_hSSRShader;
  ezShaderResourceHandle m_hSSRAntiflicker;
  ezShaderResourceHandle m_hSSRBlur;

  ezConstantBufferStorageHandle m_hSSRConstantBuffer;
  ezConstantBufferStorageHandle m_hBlurConstantBuffer;
};
