#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezAiSensor
{
public:
  ezAiSensor();
  virtual ~ezAiSensor();

  virtual void UpdateSensor(ezGameObject* pOwner) = 0;
};

class EZ_GAMEENGINE_DLL ezAiSensorSpatial : public ezAiSensor
{
public:
  ezAiSensorSpatial(ezTempHashedString sObjectName);
  ~ezAiSensorSpatial();

  virtual void UpdateSensor(ezGameObject* pOwner) override;
  void RetrieveSensations(ezGameObject* pOwner, ezDynamicArray<ezGameObjectHandle>& out_Sensations) const;

  ezTempHashedString m_sObjectName;

private:
  ezGameObjectHandle m_hSensorObject;
};
