#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AiCommand.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>

ezAiCommand::ezAiCommand() = default;
ezAiCommand::~ezAiCommand() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandWait);

ezAiCommandWait::ezAiCommandWait() = default;
ezAiCommandWait::~ezAiCommandWait() = default;

void ezAiCommandWait::Reset()
{
  m_Duration = ezTime::Zero();
}

void ezAiCommandWait::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait: {}", m_Duration);
}

ezAiCommandResult ezAiCommandWait::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (tDiff >= m_Duration)
    return ezAiCommandResult::Finished; // or canceled

  m_Duration -= tDiff;
  return ezAiCommandResult::Succeded;
}

void ezAiCommandWait::Cancel(ezGameObject* pOwner)
{
  m_Duration = ezTime::Zero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandTurn);

ezAiCommandTurn::ezAiCommandTurn() = default;
ezAiCommandTurn::~ezAiCommandTurn() = default;

void ezAiCommandTurn::Reset()
{
  m_vTurnAxis = ezVec3::UnitZAxis();
  m_TurnAngle = {};
  m_TurnAnglesPerSec = {};
}

void ezAiCommandTurn::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn: {}/{}/{} - {} @ {}/sec", m_vTurnAxis.x, m_vTurnAxis.y, m_vTurnAxis.z, m_TurnAngle, m_TurnAnglesPerSec);
}

ezAiCommandResult ezAiCommandTurn::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiCommandResult::Finished; // or canceled

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
    return ezAiCommandResult::Finished;

  m_TurnAngle -= toTurn;
  return ezAiCommandResult::Succeded;
}

void ezAiCommandTurn::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandSlide);

ezAiCommandSlide::ezAiCommandSlide() = default;
ezAiCommandSlide::~ezAiCommandSlide() = default;

void ezAiCommandSlide::Reset()
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide = ezVec3::ZeroVector();
}

void ezAiCommandSlide::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Slide: {}/{}/{} @ {}/sec", m_vLocalSpaceSlide.x, m_vLocalSpaceSlide.y, m_vLocalSpaceSlide.z, m_fSpeed);
}

ezAiCommandResult ezAiCommandSlide::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_vLocalSpaceSlide.IsZero())
    return ezAiCommandResult::Finished; // or canceled

  if (m_fSpeed <= 0.0f)
    return ezAiCommandResult::Failed;

  ezVec3 vSlideDir = m_vLocalSpaceSlide;
  const float fMaxSlide = vSlideDir.GetLengthAndNormalize();

  const float fSlideAmount = tDiff.AsFloatInSeconds() * m_fSpeed;
  const float fSlideDist = ezMath::Min(fMaxSlide, fSlideAmount);
  const ezVec3 vSlide = fSlideDist * vSlideDir;

  const ezVec3 vCurGlobalPos = pOwner->GetGlobalPosition();

  pOwner->SetGlobalPosition(vCurGlobalPos + pOwner->GetGlobalRotation() * vSlide);

  if (fSlideAmount >= fMaxSlide)
    return ezAiCommandResult::Finished;

  m_vLocalSpaceSlide -= vSlide;
  return ezAiCommandResult::Succeded;
}

void ezAiCommandSlide::Cancel(ezGameObject* pOwner)
{
  m_fSpeed = 0.0f;
  m_vLocalSpaceSlide.SetZero();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandTurnTowards);

ezAiCommandTurnTowards::ezAiCommandTurnTowards() = default;
ezAiCommandTurnTowards::~ezAiCommandTurnTowards() = default;

void ezAiCommandTurnTowards::Reset()
{
  m_vTargetPosition = ezVec3::ZeroVector();
  m_hTargetObject.Invalidate();
  m_TurnAnglesPerSec = {};
}

void ezAiCommandTurnTowards::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Turn Towards: {}/{}/{} - @{}/sec", m_vTargetPosition.x, m_vTargetPosition.y, m_vTargetPosition.z, m_TurnAnglesPerSec);
}

