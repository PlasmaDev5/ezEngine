#pragma once

#include <GameEngine/AI/AiGoalGenerator.h>
#include <GameEngine/GameEngineDLL.h>

class ezAiBehavior;

struct ezAiScoredGoal
{
  ezAiBehavior* m_pBehavior = nullptr;
  float m_fScore = 0.0f;
  const ezAiGoal* m_pGoal = nullptr;
};

class EZ_GAMEENGINE_DLL ezAiBehavior
{
public:
  ezAiBehavior();
  virtual ~ezAiBehavior();

  virtual ezAiScoredGoal DetermineBestGoal(const ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup) = 0;
  virtual void SetUpActions(const ezGameObject& owner, const ezAiGoal* pGoal, ezAiActionQueue& inout_ActionQueue) = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiBehaviorGoToPOI : public ezAiBehavior
{
public:
  ezAiBehaviorGoToPOI();
  ~ezAiBehaviorGoToPOI();

  virtual ezAiScoredGoal DetermineBestGoal(const ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup) override;
  virtual void SetUpActions(const ezGameObject& owner, const ezAiGoal* pGoal, ezAiActionQueue& inout_ActionQueue) override;
};
