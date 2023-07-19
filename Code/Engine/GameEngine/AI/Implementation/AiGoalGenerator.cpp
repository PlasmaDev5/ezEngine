#include <GameEngine/GameEnginePCH.h>

#include "Core/Interfaces/PhysicsWorldModule.h"
#include <GameEngine/AI/AiGoalGenerator.h>

ezAiGoalGenerator::ezAiGoalGenerator() = default;
ezAiGoalGenerator::~ezAiGoalGenerator() = default;

ezAiGoalGenPOI::ezAiGoalGenPOI() = default;
ezAiGoalGenPOI::~ezAiGoalGenPOI() = default;

void ezAiGoalGenPOI::Evaluate(ezGameObject* pOwner)
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

        ezLog::Info("POI Goal At: {}/{}/{}", g.m_vGlobalPosition.x, g.m_vGlobalPosition.y, g.m_vGlobalPosition.z);
      }
    }
  }
}
