#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <GameEngine/AI/AiPerceptionGenerator.h>
#include <GameEngine/AI/AiSensorManager.h>

ezAiPerceptionGenerator::ezAiPerceptionGenerator() = default;
ezAiPerceptionGenerator::~ezAiPerceptionGenerator() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionManager::ezAiPerceptionManager() = default;
ezAiPerceptionManager::~ezAiPerceptionManager() = default;

void ezAiPerceptionManager::AddGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator)
{
  m_Generators.PushBack(std::move(pGenerator));
}

void ezAiPerceptionManager::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  for (auto& pGenerator : m_Generators)
  {
    pGenerator->FlagNeededSensors(ref_SensorManager);
  }
}

void ezAiPerceptionManager::UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager)
{
  for (auto& pGenerator : m_Generators)
  {
    pGenerator->UpdatePerceptions(pOwner, ref_SensorManager);
  }
}

bool ezAiPerceptionManager::HasPerceptionsOfType(ezStringView sPerceptionType) const
{
  for (auto& pGenerator : m_Generators)
  {
    if (pGenerator->GetPerceptionType() == sPerceptionType && pGenerator->HasPerceptions())
    {
      return true;
    }
  }

  return false;
}

void ezAiPerceptionManager::GetPerceptionsOfType(ezStringView sPerceptionType, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  for (auto& pGenerator : m_Generators)
  {
    if (pGenerator->GetPerceptionType() == sPerceptionType)
    {
      pGenerator->GetPerceptions(out_Perceptions);
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenPOI::ezAiPerceptionGenPOI() = default;
ezAiPerceptionGenPOI::~ezAiPerceptionGenPOI() = default;

void ezAiPerceptionGenPOI::UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  const ezAiSensorSpatial* pSensorSee = static_cast<const ezAiSensorSpatial*>(ref_SensorManager.GetSensor("Sensor_See"));
  if (pSensorSee == nullptr)
    return;

  ezWorld* pWorld = pOwner->GetWorld();

  ezHybridArray<ezGameObjectHandle, 32> detectedObjects;
  pSensorSee->RetrieveSensations(pOwner, detectedObjects);

  for (ezGameObjectHandle hObj : detectedObjects)
  {
    ezGameObject* pObj = nullptr;
    if (pWorld->TryGetObject(hObj, pObj))
    {
      auto& g = m_Perceptions.ExpandAndGetRef();
      g.m_vGlobalPosition = pObj->GetGlobalPosition();
    }
  }
}

bool ezAiPerceptionGenPOI::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void ezAiPerceptionGenPOI::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void ezAiPerceptionGenPOI::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  ref_SensorManager.FlagAsNeeded("Sensor_See");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiPerceptionGenWander::ezAiPerceptionGenWander() = default;
ezAiPerceptionGenWander::~ezAiPerceptionGenWander() = default;

void ezAiPerceptionGenWander::UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  ezWorld* pWorld = pOwner->GetWorld();
  const ezVec3 c = pOwner->GetGlobalPosition();
  const ezVec3 dir = 3.0f * pOwner->GetGlobalDirForwards();
  const ezVec3 right = 5.0f * pOwner->GetGlobalDirRight();

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir - right;
  }
}

bool ezAiPerceptionGenWander::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void ezAiPerceptionGenWander::GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void ezAiPerceptionGenWander::FlagNeededSensors(ezAiSensorManager& ref_SensorManager)
{
  ref_SensorManager.FlagAsNeeded("Sensor_See");
}
