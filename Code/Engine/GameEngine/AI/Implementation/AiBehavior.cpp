#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/AiBehavior.h>

ezAiBehavior::ezAiBehavior() = default;
ezAiBehavior::~ezAiBehavior() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorGoToPOI::ezAiBehaviorGoToPOI() = default;
ezAiBehaviorGoToPOI::~ezAiBehaviorGoToPOI() = default;

ezAiBehaviorScore ezAiBehaviorGoToPOI::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionPOI"_ezsv))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionPOI"_ezsv, perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  float fClosestSqr = ezMath::HighValue<float>();

  ezAiBehaviorScore res;
  res.m_pBehavior = this;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    const float fDistSqr = (pPerception->m_vGlobalPosition - vOwnerPos).GetLengthSquared();

    if (fDistSqr < fClosestSqr)
    {
      fClosestSqr = fDistSqr;
      res.m_pPerception = pPerception;
    }
  }

  if (res.m_pPerception != nullptr)
  {
    const float fDist = ezMath::Max(1.0f, ezMath::Sqrt(fClosestSqr));
    res.m_fScore = 1.0f / fDist;
  }

  return res;
}

void ezAiBehaviorGoToPOI::SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  inout_ActionQueue.Cancel(&owner);

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
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
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_fSpeed = 1.0f;
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.AddAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorWander::ezAiBehaviorWander() = default;
ezAiBehaviorWander::~ezAiBehaviorWander() = default;

ezAiBehaviorScore ezAiBehaviorWander::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionWander"_ezsv))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionWander"_ezsv, perceptions);

  if (perceptions.IsEmpty())
    return {};

  const ezUInt32 uiPerceptionIdx = owner.GetWorld()->GetRandomNumberGenerator().UIntInRange(perceptions.GetCount());

  ezAiBehaviorScore res;
  res.m_pBehavior = this;
  res.m_fScore = 0.1f;
  res.m_pPerception = perceptions[uiPerceptionIdx];

  return res;
}

void ezAiBehaviorWander::SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionWander* pPerception = static_cast<const ezAiPerceptionWander*>(pPerception0);

  inout_ActionQueue.Cancel(&owner);

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
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
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_fSpeed = 0.5f;
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("MoveForwards");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.AddAction(pCmd);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezAiBehaviorShoot::ezAiBehaviorShoot() = default;
ezAiBehaviorShoot::~ezAiBehaviorShoot() = default;

ezAiBehaviorScore ezAiBehaviorShoot::DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager)
{
  if (!perceptionManager.HasPerceptionsOfType("ezAiPerceptionPOI"_ezsv))
    return {};

  ezHybridArray<const ezAiPerception*, 32> perceptions;
  perceptionManager.GetPerceptionsOfType("ezAiPerceptionPOI"_ezsv, perceptions);

  const ezVec3 vOwnerPos = owner.GetGlobalPosition();
  const ezVec3 vOwnerDir = owner.GetGlobalDirForwards();
  float fClosest = ezMath::HighValue<float>();

  ezAiBehaviorScore res;
  res.m_pBehavior = this;

  for (const ezAiPerception* pPerception0 : perceptions)
  {
    const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

    ezVec3 vDirTo = pPerception->m_vGlobalPosition - vOwnerPos;
    const float fDist = vDirTo.GetLengthAndNormalize();

    if (fDist < 2.0f || fDist > 5.0f)
      continue;

    if (fDist >= fClosest)
      continue;

    fClosest = fDist;
    res.m_pPerception = pPerception;
  }

  if (res.m_pPerception != nullptr)
  {
    const float fDist = ezMath::Max(1.0f, fClosest);
    res.m_fScore = (0.8f / fDist) + 0.1f;
  }

  return res;
}

void ezAiBehaviorShoot::SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception0, ezAiActionQueue& inout_ActionQueue)
{
  const ezAiPerceptionPOI* pPerception = static_cast<const ezAiPerceptionPOI*>(pPerception0);

  inout_ActionQueue.Cancel(&owner);

  {
    auto pAct = ezAiActionLerpRotationTowards::Create();
    pAct->m_vTargetPosition = pPerception->m_vGlobalPosition;
    pAct->m_TurnAnglesPerSec = ezAngle::Degree(90);
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("Aim");
    pCmd->m_Value = 1;
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto pAct = ezAiActionSpawn::Create();
    pAct->m_sChildObjectName = ezTempHashedString("Spawn");
    inout_ActionQueue.AddAction(pAct);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetAndWait::Create();
    pCmd->m_sEntryName = ezTempHashedString("Shoot");
    pCmd->m_SetValue = 1;
    pCmd->m_WaitValue = 0;
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionWait::Create();
    pCmd->m_Duration = ezTime::Seconds(0.5);
    inout_ActionQueue.AddAction(pCmd);
  }
  {
    auto* pCmd = ezAiActionBlackboardSetEntry::Create();
    pCmd->m_sEntryName = ezTempHashedString("Aim");
    pCmd->m_Value = 0;
    pCmd->m_bNoCancel = true;
    inout_ActionQueue.AddAction(pCmd);
  }
}
