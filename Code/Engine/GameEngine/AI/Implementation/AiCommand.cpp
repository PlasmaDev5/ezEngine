#include <GameEngine/GameEnginePCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AiCommand.h>

ezAiCommand::ezAiCommand() = default;
ezAiCommand::~ezAiCommand() = default;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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
