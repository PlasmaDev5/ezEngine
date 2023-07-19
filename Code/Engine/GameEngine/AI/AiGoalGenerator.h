#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/AI/AiGoal.h>

class ezGameObject;

class EZ_GAMEENGINE_DLL ezAiGoalGenerator
{
public:
  ezAiGoalGenerator();
  virtual ~ezAiGoalGenerator();

  virtual ezStringView GetGoalType() = 0;
  virtual void UpdateGoals(ezGameObject* pOwner) = 0;
  virtual bool HasGoals() const = 0;
  virtual void GetGoals(ezDynamicArray<const ezAiGoal*>& out_Goals) const = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiGoalGeneratorGroup
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiGoalGeneratorGroup);

public:
  ezAiGoalGeneratorGroup();
  ~ezAiGoalGeneratorGroup();

  void AddGenerator(ezUniquePtr<ezAiGoalGenerator>&& pGenerator);

  void UpdateGoals(ezGameObject* pOwner);

  bool HasGoalsOfType(ezStringView sGoalType) const;
  void GetGoalsOfType(ezStringView sGoalType, ezDynamicArray<const ezAiGoal*>& out_Goals) const;

private:
  ezHybridArray<ezUniquePtr<ezAiGoalGenerator>, 12> m_Generators;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiGoalGenPOI : public ezAiGoalGenerator
{
public:
  ezAiGoalGenPOI();
  ~ezAiGoalGenPOI();

  virtual ezStringView GetGoalType() override { return "ezAiGoalPOI"_ezsv; }
  virtual void UpdateGoals(ezGameObject* pOwner) override;
  virtual bool HasGoals() const override;
  virtual void GetGoals(ezDynamicArray<const ezAiGoal*>& out_Goals) const override;

private:
  ezDynamicArray<ezAiGoalPOI> m_Goals;
};
