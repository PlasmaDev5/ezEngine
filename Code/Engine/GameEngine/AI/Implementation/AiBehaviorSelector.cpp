#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/AiBehaviorSelector.h>

ezAiBehaviorSelector::ezAiBehaviorSelector() = default;
ezAiBehaviorSelector::~ezAiBehaviorSelector() = default;

void ezAiBehaviorSelector::AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior)
{
  m_Behaviors.PushBack(std::move(pBehavior));
}

ezAiScoredGoal ezAiBehaviorSelector::SelectGoal(ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup)
{
  ezAiScoredGoal res;

  for (auto& pBehavior : m_Behaviors)
  {
    const ezAiScoredGoal scored = pBehavior->DetermineBestGoal(owner, goalGroup);

    if (scored.m_fScore > res.m_fScore)
    {
      res = scored;
    }
  }

  return res;
}
