#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/AiBehavior.h>

ezAiBehavior::ezAiBehavior() = default;
ezAiBehavior::~ezAiBehavior() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToPOI::ezAiBehaviorGoToPOI() = default;
ezAiBehaviorGoToPOI::~ezAiBehaviorGoToPOI() = default;

ezAiScoredGoal ezAiBehaviorGoToPOI::DetermineBestGoal(ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup)
{
  if (!goalGroup.HasGoalsOfType("ezAiGoalPOI"_ezsv))
    return {};

  ezHybridArray<const ezAiGoal*, 32> goals;
  goalGroup.GetGoalsOfType("ezAiGoalPOI"_ezsv, goals);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  float fClosestSqr = ezMath::HighValue<float>();

  ezAiScoredGoal res;
  res.m_pBehavior = this;

  for (const ezAiGoal* pGoal0 : goals)
  {
    const ezAiGoalPOI* pGoal = static_cast<const ezAiGoalPOI*>(pGoal0);

    const float fDistSqr = (pGoal->m_vGlobalPosition - vOwnerPos).GetLengthSquared();

    if (fDistSqr < fClosestSqr)
    {
      fClosestSqr = fDistSqr;
      res.m_pGoal = pGoal;
    }
  }

  if (res.m_pGoal != nullptr)
  {
    const float fDist = ezMath::Max(1.0f, ezMath::Sqrt(fClosestSqr));
    res.m_fScore = 1.0f / fDist;
  }

  return res;
}

void ezAiBehaviorGoToPOI::SetUpActions(const ezGameObject& owner, const ezAiGoal* pGoal0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiGoalPOI* pGoal = static_cast<const ezAiGoalPOI*>(pGoal0);

  inout_ActionQueue.ClearQueue();

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pGoal->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = ezAngle::Degree(90);
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 1;
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto pAct = ezAiActionCCMoveTo::Create();
    pAct->m_vTargetPosition = pGoal->m_vGlobalPosition;
    pAct->m_fSpeed = 1.0f;
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 0;
    inout_ActionQueue.AddAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorWander::ezAiBehaviorWander() = default;
ezAiBehaviorWander::~ezAiBehaviorWander() = default;

ezAiScoredGoal ezAiBehaviorWander::DetermineBestGoal(ezGameObject& owner, const ezAiGoalGeneratorGroup& goalGroup)
{
  if (!goalGroup.HasGoalsOfType("ezAiGoalWander"_ezsv))
    return {};

  ezHybridArray<const ezAiGoal*, 32> goals;
  goalGroup.GetGoalsOfType("ezAiGoalWander"_ezsv, goals);

  if (goals.IsEmpty())
    return {};

  const ezUInt32 uiGoalIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(goals.GetCount());

  ezAiScoredGoal res;
  res.m_pBehavior = this;
  res.m_fScore = 0.1f;
  res.m_pGoal = goals[uiGoalIdx];

  return res;
}

void ezAiBehaviorWander::SetUpActions(const ezGameObject& owner, const ezAiGoal* pGoal0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiGoalWander* pGoal = static_cast<const ezAiGoalWander*>(pGoal0);

  inout_ActionQueue.ClearQueue();

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pGoal->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = ezAngle::Degree(90);
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 0.5f;
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto pAct = ezAiActionCCMoveTo::Create();
    pAct->m_vTargetPosition = pGoal->m_vGlobalPosition;
    pAct->m_fSpeed = 0.5f;
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 0;
    inout_ActionQueue.AddAction(pCmd);
  }
}
