#pragma once

#include <GameEngine/GameEngineDLL.h>

class ezGameObject;

enum class [[nodiscard]] ezAiCommandResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again.
};

template <typename TYPE>
class ezCommandAlloc
{
public:
  TYPE* Acquire()
  {
    TYPE* pType = nullptr;

    if (m_FreeList.IsEmpty())
    {
      pType = &m_Allocated.ExpandAndGetRef();
    }
    else
    {
      pType = m_FreeList.PeekBack();
      m_FreeList.PopBack();
    }

    pType->Reset();
    return pType;
  }

  void Release(TYPE* pType)
  {
    m_FreeList.PushBack(pType);
  }

private:
  ezHybridArray<TYPE*, 16> m_FreeList;
  ezDeque<TYPE> m_Allocated;
};

#define EZ_DECLARE_AICMD(OwnType)           \
public:                                     \
  static OwnType* Create()                  \
  {                                         \
    OwnType* pType = s_Allocator.Acquire(); \
    pType->m_bFromAllocator = true;         \
    return pType;                           \
  }                                         \
                                            \
private:                                    \
  virtual void Destroy() override           \
  {                                         \
    Reset();                                \
    if (m_bFromAllocator)                   \
      s_Allocator.Release(this);            \
  }                                         \
  bool m_bFromAllocator = false;            \
  static ezCommandAlloc<OwnType> s_Allocator;

#define EZ_IMPLEMENT_AICMD(OwnType) \
  ezCommandAlloc<OwnType> OwnType::s_Allocator;

class EZ_GAMEENGINE_DLL ezAiCommand
{
public:
  ezAiCommand();
  virtual ~ezAiCommand();

  virtual void Reset() = 0;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) = 0;
  virtual void Cancel(ezGameObject* pOwner) = 0;

private:
  friend class ezAiCommandQueue;
  virtual void Destroy() = 0;
};

class EZ_GAMEENGINE_DLL ezAiCommandWait : public ezAiCommand
{
  EZ_DECLARE_AICMD(ezAiCommandWait);

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
  EZ_DECLARE_AICMD(ezAiCommandTurn);

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
  EZ_DECLARE_AICMD(ezAiCommandSlide);

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

class EZ_GAMEENGINE_DLL ezAiCommandTurnTowards : public ezAiCommand
{
  EZ_DECLARE_AICMD(ezAiCommandTurnTowards);

public:
  ezAiCommandTurnTowards();
  ~ezAiCommandTurnTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTargetPosition = ezVec3::ZeroVector();
  ezGameObjectHandle m_hTargetObject;
  ezAngle m_TargetReachedAngle = ezAngle::Degree(5);
  ezAngle m_TurnAnglesPerSec;
};

class EZ_GAMEENGINE_DLL ezAiCommandFollowPath : public ezAiCommand
{
  EZ_DECLARE_AICMD(ezAiCommandFollowPath);

public:
  ezAiCommandFollowPath();
  ~ezAiCommandFollowPath();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCommandResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezGameObjectHandle m_hPath;
  float m_fSpeed = 0.0f;
};
