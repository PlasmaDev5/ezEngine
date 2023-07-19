#include <GameEngine/GameEnginePCH.h>

#include "Core/Interfaces/PhysicsWorldModule.h"
#include <GameEngine/AI/AiGoalGenerator.h>

ezAiGoalGenerator::ezAiGoalGenerator() = default;
ezAiGoalGenerator::~ezAiGoalGenerator() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiGoalGeneratorGroup::ezAiGoalGeneratorGroup() = default;
ezAiGoalGeneratorGroup::~ezAiGoalGeneratorGroup() = default;

void ezAiGoalGeneratorGroup::AddGenerator(ezUniquePtr<ezAiGoalGenerator>&& pGenerator)
{
  m_Generators.PushBack(std::move(pGenerator));
}

void ezAiGoalGeneratorGroup::UpdateGoals(ezGameObject* pOwner)
{
  for (auto& pGenerator : m_Generators)
  {
    pGenerator->UpdateGoals(pOwner);
  }
}

bool ezAiGoalGeneratorGroup::HasGoalsOfType(ezStringView sGoalType) const
{
  for (auto& pGenerator : m_Generators)
  {
    if (pGenerator->GetGoalType() == sGoalType && pGenerator->HasGoals())
    {
      return true;
    }
  }

  return false;
}

void ezAiGoalGeneratorGroup::GetGoalsOfType(ezStringView sGoalType, ezDynamicArray<const ezAiGoal*>& out_Goals) const
{
  for (auto& pGenerator : m_Generators)
  {
    if (pGenerator->GetGoalType() == sGoalType)
    {
      pGenerator->GetGoals(out_Goals);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiGoalGenPOI::ezAiGoalGenPOI() = default;
ezAiGoalGenPOI::~ezAiGoalGenPOI() = default;

void ezAiGoalGenPOI::UpdateGoals(ezGameObject* pOwner)
{
  m_Goals.Clear();

  ezGameObject* pSensors = pOwner->FindChildByName("Sensor_POI");

  if (pSensors == nullptr)
    return;

  ezDynamicArray<ezSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;

  ezWorld* pWorld = pOwner->GetWorld();

  ezPhysicsWorldModuleInterface* pPhysicsWorldModule = pWorld->GetModule<ezPhysicsWorldModuleInterface>();

  ezHybridArray<ezGameObject*, 32> objectsInSensorVolume;
  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;

  for (auto pSensor : sensors)
  {
    pSensor->RunSensorCheck(pPhysicsWorldModule, objectsInSensorVolume, detectedObjects, false);

    for (ezGameObjectHandle hObj : pSensor->GetLastDetectedObjects())
    {
      ezGameObject* pObj = nullptr;
      if (pWorld->TryGetObject(hObj, pObj))
      {
        auto& g = m_Goals.ExpandAndGetRef();
        g.m_vGlobalPosition = pObj->GetGlobalPosition();

        //ezLog::Info("POI Goal At: {}/{}/{}", g.m_vGlobalPosition.x, g.m_vGlobalPosition.y, g.m_vGlobalPosition.z);
      }
    }
  }
}

bool ezAiGoalGenPOI::HasGoals() const
{
  return !m_Goals.IsEmpty();
}

void ezAiGoalGenPOI::GetGoals(ezDynamicArray<const ezAiGoal*>& out_Goals) const
{
  out_Goals.Reserve(out_Goals.GetCount() + m_Goals.GetCount());

  for (const auto& goal : m_Goals)
  {
    out_Goals.PushBack(&goal);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiGoalGenWander::ezAiGoalGenWander() = default;
ezAiGoalGenWander::~ezAiGoalGenWander() = default;

void ezAiGoalGenWander::UpdateGoals(ezGameObject* pOwner)
{
  m_Goals.Clear();

  ezWorld* pWorld = pOwner->GetWorld();
  const ezVec3 c = pOwner->GetGlobalPosition();
  const ezVec3 dir = 3.0f * pOwner->GetGlobalDirForwards();
  const ezVec3 right = 5.0f * pOwner->GetGlobalDirRight();

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c + right;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c - right;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir + right;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir - right;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir + right;
  }

  {
    auto& g = m_Goals.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir - right;
  }
}

bool ezAiGoalGenWander::HasGoals() const
{
  return !m_Goals.IsEmpty();
}

void ezAiGoalGenWander::GetGoals(ezDynamicArray<const ezAiGoal*>& out_Goals) const
{
  out_Goals.Reserve(out_Goals.GetCount() + m_Goals.GetCount());

  for (const auto& goal : m_Goals)
  {
    out_Goals.PushBack(&goal);
  }
}
