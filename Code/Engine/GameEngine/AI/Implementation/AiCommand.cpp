#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AiCommand.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

ezAiCmd::ezAiCmd() = default;
ezAiCmd::~ezAiCmd() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdWait);

ezAiCmdWait::ezAiCmdWait() = default;
ezAiCmdWait::~ezAiCmdWait() = default;

void ezAiCmdWait::Reset()
{
  m_Duration = ezTime::Zero();
}

void ezAiCmdWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait: {}", m_Duration);
}

ezAiCmdResult ezAiCmdWait::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (tDiff >= m_Duration)
    return ezAiCmdResult::Finished; // or canceled

  m_Duration -= tDiff;
  return ezAiCmdResult::Succeded;
}

void ezAiCmdWait::Cancel(ezGameObject* pOwner)
{
  m_Duration = ezTime::Zero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdLerpRotation);

ezAiCmdLerpRotation::ezAiCmdLerpRotation() = default;
ezAiCmdLerpRotation::~ezAiCmdLerpRotation() = default;

void ezAiCmdLerpRotation::Reset()
{
  m_vTurnAxis = ezVec3::UnitZAxis();
  m_TurnAngle = {};
  m_TurnAnglesPerSec = {};
}

void ezAiCmdLerpRotation::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn: {}/{}/{} - {} @ {}/sec", m_vTurnAxis.x, m_vTurnAxis.y, m_vTurnAxis.z, m_TurnAngle, m_TurnAnglesPerSec);
}

ezAiCmdResult ezAiCmdLerpRotation::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiCmdResult::Finished; // or canceled

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
    return ezAiCmdResult::Finished;

  m_TurnAngle -= toTurn;
  return ezAiCmdResult::Succeded;
}

void ezAiCmdLerpRotation::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdLerpPosition);

ezAiCmdLerpPosition::ezAiCmdLerpPosition() = default;
ezAiCmdLerpPosition::~ezAiCmdLerpPosition() = default;

void ezAiCmdLerpPosition::Reset()
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide = ezVec3::ZeroVector();
}

void ezAiCmdLerpPosition::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Slide: {}/{}/{} @ {}/sec", m_vLocalSpaceSlide.x, m_vLocalSpaceSlide.y, m_vLocalSpaceSlide.z, m_fSpeed);
}

ezAiCmdResult ezAiCmdLerpPosition::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_vLocalSpaceSlide.IsZero())
    return ezAiCmdResult::Finished; // or canceled

  if (m_fSpeed <= 0.0f)
    return ezAiCmdResult::Failed;

  ezVec3 vSlideDir = m_vLocalSpaceSlide;
  const float fMaxSlide = vSlideDir.GetLengthAndNormalize();

  const float fSlideAmount = tDiff.AsFloatInSeconds() * m_fSpeed;
  const float fSlideDist = ezMath::Min(fMaxSlide, fSlideAmount);
  const ezVec3 vSlide = fSlideDist * vSlideDir;

  const ezVec3 vCurGlobalPos = pOwner->GetGlobalPosition();

  pOwner->SetGlobalPosition(vCurGlobalPos + pOwner->GetGlobalRotation() * vSlide);

  if (fSlideAmount >= fMaxSlide)
    return ezAiCmdResult::Finished;

  m_vLocalSpaceSlide -= vSlide;
  return ezAiCmdResult::Succeded;
}

void ezAiCmdLerpPosition::Cancel(ezGameObject* pOwner)
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide.SetZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdLerpRotationTowards);

ezAiCmdLerpRotationTowards::ezAiCmdLerpRotationTowards() = default;
ezAiCmdLerpRotationTowards::~ezAiCmdLerpRotationTowards() = default;

void ezAiCmdLerpRotationTowards::Reset()
{
  m_vTargetPosition = ezVec3::ZeroVector();
  m_hTargetObject.Invalidate();
  m_TurnAnglesPerSec = {};
}

void ezAiCmdLerpRotationTowards::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn Towards: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_TurnAnglesPerSec);
}

ezAiCmdResult ezAiCmdLerpRotationTowards::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiCmdResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!pOwner->GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      return ezAiCmdResult::Failed;
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
    return ezAiCmdResult::Failed;
  }

  const ezAngle turnAngle = vCurDir.GetAngleBetween(vTargetDir);

  const ezVec3 vTurnAxis = vCurDir.CrossRH(vTargetDir).GetNormalized();

  const ezAngle turnAmount = tDiff.AsFloatInSeconds() * m_TurnAnglesPerSec;
  const ezAngle toTurn = ezMath::Min(turnAngle, turnAmount);

  ezQuat qRot;
  qRot.SetFromAxisAndAngle(vTurnAxis, toTurn);

  const ezQuat qCurRot = pOwner->GetGlobalRotation();

  pOwner->SetGlobalRotation(qRot * qCurRot);

  if (turnAngle - toTurn <= m_TargetReachedAngle)
    return ezAiCmdResult::Finished;

  return ezAiCmdResult::Succeded;
}

