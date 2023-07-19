#include <GameEngine/GameEnginePCH.h>

#include "../SensorComponent.h"
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AiComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezAiComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezAiComponent::ezAiComponent() = default;
ezAiComponent::~ezAiComponent() = default;


void ezAiComponent::DoSensorCheck()
{
  ezGameObject* pSensors = GetOwner()->FindChildByName("Sensor");

  if (pSensors == nullptr)
    return;

  ezDynamicArray<ezSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;

  ezPhysicsWorldModuleInterface* pPhysicsWorldModule = GetWorld()->GetModule<ezPhysicsWorldModuleInterface>();

  ezHybridArray<ezGameObject*, 32> objectsInSensorVolume;
  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;

  for (auto pSensor : sensors)
  {
    if (pSensor->RunSensorCheck(pPhysicsWorldModule, objectsInSensorVolume, detectedObjects, false))
    {
      ezLog::Info("In sensor: {}", pSensor->GetLastDetectedObjects().GetCount());
    }
  }
}

void ezAiComponent::FillCmdQueue()
{
  if (!m_CommandQueue.IsEmpty())
    return;

  ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

  {
    auto* pCmd = ezAiCmdWait::Create();
    pCmd->m_Duration = ezTime::Seconds(rng.DoubleMinMax(0.5, 1.0));
    m_CommandQueue.AddCommand(pCmd);
  }

  {
    ezGameObject* pPlayer;
    if (GetWorld()->TryGetObjectWithGlobalKey("Player", pPlayer))
    {
      {
        auto* pCmd = ezAiCmdLerpRotationTowards::Create();
        pCmd->m_hTargetObject = pPlayer->GetHandle();
        pCmd->m_TurnAnglesPerSec = ezAngle::Degree(90);
        m_CommandQueue.AddCommand(pCmd);
      }
      {
        auto* pCmd = ezAiCmdBlackboardSetEntry::Create();
        pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
        pCmd->m_Value = 1;
        m_CommandQueue.AddCommand(pCmd);
      }
      {
        auto* pCmd = ezAiCmdCCMoveTo::Create();
        pCmd->m_hTargetObject = pPlayer->GetHandle();
        pCmd->m_fSpeed = 1.0f;
        m_CommandQueue.AddCommand(pCmd);
      }
      {
        auto* pCmd = ezAiCmdBlackboardSetEntry::Create();
        pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
        pCmd->m_Value = 0;
        m_CommandQueue.AddCommand(pCmd);
      }
    }
  }

  {
    auto* pCmd = ezAiCmdBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("Wave");
    pCmd->m_Value = 1;
    m_CommandQueue.AddCommand(pCmd);
  }

  {
    auto* pCmd = ezAiCmdBlackboardWait::Create();
    pCmd->m_sEntryName = ezTempHashedString("Wave");
    pCmd->m_Value = 0;
    m_CommandQueue.AddCommand(pCmd);
  }
}

void ezAiComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;
}

void ezAiComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bDebugInfo;
}

void ezAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void ezAiComponent::OnDeactivated()
{
  m_CommandQueue.ClearQueue();

  SUPER::OnDeactivated();
}

void ezAiComponent::Update()
{
  if (GetWorld()->GetClock().GetAccumulatedTime() > m_LastAiUpdate + ezTime::Seconds(1))
  {
    m_LastAiUpdate = GetWorld()->GetClock().GetAccumulatedTime();

    DoSensorCheck();
    FillCmdQueue();
  }

  m_CommandQueue.Execute(GetOwner(), GetWorld()->GetClock().GetTimeDiff());

  if (m_bDebugInfo)
  {
    m_CommandQueue.PrintDebugInfo(GetOwner());
  }
}
