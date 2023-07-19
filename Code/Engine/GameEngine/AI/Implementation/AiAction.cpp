#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AiAction.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

ezAiAction::ezAiAction() = default;
ezAiAction::~ezAiAction() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionWait);

ezAiActionWait::ezAiActionWait() = default;
ezAiActionWait::~ezAiActionWait() = default;

void ezAiActionWait::Reset()
{
  m_Duration = ezTime::Zero();
}

void ezAiActionWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait: {}", m_Duration);
}

ezAiActionResult ezAiActionWait::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (tDiff >= m_Duration)
    return ezAiActionResult::Finished; // or canceled

  m_Duration -= tDiff;
  return ezAiActionResult::Succeded;
}

void ezAiActionWait::Cancel(ezGameObject* pOwner)
{
  m_Duration = ezTime::Zero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpRotation);

ezAiActionLerpRotation::ezAiActionLerpRotation() = default;
ezAiActionLerpRotation::~ezAiActionLerpRotation() = default;

void ezAiActionLerpRotation::Reset()
{
  m_vTurnAxis = ezVec3::UnitZAxis();
  m_TurnAngle = {};
  m_TurnAnglesPerSec = {};
}

void ezAiActionLerpRotation::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn: {}/{}/{} - {} @ {}/sec", m_vTurnAxis.x, m_vTurnAxis.y, m_vTurnAxis.z, m_TurnAngle, m_TurnAnglesPerSec);
}

ezAiActionResult ezAiActionLerpRotation::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiActionResult::Finished; // or canceled

  if (m_TurnAngle < ezAngle())
  {
    m_TurnAngle = -m_TurnAngle;
    m_vTurnAxis = -m_vTurnAxis;
  }

  const ezAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const ezAngle toTurn = ezMath::Min(m_TurnAngle, turnAmount);

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(m_vTurnAxis, toTurn);

  const ezQuat qCurRot = pOwner->GetGlobalRotation();

  pOwner->SetGlobalRotation(qRot * qCurRot);

  if (turnAmount >= m_TurnAngle)
    return ezAiActionResult::Finished;

  m_TurnAngle -= toTurn;
  return ezAiActionResult::Succeded;
}

void ezAiActionLerpRotation::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpPosition);

ezAiActionLerpPosition::ezAiActionLerpPosition() = default;
ezAiActionLerpPosition::~ezAiActionLerpPosition() = default;

void ezAiActionLerpPosition::Reset()
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide = ezVec3::ZeroVector();
}

void ezAiActionLerpPosition::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Slide: {}/{}/{} @ {}/sec", m_vLocalSpaceSlide.x, m_vLocalSpaceSlide.y, m_vLocalSpaceSlide.z, m_fSpeed);
}

ezAiActionResult ezAiActionLerpPosition::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_vLocalSpaceSlide.IsZero())
    return ezAiActionResult::Finished; // or canceled

  if (m_fSpeed <= 0.0f)
    return ezAiActionResult::Failed;

  ezVec3 vSlideDir = m_vLocalSpaceSlide;
  const float fMaxSlide = vSlideDir.GetLengthAndNormalize();

  const float fSlideAmount = tDiff.AsFloatInSeconds() * m_fSpeed;
  const float fSlideDist = ezMath::Min(fMaxSlide, fSlideAmount);
  const ezVec3 vSlide = fSlideDist * vSlideDir;

  const ezVec3 vCurGlobalPos = pOwner->GetGlobalPosition();

  pOwner->SetGlobalPosition(vCurGlobalPos + pOwner->GetGlobalRotation() * vSlide);

  if (fSlideAmount >= fMaxSlide)
    return ezAiActionResult::Finished;

  m_vLocalSpaceSlide -= vSlide;
  return ezAiActionResult::Succeded;
}

void ezAiActionLerpPosition::Cancel(ezGameObject* pOwner)
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide.SetZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionLerpRotationTowards);

ezAiActionLerpRotationTowards::ezAiActionLerpRotationTowards() = default;
ezAiActionLerpRotationTowards::~ezAiActionLerpRotationTowards() = default;

void ezAiActionLerpRotationTowards::Reset()
{
  m_vTargetPosition = ezVec3::ZeroVector();
  m_hTargetObject.Invalidate();
  m_TurnAnglesPerSec = {};
}

void ezAiActionLerpRotationTowards::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn Towards: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_TurnAnglesPerSec);
}

ezAiActionResult ezAiActionLerpRotationTowards::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!pOwner->GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      return ezAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const ezVec3 vOwnPos = pOwner->GetGlobalPosition();

  ezVec3 vCurDir = pOwner->GetGlobalDirForwards();
  vCurDir.z = 0.0f;

  ezVec3 vTargetDir = m_vTargetPosition - vOwnPos;
  vTargetDir.z = 0.0f;

  m_vTargetPosition.z = vOwnPos.z;

  if (vCurDir.NormalizeIfNotZero(ezVec3::ZeroVector()).Failed() ||
      vTargetDir.NormalizeIfNotZero(ezVec3::ZeroVector()).Failed())
  {
    return ezAiActionResult::Failed;
  }

  if (vCurDir.IsEqual(vTargetDir, 0.001f))
    return ezAiActionResult::Finished;

  const ezAngle turnAngle = vCurDir.GetAngleBetween(vTargetDir);

  const ezVec3 vTurnAxis = vCurDir.CrossRH(vTargetDir).GetNormalized();

  const ezAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const ezAngle toTurn = ezMath::Min(turnAngle, turnAmount);

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(vTurnAxis, toTurn);

  const ezQuat qCurRot = pOwner->GetGlobalRotation();

  pOwner->SetGlobalRotation(qRot * qCurRot);

  if (turnAngle - toTurn <= m_TargetReachedAngle)
    return ezAiActionResult::Finished;

  return ezAiActionResult::Succeded;
}

