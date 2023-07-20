#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/AI/AiPerception.h>

class ezGameObject;
class ezAiSensorManager;

class EZ_GAMEENGINE_DLL ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenerator();
  virtual ~ezAiPerceptionGenerator();

  virtual ezStringView GetPerceptionType() = 0;
  virtual void UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager) = 0;
  virtual bool HasPerceptions() const = 0;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const = 0;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiPerceptionManager
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiPerceptionManager);

public:
  ezAiPerceptionManager();
  ~ezAiPerceptionManager();

  void AddGenerator(ezUniquePtr<ezAiPerceptionGenerator>&& pGenerator);

  void FlagNeededSensors(ezAiSensorManager& ref_SensorManager);

  void UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager);

  bool HasPerceptionsOfType(ezStringView sPerceptionType) const;
  void GetPerceptionsOfType(ezStringView sPerceptionType, ezDynamicArray<const ezAiPerception*>& out_Perceptions) const;

private:
  ezHybridArray<ezUniquePtr<ezAiPerceptionGenerator>, 12> m_Generators;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiPerceptionGenPOI : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenPOI();
  ~ezAiPerceptionGenPOI();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionPOI"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;

private:
  ezDynamicArray<ezAiPerceptionPOI> m_Perceptions;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiPerceptionGenWander : public ezAiPerceptionGenerator
{
public:
  ezAiPerceptionGenWander();
  ~ezAiPerceptionGenWander();

  virtual ezStringView GetPerceptionType() override { return "ezAiPerceptionWander"_ezsv; }
  virtual void UpdatePerceptions(ezGameObject* pOwner, const ezAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(ezDynamicArray<const ezAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(ezAiSensorManager& ref_SensorManager) override;

private:
  ezDynamicArray<ezAiPerceptionWander> m_Perceptions;
};