ezAiCommandResult ezAiCommandTurnTowards::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_TurnAnglesPerSec <= ezAngle())
    return ezAiCommandResult::Finished; // or canceled

  if (!m_hTargetObject.IsInvalidated())
  {
    ezGameObject* pTarget;
    if (!pOwner->GetWorld()->TryGetObject(m_hTargetObject, pTarget))
    {
      return ezAiCommandResult::Failed;
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
    return ezAiCommandResult::Failed;
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
    return ezAiCommandResult::Finished;

  return ezAiCommandResult::Succeded;
}

void ezAiCommandTurnTowards::Cancel(ezGameObject* pOwner)
{
  m_TurnAnglesPerSec = {};
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandFollowPath);

ezAiCommandFollowPath::ezAiCommandFollowPath() = default;
ezAiCommandFollowPath::~ezAiCommandFollowPath() = default;

void ezAiCommandFollowPath::Reset()
{
  m_fSpeed = 0.0f;
  m_hPath.Invalidate();
}

void ezAiCommandFollowPath::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Follow Path: @{}/sec", m_fSpeed);
}

ezAiCommandResult ezAiCommandFollowPath::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_hPath.IsInvalidated())
    return ezAiCommandResult::Finished; // or canceled

  ezGameObject* pPath;
  if (!pOwner->GetWorld()->TryGetObject(m_hPath, pPath))
  {
    return ezAiCommandResult::Failed;
  }


  return ezAiCommandResult::Succeded;
}

void ezAiCommandFollowPath::Cancel(ezGameObject* pOwner)
{
  m_hPath.Invalidate();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandSetBlackboardEntry);

ezAiCommandSetBlackboardEntry::ezAiCommandSetBlackboardEntry() = default;
ezAiCommandSetBlackboardEntry::~ezAiCommandSetBlackboardEntry() = default;

void ezAiCommandSetBlackboardEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
}

void ezAiCommandSetBlackboardEntry::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Set Blackboard Entry '{}' to '{}'", m_sEntryName.GetHash(), m_Value);
}

ezAiCommandResult ezAiCommandSetBlackboardEntry::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiCommandResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiCommandResult::Failed;
  }

  if (pBlackboard->SetEntryValue(m_sEntryName, m_Value).Failed())
    return ezAiCommandResult::Failed;

  return ezAiCommandResult::Finished;
}

void ezAiCommandSetBlackboardEntry::Cancel(ezGameObject* pOwner)
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_AICMD(ezAiCommandWaitForBlackboardEntry);

ezAiCommandWaitForBlackboardEntry::ezAiCommandWaitForBlackboardEntry() = default;
ezAiCommandWaitForBlackboardEntry::~ezAiCommandWaitForBlackboardEntry() = default;

void ezAiCommandWaitForBlackboardEntry::Reset()
{
  m_sEntryName.Clear();
  m_Value = {};
  m_bEquals = true;
}

void ezAiCommandWaitForBlackboardEntry::GetDebugDesc(ezStringBuilder& inout_sText)
{
  inout_sText.Format("Wait for Blackboard Entry '{}' {} '{}'", m_sEntryName.GetHash(), m_bEquals ? "==" : "!=", m_Value);
}

ezAiCommandResult ezAiCommandWaitForBlackboardEntry::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  if (m_sEntryName.IsEmpty())
    return ezAiCommandResult::Finished; // or canceled

  auto pBlackboard = ezBlackboardComponent::FindBlackboard(pOwner);

  if (pBlackboard == nullptr)
  {
    return ezAiCommandResult::Failed;
  }

  const ezVariant val = pBlackboard->GetEntryValue(m_sEntryName, m_Value);
  const bool bIsEqual = (val == m_Value);

  if (m_bEquals == bIsEqual)
    return ezAiCommandResult::Finished;

  return ezAiCommandResult::Succeded;
}

void ezAiCommandWaitForBlackboardEntry::Cancel(ezGameObject* pOwner)
{
  Reset();
}
