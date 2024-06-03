#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SSRAdvanced.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/BlurConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/CopyConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/PostprocessConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/SSR/SSRConstants.h"

#include <Foundation/IO/TypeVersionContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSSRAdvancedPass, 1, ezRTTIDefaultAllocator<ezSSRAdvancedPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SceneColor", m_PinSceneColor),
    EZ_MEMBER_PROPERTY("Material", m_PinInputMaterial),
    EZ_MEMBER_PROPERTY("Velocity", m_PinInputVelocity),
    EZ_MEMBER_PROPERTY("Depth", m_PinInputDepth),

    EZ_MEMBER_PROPERTY("MaxDist", m_fMaxDist)->AddAttributes(new ezDefaultValueAttribute(15.0f)),
    EZ_MEMBER_PROPERTY("Resolution", m_fResolution)->AddAttributes(new ezDefaultValueAttribute(0.1f), new ezClampValueAttribute(0, 1)),
    EZ_MEMBER_PROPERTY("Steps", m_iSteps)->AddAttributes(new ezDefaultValueAttribute(10)),
    EZ_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new ezDefaultValueAttribute(0.1f))
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
  : ezRenderPipelinePass("SSRAdvancedPass", true)
  , m_fMaxDist(15.0f)
  , m_fResolution(0.1f)
  , m_iSteps(10)
  , m_fThickness(0.1f)
{
  {
    m_hShaderTileMinMaxRoughnessHorizontal = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRTileMinMaxRoughessHorizontal.ezShader");
    EZ_ASSERT_DEV(m_hShaderTileMinMaxRoughnessHorizontal.IsValid(), "SSR: Could not load min max roughness horizontal shader!");
  }
  {
    m_hShaderTileMinMaxRoughnessVerticle = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRTileMinMaxRoughessVertical.ezShader");
    EZ_ASSERT_DEV(m_hShaderTileMinMaxRoughnessVerticle.IsValid(), "SSR: Could not load min max roughness vertical shader!");
  }
  {
    m_hShaderDepthHierarchy = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRDepthHierarchy.ezShader");
    EZ_ASSERT_DEV(m_hShaderDepthHierarchy.IsValid(), "SSR: Could not load depth hierarchy shader!");
  }
  {
    m_hShaderSSRTrace = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SSRAdvanced/SSRTracePass.ezShader");
    EZ_ASSERT_DEV(m_hShaderSSRTrace.IsValid(), "SSR: Could not load trace shader shader!");
  }
  {
    m_hSSRConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSSRConstants>();
    m_hPostProcessConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezPostProcessingConstants>();
  }
  {
    m_hBlueNoiseTexture = ezResourceManager::LoadResource<ezTexture2DResource>("Textures/BlueNoise.dds");
  }
}

ezSSRAdvancedPass::~ezSSRAdvancedPass()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hTilesTracingEarlyexitBuffer);
  EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingEarlyexit);
  pDevice->DestroyBuffer(m_hTilesTracingCheapBuffer);
  EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingCheap);
  pDevice->DestroyBuffer(m_hTilesTracingExpensiveBuffer);
  EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingExpensive);

  ezRenderContext::DeleteConstantBufferStorage(m_hSSRConstantBuffer);
  m_hSSRConstantBuffer.Invalidate();

  ezRenderContext::DeleteConstantBufferStorage(m_hPostProcessConstantBuffer);
  m_hPostProcessConstantBuffer.Invalidate();
}

ezResult ezSSRAdvancedPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fMaxDist;
  inout_stream << m_fResolution;
  inout_stream << m_iSteps;
  inout_stream << m_fThickness;
  return EZ_SUCCESS;
}

ezResult ezSSRAdvancedPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fMaxDist;
  inout_stream >> m_fResolution;
  inout_stream >> m_iSteps;
  inout_stream >> m_fThickness;
  return EZ_SUCCESS;
}

