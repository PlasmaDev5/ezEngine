#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SSRAdvanced.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/PostprocessConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/CopyConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/SSR/SSRConstants.h"

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSSRAdvancedPass, 1, ezRTTIDefaultAllocator<ezSSRAdvancedPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SceneColor", m_PinSceneColor),
    EZ_MEMBER_PROPERTY("Material", m_PinInputMaterial),
    EZ_MEMBER_PROPERTY("Velocity", m_PinInputVelocity),
    EZ_MEMBER_PROPERTY("Depth", m_PinInputDepth),

    EZ_MEMBER_PROPERTY("RoughnessCutoff", m_fRoughnessCutoff)->AddAttributes(new ezDefaultValueAttribute(0.8f)),
    EZ_MEMBER_PROPERTY("BlurStrength", m_fBlurStrength)->AddAttributes(new ezDefaultValueAttribute(50.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Alpha),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSSRAdvancedPass::ezSSRAdvancedPass()
  : ezRenderPipelinePass("SSRPass", true),
  m_fRoughnessCutoff(0.8f),
  m_fBlurStrength(50.0f)
{
  {
    m_hShaderDepthHierarchy = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRDepthHierarchy.ezShader");
    EZ_ASSERT_DEV(m_hShaderDepthHierarchy.IsValid(), "SSR: Could not load depth hierarchy shader!");
  }
  {
    m_hSSRShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRTracePass.ezShader");
    EZ_ASSERT_DEV(m_hSSRShader.IsValid(), "Could not load SSR shader!");
  }
  {
    m_hSSRAntiflicker = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSR/SSRAntiflicker.ezShader");
    EZ_ASSERT_DEV(m_hSSRAntiflicker.IsValid(), "Could not load SSR Antiflicker shader!");
  }
  {
    m_hSSRBlur = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSR/SSRBlur.ezShader");
    EZ_ASSERT_DEV(m_hSSRBlur.IsValid(), "Could not load SSR Blur shader!");
  }
  {
    m_hSSRConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSSRConstants>();
    m_hBlurConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezBlurConstants>();
    m_hPostProcessConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezPostProcessingConstants>();
  }
}

ezSSRAdvancedPass::~ezSSRAdvancedPass()
{
  {
    ezRenderContext::DeleteConstantBufferStorage(m_hSSRConstantBuffer);
    m_hSSRConstantBuffer.Invalidate();
  }
  {
    ezRenderContext::DeleteConstantBufferStorage(m_hBlurConstantBuffer);
    m_hBlurConstantBuffer.Invalidate();
  }
}

ezResult ezSSRAdvancedPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fRoughnessCutoff;
  inout_stream << m_fBlurStrength;

  return EZ_SUCCESS;
}

ezResult ezSSRAdvancedPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_fRoughnessCutoff;
  inout_stream >> m_fBlurStrength;

  return EZ_SUCCESS;
}

bool ezSSRAdvancedPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Final
  if (inputs[m_PinSceneColor.m_uiInputIndex])
  {
    outputs[m_PinSceneColor.m_uiOutputIndex] = *inputs[m_PinSceneColor.m_uiInputIndex];
    if (!inputs[m_PinSceneColor.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' color input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No velocity input connected to pass '{0}'.", GetName());
    return false;
  }

  // Material
  if (inputs[m_PinInputMaterial.m_uiInputIndex])
  {
    if (!inputs[m_PinInputMaterial.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' color input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No velocity input connected to pass '{0}'.", GetName());
    return false;
  }

  // Depth
  if (inputs[m_PinInputDepth.m_uiInputIndex])
  {
    if (!inputs[m_PinInputDepth.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' Depth input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No Depth input connected to '{0}'!", GetName());
    return false;
  }

  // Velocity
  if (inputs[m_PinInputVelocity.m_uiInputIndex])
  {
    if (!inputs[m_PinInputVelocity.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' velocity input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No velocity input connected to pass '{0}'.", GetName());
    return false;
  }

  {
    ezGALTextureCreationDescription desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_uiWidth = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)desc.m_uiWidth)));
    desc.m_uiHeight = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)desc.m_uiHeight)));
    desc.m_uiMipLevelCount = 1 + (uint32_t)std::floor(std::log2f(std::max((float)desc.m_uiWidth, (float)desc.m_uiHeight)));
    desc.m_Format = ezGALResourceFormat::RGFloat;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hDepthHierarchy = pDevice->CreateTexture(desc);
    m_hDepthHierarchy2 = pDevice->CreateTexture(desc);
  }

  {
    ezGALTextureCreationDescription desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    outputs[m_PinSceneColor.m_uiOutputIndex] = desc;
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_Format = ezGALResourceFormat::RGBAFloat;
    desc.m_uiWidth = desc.m_uiWidth;
    desc.m_uiHeight = desc.m_uiHeight;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hTextureSSR = pDevice->CreateTexture(desc);
    m_hTextureSSR2 = pDevice->CreateTexture(desc);
    m_hTextureRoughness = pDevice->CreateTexture(desc);
    m_hTextureSSRBlur = pDevice->CreateTexture(desc);
    m_hTextureOutput = pDevice->CreateTexture(desc);
    m_tmp = pDevice->CreateTexture(desc);
  }

  {
    m_hBlueNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/BlueNoise.dds");
  }

  return true;
}

void ezSSRAdvancedPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputFinal = inputs[m_PinSceneColor.m_uiOutputIndex];
  const auto* const pInputVelocity = inputs[m_PinInputVelocity.m_uiInputIndex];
  const auto* const pInputDepth = inputs[m_PinInputDepth.m_uiInputIndex];
  const auto* const pInputMaterial = inputs[m_PinInputMaterial.m_uiInputIndex];

  if (pInputFinal == nullptr || pInputVelocity == nullptr || pInputDepth == nullptr || pInputMaterial == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezResourceManager::ForceLoadResourceNow(m_hSSRShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  ezGALPass* pPass = pDevice->BeginPass(GetName());
  {
    // Clear
    {
      ezGALRenderingSetup renderingSetup;
      renderingSetup.m_ClearColor = ezColor::Black;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_tmp));

      auto pCommandEncoder = ezRenderContext::BeginRenderingScope(pPass, renderViewContext, renderingSetup, "SSR Clear");
    }

    ezUInt32 hierarchyWidth = (ezUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pInputDepth->m_Desc.m_uiWidth )));
    ezUInt32 hierarchyHeight = (ezUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pInputDepth->m_Desc.m_uiHeight)));

    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "Depth Hierarchy");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderDepthHierarchy);
      {

        ezGALTextureUnorderedAccessViewHandle hOutput;
        {
          ezGALTextureUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = m_hDepthHierarchy;
          desc.m_uiMipLevelToUse = 0;
          hOutput = pDevice->CreateUnorderedAccessView(desc);
        }

        renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));

        auto* postProcessConst = ezRenderContext::GetConstantBufferData<ezPostProcessingConstants>(m_hPostProcessConstantBuffer);
        postProcessConst->params0.x = hierarchyWidth;
        postProcessConst->params0.y = hierarchyHeight;
        postProcessConst->params0.z = 1.0;

        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_DEPTH_MODE", "SSR_DEPTH_MODE_FIRST_PASS");

        const ezUInt32 uiDispatchX = ezMath::Max(1u, (ezUInt32)hierarchyWidth / POSTPROCESS_BLOCKSIZE);
        const ezUInt32 uiDispatchY = ezMath::Max(1u, (ezUInt32)hierarchyHeight / POSTPROCESS_BLOCKSIZE);

        renderViewContext.m_pRenderContext->BindConstantBuffer("ezPostProcessingConstants", m_hPostProcessConstantBuffer);

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

        pCommandEncoder->CopyTexture(m_hDepthHierarchy2, m_hDepthHierarchy);
      }

      {
        ezUInt32 mipCount = 1 + (uint32_t)std::floor(std::log2f(std::max((float)hierarchyWidth, (float)hierarchyHeight)));
        for (ezUInt32 i = 1; i < mipCount; i++)
        {
          ezGALTextureUnorderedAccessViewHandle hOutput;
          {
            ezGALTextureUnorderedAccessViewCreationDescription desc;
            desc.m_hTexture = m_hDepthHierarchy;
            desc.m_uiMipLevelToUse = i;
            hOutput = pDevice->CreateUnorderedAccessView(desc);
          }

          hierarchyWidth /= 2;
          hierarchyHeight /= 2;

          hierarchyWidth = ezMath::Max(1u, hierarchyWidth);
          hierarchyHeight = ezMath::Max(1u, hierarchyHeight);

          renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
          renderViewContext.m_pRenderContext->BindTexture2D("Input", pDevice->GetDefaultResourceView(m_hDepthHierarchy2));

          auto* postProcessConst = ezRenderContext::GetConstantBufferData<ezPostProcessingConstants>(m_hPostProcessConstantBuffer);
          postProcessConst->params0.x = hierarchyWidth;
          postProcessConst->params0.y = hierarchyHeight;
          postProcessConst->params0.z = 1.0;

          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_DEPTH_MODE", "SSR_DEPTH_MODE_SECOND_PASS");

          const ezUInt32 uiDispatchX = ezMath::Max(1u, (ezUInt32)hierarchyWidth / POSTPROCESS_BLOCKSIZE);
          const ezUInt32 uiDispatchY = ezMath::Max(1u, (ezUInt32)hierarchyHeight / POSTPROCESS_BLOCKSIZE);

          renderViewContext.m_pRenderContext->BindConstantBuffer("ezPostProcessingConstants", m_hPostProcessConstantBuffer);

          renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

          pCommandEncoder->CopyTexture(m_hDepthHierarchy2, m_hDepthHierarchy);
        }
      }
    }

    // SSR Pass
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "Trace");

      renderViewContext.m_pRenderContext->BindShader(m_hSSRShader);

      ezGALTextureUnorderedAccessViewHandle hOutputSSR;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureSSR;
        desc.m_uiMipLevelToUse = 0;
        hOutputSSR = pDevice->CreateUnorderedAccessView(desc);
      }

      ezGALTextureUnorderedAccessViewHandle hOutputRoughness;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureRoughness;
        desc.m_uiMipLevelToUse = 0;
        hOutputRoughness = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("SSROutput", hOutputSSR);
      renderViewContext.m_pRenderContext->BindUAV("RoughnessOutput", hOutputRoughness);
      renderViewContext.m_pRenderContext->BindTexture2D("ColorInput", pDevice->GetDefaultResourceView(pInputFinal->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("velocityTexture", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("materialTexture", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("depthTexture", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthInput", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthHierarchy", pDevice->GetDefaultResourceView(m_hDepthHierarchy2));
      renderViewContext.m_pRenderContext->BindTexture2D("BlueNoise",m_hBlueNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);
      renderViewContext.m_pRenderContext->BindConstantBuffer("ezSSRConstants", m_hSSRConstantBuffer);

      const ezUInt32 uiWidth = pInputFinal->m_Desc.m_uiWidth;
      const ezUInt32 uiHeight = pInputFinal->m_Desc.m_uiHeight;

      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      UpdateSSRConstantBuffer();

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    // Antiflicker Pass
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "Antiflicker");

      renderViewContext.m_pRenderContext->BindShader(m_hSSRAntiflicker);

      ezGALTextureUnorderedAccessViewHandle hOutputSSR;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureSSR2;
        desc.m_uiMipLevelToUse = 0;
        hOutputSSR = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutputSSR);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(m_hTextureSSR));

      const ezUInt32 uiWidth = pInputFinal->m_Desc.m_uiWidth;
      const ezUInt32 uiHeight = pInputFinal->m_Desc.m_uiHeight;

      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    // Blur Pass 1
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "Blur");
      renderViewContext.m_pRenderContext->BindShader(m_hSSRBlur);

      ezGALTextureUnorderedAccessViewHandle hOutputSSR;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureSSRBlur;
        desc.m_uiMipLevelToUse = 0;
        hOutputSSR = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_RADIUS_FORMAT", "BLUR_RADIUS_FORMAT_TEXTURE");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_PASS", "BLUR_PASS_GAUSSIAN_BILATERAL");

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutputSSR);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(m_hTextureSSR));
      renderViewContext.m_pRenderContext->BindTexture2D("BlurTexture", pDevice->GetDefaultResourceView(m_hTextureRoughness));
      renderViewContext.m_pRenderContext->BindTexture2D("materialTexture", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("depthTexture", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("ezBlurConstants", m_hBlurConstantBuffer);

      UpdateBlurConstantBuffer();

      const ezUInt32 uiWidth = pInputFinal->m_Desc.m_uiWidth;
      const ezUInt32 uiHeight = pInputFinal->m_Desc.m_uiHeight;

      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    // Blur Pass 1
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "Blur");
      renderViewContext.m_pRenderContext->BindShader(m_hSSRBlur);

      ezGALTextureUnorderedAccessViewHandle hOutputSSR;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_tmp;
        desc.m_uiMipLevelToUse = 0;
        hOutputSSR = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_RADIUS_FORMAT", "BLUR_RADIUS_FORMAT_TEXTURE");
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_PASS", "BLUR_PASS_GAUSSIAN_BILATERAL");

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutputSSR);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(m_hTextureSSRBlur));
      renderViewContext.m_pRenderContext->BindTexture2D("BlurTexture", pDevice->GetDefaultResourceView(m_hTextureRoughness));
      renderViewContext.m_pRenderContext->BindTexture2D("materialTexture", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("depthTexture", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("ezBlurConstants", m_hBlurConstantBuffer);

      UpdateBlurConstantBuffer();

      const ezUInt32 uiWidth = pInputFinal->m_Desc.m_uiWidth;
      const ezUInt32 uiHeight = pInputFinal->m_Desc.m_uiHeight;

      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    pDevice->EndPass(pPass);

    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
  }
}

void ezSSRAdvancedPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputFinal = inputs[m_PinSceneColor.m_uiInputIndex];

  const ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALTexture* pDest = pDevice->GetTexture(pInputFinal->m_TextureHandle);

  if (const ezGALTexture* pSource = pDevice->GetTexture(pInputFinal->m_TextureHandle); pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    ezLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = ezRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pInputFinal->m_TextureHandle, pInputFinal->m_TextureHandle);
  }
}

void ezSSRAdvancedPass::UpdateSSRConstantBuffer() const
{
  auto* constants = ezRenderContext::GetConstantBufferData<ezSSRConstants>(m_hSSRConstantBuffer);
  constants->RoughnessCutoff = m_fRoughnessCutoff;
  constants->Frame = ezRenderWorld::GetFrameCounter();
}

void ezSSRAdvancedPass::UpdateBlurConstantBuffer() const
{
  auto* constants = ezRenderContext::GetConstantBufferData<ezBlurConstants>(m_hBlurConstantBuffer);
  constants->BlurRadius = m_fBlurStrength;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SSRAdvancedPass);
