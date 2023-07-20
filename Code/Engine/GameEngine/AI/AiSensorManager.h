#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/AI/AiPerception.h>
#include <GameEngine/AI/AiSensor.h>

class EZ_GAMEENGINE_DLL ezAiSensorManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiSensorManager);

public:
  ezAiSensorManager();
  ~ezAiSensorManager();

  void AddSensor(ezStringView sName, ezUniquePtr<ezAiSensor>&& pSensor);

  void FlagAsNeeded(ezStringView sName);

  void UpdateNeededSensors(ezGameObject* pOwner);

  const ezAiSensor* GetSensor(ezStringView sName) const;

private:
  struct Sensor
  {
    ezString m_sName;
    ezUniquePtr<ezAiSensor> m_pSensor;
    bool m_bActive = true;
    ezUInt32 m_uiNeededInUpdate = 0;
  };

  ezUInt32 m_uiUpdateCount = 0;
  ezHybridArray<Sensor, 2> m_Sensors;
};
