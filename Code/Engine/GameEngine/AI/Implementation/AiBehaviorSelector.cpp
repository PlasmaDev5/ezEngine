#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/AiBehaviorSelector.h>

ezAiBehaviorSelector::ezAiBehaviorSelector() = default;
ezAiBehaviorSelector::~ezAiBehaviorSelector() = default;

void ezAiBehaviorSelector::AddBehavior(ezUniquePtr<ezAiBehavior>&& pBehavior)
{
  m_Behaviors.PushBack(std::move(pBehavior));
}

ezAiBehaviorScore ezAiBehaviorSelector::SelectBehavior(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  ezAiBehaviorScore res;

  for (auto& pBehavior : m_Behaviors)
  {
    const ezAiBehaviorScore scored = pBehavior->DetermineBehaviorScore(owner, perceptionManager);

    if (scored.m_fScore > res.m_fScore)
    {
      res = scored;
    }
  }

  return res;
}
