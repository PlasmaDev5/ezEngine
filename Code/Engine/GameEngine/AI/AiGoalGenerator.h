#pragma once

#include <GameEngine/AI/AiGoal.h>

class ezGameObject;

class EZ_GAMEENGINE_DLL ezAiGoalGenerator
{
public:
  ezAiGoalGenerator();
  virtual ~ezAiGoalGenerator();

  virtual void Evaluate(ezGameObject* pOwner) = 0;
};


class EZ_GAMEENGINE_DLL ezAiGoalGenPOI : public ezAiGoalGenerator
{
public:
  ezAiGoalGenPOI();
  ~ezAiGoalGenPOI();

  virtual void Evaluate(ezGameObject* pOwner) override;

private:
  ezDynamicArray<ezAiGoalPOI> m_Goals;
};