void ezAiActionLerpRotationTowards::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// EZ_IMPLEMENT_AICMD(ezAiActionFollowPath);
//
// ezAiActionFollowPath::ezAiActionFollowPath() = default;
// ezAiActionFollowPath::~ezAiActionFollowPath() = default;
//
// void ezAiActionFollowPath::Reset()
//{
//  m_fSpeed = 0.0f;
//  m_hPath.Invalidate();
//}
//
// void ezAiActionFollowPath::GetDebugDesc(ezStringBuilder& inout_sText)
//{
//  inout_sText.Format("Follow Path: @{}/sec", m_fSpeed);
//}
//
// ezAiActionResult ezAiActionFollowPath::Execute(ezGameObject* pOwner, ezTime tDiff)
//{
//  if (m_hPath.IsInvalidated())
//    return ezAiActionResult::Finished; // or canceled
//
//  ezGameObject* pPath;
//  if (!pOwner->GetWorld()->TryGetObject(m_hPath, pPath))
//  {
//    return ezAiActionResult::Failed;
//  }
//
//
//  return ezAiActionResult::Succeded;
//}
//
// void ezAiActionFollowPath::Cancel(ezGameObject* pOwner)
//{
//  m_hPath.Invalidate();
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionBlackboardSetEntry);

ezAiActionBlackboardSetEntry::ezAiActionBlackboardSetEntry() = default;
ezAiActionBlackboardSetEntry::~ezAiActionBlackboardSetEntry() = default;

void ezAiActionBlackboardSetEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
}

void ezAiActionBlackboardSetEntry::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Set Blackboard Entry '{}' to '{}'", m_sEntryName.GetHash(), m_Value);
}

ezAiActionResult ezAiActionBlackboardSetEntry::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiActionResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiActionResult::Failed;
  }

  if (pBlackboard->SetEntryValue(m_sEntryName, m_Value).Failed())
    return ezAiActionResult::Failed;

  return ezAiActionResult::Finished;
}

void ezAiActionBlackboardSetEntry::Cancel(ezGameObject* pOwner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionBlackboardWait);

ezAiActionBlackboardWait::ezAiActionBlackboardWait() = default;
ezAiActionBlackboardWait::~ezAiActionBlackboardWait() = default;

void ezAiActionBlackboardWait::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
  m_bEquals = true;
}

void ezAiActionBlackboardWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait for Blackboard Entry '{}' {} '{}'", m_sEntryName.GetHash(), m_bEquals ? "==" : "!=", m_Value);
}

ezAiActionResult ezAiActionBlackboardWait::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiActionResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiActionResult::Failed;
  }

  const ezVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_Value);
  const bool bIsEqual = (val == m_Value);

  if (m_bEquals == bIsEqual)
    return ezAiActionResult::Finished;

  return ezAiActionResult::Succeded;
}

void ezAiActionBlackboardWait::Cancel(ezGameObject* pOwner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiActionCCMoveTo);

ezAiActionCCMoveTo::ezAiActionCCMoveTo() = default;
ezAiActionCCMoveTo::~ezAiActionCCMoveTo() = default;

void ezAiActionCCMoveTo::Reset()
{
  m_vTargetPosition = ezVec3::ZeroVector();
  m_hTargetObject.Invalidate();
  m_fSpeed = 0.0f;
  m_fReachedDistSQR = 1.0f;
}

void ezAiActionCCMoveTo::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("CCMoveTo: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_fSpeed);
}

ezAiActionResult ezAiActionCCMoveTo::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_fSpeed <= 0.0f)
    return ezAiActionResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!pOwner->GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      return ezAiActionResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const ezVec3 vOwnPos = pOwner->GetGlobalPosition();
  ezVec3 vDir = m_vTargetPosition - vOwnPos;
  vDir.z = 0.0f; // TODO: not the best idea


  if (vDir.GetLengthSquared() <= m_fReachedDistSQR)
    return ezAiActionResult::Finished;

  vDir.z = 0.0f;

  ezQuat qRot;
  qRot.SetShortestRotation(ezVec3::UnitXAxis(), vDir.GetNormalized());
  pOwner->SetGlobalRotation(qRot);

  // const ezVec3 vLocalDir = -pOwner->GetGlobalTransform().m_qRotation * vDir;

  ezMsgMoveCharacterController msg;
  msg.m_fMoveForwards = m_fSpeed;
  // msg.m_fMoveForwards = ezMath::Clamp(vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fMoveBackwards = ezMath::Clamp(-vLocalDir.x, 0.0f, 1.0f);
  // msg.m_fStrafeLeft = ezMath::Clamp(-vLocalDir.y, 0.0f, 1.0f);
  // msg.m_fStrafeRight = ezMath::Clamp(vLocalDir.y, 0.0f, 1.0f);
  pOwner->SendMessage(msg);

  return ezAiActionResult::Succeded;
}

void ezAiActionCCMoveTo::Cancel(ezGameObject* pOwner)
{
  m_fSpeed = 0.0f;
}
