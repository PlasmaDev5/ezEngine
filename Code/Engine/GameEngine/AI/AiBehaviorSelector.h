#pragma once

#include <GameEngine/AI/AiBehavior.h>

class ezGameObject;

class EZ_GAMEENGINE_DLL ezAiBehaviorSelector
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiBehaviorSelector);

public:
  ezAiBehaviorSelector();
  ~ezAiBehaviorSelector();

  void AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior);

  ezAiScoredGoal SelectGoal(ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup);

private:
  ezHybridArray<ezUniquePtr<ezAiBehavior>, 12> m_Behaviors;
};