bool ezSSRAdvancedPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Color
  if (inputs[m_PinSceneColor.m_uiInputIndex])
  {
    if (!inputs[m_PinSceneColor.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' Color input must allow shader resource view.", GetName());
      return false;
    }

    auto desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;
    m_tmp = pDevice->CreateTexture(desc);
    outputs[m_PinSceneColor.m_uiOutputIndex] = *inputs[m_PinSceneColor.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No Color input connected to '{0}'!", GetName());
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

  // Material
  if (inputs[m_PinInputMaterial.m_uiInputIndex])
  {
    if (!inputs[m_PinInputMaterial.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("'{0}' material input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    ezLog::Error("No material input connected to pass '{0}'.", GetName());
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

  ezGALTextureCreationDescription output = outputs[m_PinSceneColor.m_uiOutputIndex];

  m_MinMaxRoughnessWidth = (output.m_uiWidth + SSR_TILESIZE - 1) / SSR_TILESIZE;
  m_MinMaxRoughnessHeight = (output.m_uiHeight + SSR_TILESIZE - 1) / SSR_TILESIZE;
  {
    ezGALTextureCreationDescription desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_uiWidth = m_MinMaxRoughnessWidth;
    desc.m_uiHeight = m_MinMaxRoughnessHeight;
    desc.m_Format = ezGALResourceFormat::RGFloat;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hTileMinMaxRoughness = pDevice->CreateTexture(desc);


    desc.m_uiHeight = output.m_uiHeight;
    m_hTextureTileMinMaxRoughnessHorizontal = pDevice->CreateTexture(desc);
  }

  {
    // because these are all created together can assume 1 valid = all valid
    if (!m_hTilesTracingEarlyexitBuffer.IsInvalidated())
    {
      pDevice->DestroyBuffer(m_hTilesTracingEarlyexitBuffer);
      EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingEarlyexit);
      pDevice->DestroyBuffer(m_hTilesTracingCheapBuffer);
      EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingCheap);
      pDevice->DestroyBuffer(m_hTilesTracingExpensiveBuffer);
      EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TilesTracingExpensive);
    }

    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight * sizeof(ezUInt32);
    desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_TilesTracingEarlyexit = EZ_NEW_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), ezUInt32, 1);
    m_TilesTracingEarlyexit[0] = 0;
    m_TilesTracingCheap = EZ_NEW_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), ezUInt32, 1);
    m_TilesTracingCheap[0] = 0;
    m_TilesTracingExpensive = EZ_NEW_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), ezUInt32, 1);
    m_TilesTracingExpensive[0] = 0;

    m_hTilesTracingEarlyexitBuffer = pDevice->CreateBuffer(desc, m_TilesTracingEarlyexit.ToByteArray());
    m_hTilesTracingCheapBuffer = pDevice->CreateBuffer(desc, m_TilesTracingCheap.ToByteArray());
    m_hTilesTracingExpensiveBuffer = pDevice->CreateBuffer(desc, m_TilesTracingExpensive.ToByteArray());
  }

  {
    ezGALTextureCreationDescription desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_uiWidth = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)output.m_uiWidth / 2)));
    desc.m_uiHeight = (uint32_t)std::pow(2.0f, 1.0f + std::floor(std::log2((float)output.m_uiHeight / 2)));
    desc.m_uiMipLevelCount = 1 + (uint32_t)std::floor(std::log2f(std::max((float)desc.m_uiWidth, (float)desc.m_uiHeight)));
    desc.m_Format = ezGALResourceFormat::RGFloat;
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hDepthHierarchy = pDevice->CreateTexture(desc);
    m_hDepthHierarchyTmp = pDevice->CreateTexture(desc);
  }

  {
    ezGALTextureCreationDescription desc = *inputs[m_PinSceneColor.m_uiInputIndex];
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_bAllowUAV = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_uiWidth = desc.m_uiWidth / 2;
    desc.m_uiHeight = desc.m_uiHeight / 2;
    m_hIndirectSpecular = pDevice->CreateTexture(desc);
    m_hDirectionPDF = pDevice->CreateTexture(desc);

    desc.m_Format = ezGALResourceFormat::RHalf;
    m_hRayLength = pDevice->CreateTexture(desc);
  }
  return true;
}

void ezSSRAdvancedPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputColor = inputs[m_PinSceneColor.m_uiInputIndex];
  const auto* const pInputVelocity = inputs[m_PinInputVelocity.m_uiInputIndex];
  const auto* const pInputDepth = inputs[m_PinInputDepth.m_uiInputIndex];
  const auto* const pInputMaterial = inputs[m_PinInputMaterial.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinSceneColor.m_uiOutputIndex];

  if (pInputColor == nullptr || pInputVelocity == nullptr || pInputDepth == nullptr || pOutput == nullptr || pInputMaterial == nullptr)
  {
    return;
  }


  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezResourceManager::ForceLoadResourceNow(m_hShaderTileMinMaxRoughnessHorizontal);
  ezResourceManager::ForceLoadResourceNow(m_hShaderTileMinMaxRoughnessVerticle);
  ezResourceManager::ForceLoadResourceNow(m_hShaderDepthHierarchy);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  // SSR Clear textures
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_ClearColor = ezColor::Black;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;

    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hTextureTileMinMaxRoughnessHorizontal));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(1, pDevice->GetDefaultRenderTargetView(m_hTileMinMaxRoughness));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(2, pDevice->GetDefaultRenderTargetView(m_hDepthHierarchy));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(3, pDevice->GetDefaultRenderTargetView(m_hIndirectSpecular));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(4, pDevice->GetDefaultRenderTargetView(m_hDirectionPDF));
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(5, pDevice->GetDefaultRenderTargetView(m_tmp));

    auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, "SSR Clear UAV");
  }

  ezGALPass* pPass = pDevice->BeginPass(GetName());
  {
    UpdateSSRConstantBuffer();

    {
      ezGALBufferCreationDescription desc;
      desc.m_uiStructSize = sizeof(ezUInt32);
      desc.m_uiTotalSize = sizeof(ezUInt32) * 9;
      desc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::UnorderedAccess | ezGALBufferUsageFlags::DrawIndirect;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_TileStatistics = EZ_NEW_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), ezPostprocessTileStatistics, 9);

      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountX = 0; // shader atomic
      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Earlyexit_ThreadGroupCountZ = 1;
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountX = 0;     // shader atomic
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Cheap_ThreadGroupCountZ = 1;
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountX = 0; // shader atomic
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountY = 1;
      m_TileStatistics[0].Dispatch_Expensive_ThreadGroupCountZ = 1;

      m_hTileStatisticsBuffer = pDevice->CreateBuffer(desc, m_TileStatistics.ToByteArray());
    }

    // SSR Compute tile classification (horizontal)
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Tile Classification - Horizontal");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderTileMinMaxRoughnessHorizontal);

      ezGALTextureUnorderedAccessViewHandle hOutput;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTextureTileMinMaxRoughnessHorizontal;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("MaterialParamsBuffer", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));

      const ezUInt32 uiWidth = (pOutput->m_Desc.m_uiWidth + SSR_TILESIZE - 1) / SSR_TILESIZE;
      const ezUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }
    // SSR Compute tile classification (verticle)
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Tile Classification - Verticle");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderTileMinMaxRoughnessVerticle);

      ezGALBufferUnorderedAccessViewHandle hTileStatistics;
      {
        ezGALBufferUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTileStatisticsBuffer;
        desc.m_uiNumElements = 9;
        desc.m_uiFirstElement = 0;
        hTileStatistics = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TileTracingStatistics", hTileStatistics);

      ezGALBufferUnorderedAccessViewHandle hTilesTracingEarlyexit;
      {
        ezGALBufferUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingEarlyexitBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingEarlyexit = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingEarlyexit", hTilesTracingEarlyexit);

      ezGALBufferUnorderedAccessViewHandle hTilesTracingCheap;
      {
        ezGALBufferUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingCheapBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingCheap = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingCheap", hTilesTracingCheap);

      ezGALBufferUnorderedAccessViewHandle hTilesTracingExpensive;
      {
        ezGALBufferUnorderedAccessViewCreationDescription desc;
        desc.m_OverrideViewFormat = ezGALResourceFormat::RUInt;
        desc.m_hBuffer = m_hTilesTracingExpensiveBuffer;
        desc.m_uiNumElements = m_MinMaxRoughnessWidth * m_MinMaxRoughnessHeight;
        desc.m_uiFirstElement = 0;
        hTilesTracingExpensive = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("TilesTracingExpensive", hTilesTracingExpensive);

      ezGALTextureUnorderedAccessViewHandle hOutput;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTileMinMaxRoughness;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);

      renderViewContext.m_pRenderContext->BindTexture2D("TileMinMaxRoughnessHorizontal", pDevice->GetDefaultResourceView(m_hTextureTileMinMaxRoughnessHorizontal));
      renderViewContext.m_pRenderContext->BindConstantBuffer("ezSSRConstants", m_hSSRConstantBuffer);

      const ezUInt32 uiDispatchX = (m_MinMaxRoughnessWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      const ezUInt32 uiDispatchY = (m_MinMaxRoughnessHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    ezUInt32 hierarchyWidth = (ezUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pOutput->m_Desc.m_uiWidth / 2)));
    ezUInt32 hierarchyHeight = (ezUInt32)std::pow(2.0f, 1.0f + std::floor(std::log2((float)pOutput->m_Desc.m_uiHeight / 2)));
    // Depth hierarchy
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Depth hierarchy");
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

        pCommandEncoder->CopyTexture(m_hDepthHierarchyTmp, m_hDepthHierarchy);
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
          renderViewContext.m_pRenderContext->BindTexture2D("Input", pDevice->GetDefaultResourceView(m_hDepthHierarchyTmp));

          auto* postProcessConst = ezRenderContext::GetConstantBufferData<ezPostProcessingConstants>(m_hPostProcessConstantBuffer);
          postProcessConst->params0.x = hierarchyWidth;
          postProcessConst->params0.y = hierarchyHeight;
          postProcessConst->params0.z = 1.0;

          renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_DEPTH_MODE", "SSR_DEPTH_MODE_SECOND_PASS");

          const ezUInt32 uiDispatchX = ezMath::Max(1u, (ezUInt32)hierarchyWidth / POSTPROCESS_BLOCKSIZE);
          const ezUInt32 uiDispatchY = ezMath::Max(1u, (ezUInt32)hierarchyHeight / POSTPROCESS_BLOCKSIZE);

          renderViewContext.m_pRenderContext->BindConstantBuffer("ezPostProcessingConstants", m_hPostProcessConstantBuffer);

          renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();

          pCommandEncoder->CopyTexture(m_hDepthHierarchyTmp, m_hDepthHierarchy);
        }
      }
    }
    auto* postProcessConst = ezRenderContext::GetConstantBufferData<ezPostProcessingConstants>(m_hPostProcessConstantBuffer);
    postProcessConst->resolution.x = pOutput->m_Desc.m_uiWidth / 2;
    postProcessConst->resolution.y = pOutput->m_Desc.m_uiHeight / 2;
    postProcessConst->resolution.z = 1.0 / postProcessConst->resolution.x;
    postProcessConst->resolution.w = 1.0 / postProcessConst->resolution.y;

    // Factor to scale ratio between hierarchy and trace pass
    postProcessConst->params1.x = (float)postProcessConst->resolution.x / (float)hierarchyWidth;
    postProcessConst->params1.y = (float)postProcessConst->resolution.y / (float)hierarchyHeight;
    postProcessConst->params1.z = 1.0f / postProcessConst->params1.x;
    postProcessConst->params1.w = 1.0f / postProcessConst->params1.y;

    // SSR Trace Pass
    {
      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR Raytrace pass");
      renderViewContext.m_pRenderContext->BindShader(m_hShaderSSRTrace);

      ezGALTextureUnorderedAccessViewHandle hOutputIndirectSpec;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_tmp;
        desc.m_uiMipLevelToUse = 0;
        hOutputIndirectSpec = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputIndirectSpecular", hOutputIndirectSpec);

      ezGALTextureUnorderedAccessViewHandle hOutputDirectionPDF;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hDirectionPDF;
        desc.m_uiMipLevelToUse = 0;
        hOutputDirectionPDF = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputRayDirectionPDF", hOutputDirectionPDF);

      ezGALTextureUnorderedAccessViewHandle hOutputRayLength;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hRayLength;
        desc.m_uiMipLevelToUse = 0;
        hOutputRayLength = pDevice->CreateUnorderedAccessView(desc);
      }
      renderViewContext.m_pRenderContext->BindUAV("OutputRayLengths", hOutputRayLength);

      renderViewContext.m_pRenderContext->BindTexture2D("Input", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("BlueNoise", m_hBlueNoiseTexture, ezResourceAcquireMode::BlockTillLoaded);
      renderViewContext.m_pRenderContext->BindTexture2D("MaterialInput", pDevice->GetDefaultResourceView(pInputMaterial->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("VelocityInput", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthInput", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("DepthHierarchy", pDevice->GetDefaultResourceView(m_hDepthHierarchyTmp));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plPostProcessingConstants", m_hPostProcessConstantBuffer);

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_EARLYEXIT");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingEarlyexitBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer, 0).IgnoreResult();

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_CHEAP");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingCheapBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer, sizeof(ezUInt32) * 3).IgnoreResult();

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SSR_TRACING_MODE", "SSR_TRACING_MODE_EXPENSIVE");
      renderViewContext.m_pRenderContext->BindBuffer("tiles", pDevice->GetDefaultResourceView(m_hTilesTracingExpensiveBuffer));
      renderViewContext.m_pRenderContext->DispatchIndirect(m_hTileStatisticsBuffer, sizeof(ezUInt32) * 6).IgnoreResult();
    }

    // SSR Pass
    {
      //      auto pCommandEncoder = ezRenderContext::BeginComputeScope(pPass, renderViewContext, "SSR");
      //
      //      renderViewContext.m_pRenderContext->BindShader(m_hSSRShader);
      //
      //      ezGALUnorderedAccessViewHandle hOutput;
      //      {
      //        ezGALUnorderedAccessViewCreationDescription desc;
      //        desc.m_hTexture = pOutput->m_TextureHandle;
      //        desc.m_uiMipLevelToUse = 0;
      //        hOutput = pDevice->CreateUnorderedAccessView(desc);
      //      }
      //
      //      renderViewContext.m_pRenderContext->BindUAV("tex_uav", hOutput);
      //      renderViewContext.m_pRenderContext->BindTexture2D("tex", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));
      //      renderViewContext.m_pRenderContext->BindTexture2D("tex_normal", pDevice->GetDefaultResourceView(pInputNormal->m_TextureHandle));
      //      renderViewContext.m_pRenderContext->BindTexture2D("tex_velocity", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      //      renderViewContext.m_pRenderContext->BindTexture2D("tex_material", pDevice->GetDefaultResourceView(inputs[m_PinInputMaterial.m_uiInputIndex]->m_TextureHandle));
      //      renderViewContext.m_pRenderContext->BindTexture2D("tex_depth", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      //      renderViewContext.m_pRenderContext->BindConstantBuffer("ezSSRConstants", m_hSSRConstantBuffer);
      //
      //      const ezUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
      //      const ezUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;
      //
      //      const ezUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      //      const ezUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE - 1) / POSTPROCESS_BLOCKSIZE;
      //
      //      UpdateSSRConstantBuffer();
      //
      //      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    pDevice->EndPass(pPass);

    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);

    pDevice->DestroyBuffer(m_hTileStatisticsBuffer);
    EZ_DELETE_ARRAY(ezAlignedAllocatorWrapper::GetAllocator(), m_TileStatistics);
  }
}

void ezSSRAdvancedPass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinSceneColor.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinSceneColor.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  const ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);

  if (const ezGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle); pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    ezLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = ezRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}

void ezSSRAdvancedPass::UpdateSSRConstantBuffer() const
{
  auto* constants = ezRenderContext::GetConstantBufferData<ezSSRConstants>(m_hSSRConstantBuffer);
  constants->RoughnessCutoff = 0.8;
  constants->Frame = ezRenderWorld::GetFrameCounter();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SSRAdvancedPass);
