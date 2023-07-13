#pragma once

#include <GameEngine/GameEngineDLL.h>

class ezGameObject;

enum class [[nodiscard]] ezAiCommandResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again.
};

class EZ_GAMEENGINE_DLL ezAiCommand
{
public:
  ezAiCommand();
  virtual ~ezAiCommand();

  virtual void Reset() = 0;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) = 0;
  virtual void Cancel(ezGameObject* pOwner) = 0;
};

class EZ_GAMEENGINE_DLL ezAiCommandWait : public ezAiCommand
{
public:
  ezAiCommandWait();
  ~ezAiCommandWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTime m_Duration;
};

class EZ_GAMEENGINE_DLL ezAiCommandTurn : public ezAiCommand
{
public:
  ezAiCommandTurn();
  ~ezAiCommandTurn();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTurnAxis = ezVec3::UnitZAxis();
  ezAngle m_TurnAngle;
  ezAngle m_TurnAnglesPerSec;
};

class EZ_GAMEENGINE_DLL ezAiCommandSlide : public ezAiCommand
{
public:
  ezAiCommandSlide();
  ~ezAiCommandSlide();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  float m_fSpeed = 0.0f;
  ezVec3 m_vLocalSpaceSlide = ezVec3::ZeroVector();
};