void ezAiCmdLerpRotationTowards::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// EZ_IMPLEMENT_AICMD(ezAiCmdFollowPath);
//
// ezAiCmdFollowPath::ezAiCmdFollowPath() = default;
// ezAiCmdFollowPath::~ezAiCmdFollowPath() = default;
//
// void ezAiCmdFollowPath::Reset()
//{
//  m_fSpeed = 0.0f;
//  m_hPath.Invalidate();
//}
//
// void ezAiCmdFollowPath::GetDebugDesc(ezStringBuilder& inout_sText)
//{
//  inout_sText.Format("Follow Path: @{}/sec", m_fSpeed);
//}
//
// ezAiCmdResult ezAiCmdFollowPath::Execute(ezGameObject* pOwner, ezTime tDiff)
//{
//  if (m_hPath.IsInvalidated())
//    return ezAiCmdResult::Finished; // or canceled
//
//  ezGameObject* pPath;
//  if (!pOwner->GetWorld()->TryGetObject(m_hPath, pPath))
//  {
//    return ezAiCmdResult::Failed;
//  }
//
//
//  return ezAiCmdResult::Succeded;
//}
//
// void ezAiCmdFollowPath::Cancel(ezGameObject* pOwner)
//{
//  m_hPath.Invalidate();
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdBlackboardSetEntry);

ezAiCmdBlackboardSetEntry::ezAiCmdBlackboardSetEntry() = default;
ezAiCmdBlackboardSetEntry::~ezAiCmdBlackboardSetEntry() = default;

void ezAiCmdBlackboardSetEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
}

void ezAiCmdBlackboardSetEntry::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Set Blackboard Entry '{}' to '{}'", m_sEntryName.GetHash(), m_Value);
}

ezAiCmdResult ezAiCmdBlackboardSetEntry::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiCmdResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiCmdResult::Failed;
  }

  if (pBlackboard->SetEntryValue(m_sEntryName, m_Value).Failed())
    return ezAiCmdResult::Failed;

  return ezAiCmdResult::Finished;
}

void ezAiCmdBlackboardSetEntry::Cancel(ezGameObject* pOwner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdBlackboardWait);

ezAiCmdBlackboardWait::ezAiCmdBlackboardWait() = default;
ezAiCmdBlackboardWait::~ezAiCmdBlackboardWait() = default;

void ezAiCmdBlackboardWait::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
  m_bEquals = true;
}

void ezAiCmdBlackboardWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait for Blackboard Entry '{}' {} '{}'", m_sEntryName.GetHash(), m_bEquals ? "==" : "!=", m_Value);
}

ezAiCmdResult ezAiCmdBlackboardWait::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiCmdResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiCmdResult::Failed;
  }

  const ezVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_Value);
  const bool bIsEqual = (val == m_Value);

  if (m_bEquals == bIsEqual)
    return ezAiCmdResult::Finished;

  return ezAiCmdResult::Succeded;
}

void ezAiCmdBlackboardWait::Cancel(ezGameObject* pOwner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCmdCCMoveTo);

ezAiCmdCCMoveTo::ezAiCmdCCMoveTo() = default;
ezAiCmdCCMoveTo::~ezAiCmdCCMoveTo() = default;

void ezAiCmdCCMoveTo::Reset()
{
  m_vTargetPosition = ezVec3::ZeroVector();
  m_hTargetObject.Invalidate();
  m_fSpeed = 0.0f;
  m_fReachedDistSQR = 1.0f;
}

void ezAiCmdCCMoveTo::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("CCMoveTo: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_fSpeed);
}

ezAiCmdResult ezAiCmdCCMoveTo::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_fSpeed <= 0.0f)
    return ezAiCmdResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!pOwner->GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      return ezAiCmdResult::Failed;
    }

    m_vTargetPosition = pTarget->GetGlobalPosition();
  }

  const ezVec3 vOwnPos = pOwner->GetGlobalPosition();
  ezVec3 vDir = m_vTargetPosition - vOwnPos;


  if (vDir.GetLengthSquared() <= m_fReachedDistSQR)
    return ezAiCmdResult::Finished;

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

  return ezAiCmdResult::Succeded;
}

void ezAiCmdCCMoveTo::Cancel(ezGameObject* pOwner)
{
  m_fSpeed = 0.0f;
}
