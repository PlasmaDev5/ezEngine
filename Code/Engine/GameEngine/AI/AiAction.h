#pragma once

#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class ezGameObject;

enum class [[nodiscard]] ezAiActionResult
{
  Succeded, ///< Finished for this frame, but needs to be executed again.
  Finished, ///< Completely finished (or canceled), does not need to be executed again.
  Failed,   ///< Failed and should not be executed again.
};

template <typename TYPE>
class ezAiActionAlloc
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
  static ezAiActionAlloc<OwnType> s_Allocator;

#define EZ_IMPLEMENT_AICMD(OwnType) \
  ezAiActionAlloc<OwnType> OwnType::s_Allocator;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiAction
{
public:
  ezAiAction();
  virtual ~ezAiAction();

  virtual void Reset() = 0;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) = 0;
  virtual void Cancel(ezGameObject* pOwner) = 0;

private:
  friend class ezAiActionQueue;
  virtual void Destroy() = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionWait);

public:
  ezAiActionWait();
  ~ezAiActionWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTime m_Duration;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionLerpRotation : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpRotation);

public:
  ezAiActionLerpRotation();
  ~ezAiActionLerpRotation();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTurnAxis = ezVec3::UnitZAxis();
  ezAngle m_TurnAngle;
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionLerpPosition : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpPosition);

public:
  ezAiActionLerpPosition();
  ~ezAiActionLerpPosition();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  float m_fSpeed = 0.0f;
  ezVec3 m_vLocalSpaceSlide = ezVec3::ZeroVector();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionLerpRotationTowards : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionLerpRotationTowards);

public:
  ezAiActionLerpRotationTowards();
  ~ezAiActionLerpRotationTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTargetPosition = ezVec3::ZeroVector();
  ezGameObjectHandle m_hTargetObject;
  ezAngle m_TargetReachedAngle = ezAngle::Degree(5);
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// class EZ_GAMEENGINE_DLL ezAiActionFollowPath : public ezAiAction
//{
//   EZ_DECLARE_AICMD(ezAiActionFollowPath);
//
// public:
//   ezAiActionFollowPath();
//   ~ezAiActionFollowPath();
//
//   virtual void Reset() override;
//   virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
//   virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
//   virtual void Cancel(ezGameObject* pOwner) override;
//
//   ezGameObjectHandle m_hPath;
//   float m_fSpeed = 0.0f;
// };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionBlackboardSetEntry : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardSetEntry);

public:
  ezAiActionBlackboardSetEntry();
  ~ezAiActionBlackboardSetEntry();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  bool m_bNoCancel = false;
  ezTempHashedString m_sEntryName;
  ezVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionBlackboardWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardWait);

public:
  ezAiActionBlackboardWait();
  ~ezAiActionBlackboardWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTempHashedString m_sEntryName;
  ezVariant m_Value;
  bool m_bEquals = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionBlackboardSetAndWait : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionBlackboardSetAndWait);

public:
  ezAiActionBlackboardSetAndWait();
  ~ezAiActionBlackboardSetAndWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTempHashedString m_sEntryName;
  ezVariant m_SetValue;
  ezVariant m_WaitValue;
  bool m_bEqualsWaitValue = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionCCMoveTo : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionCCMoveTo);

public:
  ezAiActionCCMoveTo();
  ~ezAiActionCCMoveTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTargetPosition = ezVec3::ZeroVector();
  ezGameObjectHandle m_hTargetObject;
  float m_fSpeed = 0.0f;
  float m_fReachedDistSQR = 1.0f;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiActionSpawn : public ezAiAction
{
  EZ_DECLARE_AICMD(ezAiActionSpawn);

public:
  ezAiActionSpawn();
  ~ezAiActionSpawn();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiActionResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTempHashedString m_sChildObjectName;
};
