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

  ezAiBehaviorScore SelectBehavior(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager);

private:
  ezHybridArray<ezUniquePtr<ezAiBehavior>, 12> m_Behaviors;
};
