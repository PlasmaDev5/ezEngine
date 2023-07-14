#pragma once

#include <Foundation/Strings/HashedString.h>
#include <GameEngine/GameEngineDLL.h>

class ezGameObject;

enum class [[nodiscard]] ezAiCmdResult
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmd
{
public:
  ezAiCmd();
  virtual ~ezAiCmd();

  virtual void Reset() = 0;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) = 0;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) = 0;
  virtual void Cancel(ezGameObject* pOwner) = 0;

private:
  friend class ezAiCmdQueue;
  virtual void Destroy() = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdWait : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdWait);

public:
  ezAiCmdWait();
  ~ezAiCmdWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTime m_Duration;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdLerpRotation : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdLerpRotation);

public:
  ezAiCmdLerpRotation();
  ~ezAiCmdLerpRotation();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTurnAxis = ezVec3::UnitZAxis();
  ezAngle m_TurnAngle;
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdLerpPosition : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdLerpPosition);

public:
  ezAiCmdLerpPosition();
  ~ezAiCmdLerpPosition();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  float m_fSpeed = 0.0f;
  ezVec3 m_vLocalSpaceSlide = ezVec3::ZeroVector();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdLerpRotationTowards : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdLerpRotationTowards);

public:
  ezAiCmdLerpRotationTowards();
  ~ezAiCmdLerpRotationTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTargetPosition = ezVec3::ZeroVector();
  ezGameObjectHandle m_hTargetObject;
  ezAngle m_TargetReachedAngle = ezAngle::Degree(5);
  ezAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// class EZ_GAMEENGINE_DLL ezAiCmdFollowPath : public ezAiCmd
//{
//   EZ_DECLARE_AICMD(ezAiCmdFollowPath);
//
// public:
//   ezAiCmdFollowPath();
//   ~ezAiCmdFollowPath();
//
//   virtual void Reset() override;
//   virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
//   virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
//   virtual void Cancel(ezGameObject* pOwner) override;
//
//   ezGameObjectHandle m_hPath;
//   float m_fSpeed = 0.0f;
// };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdBlackboardSetEntry : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdBlackboardSetEntry);

public:
  ezAiCmdBlackboardSetEntry();
  ~ezAiCmdBlackboardSetEntry();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTempHashedString m_sEntryName;
  ezVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdBlackboardWait : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdBlackboardWait);

public:
  ezAiCmdBlackboardWait();
  ~ezAiCmdBlackboardWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezTempHashedString m_sEntryName;
  ezVariant m_Value;
  bool m_bEquals = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiCmdCCMoveTo : public ezAiCmd
{
  EZ_DECLARE_AICMD(ezAiCmdCCMoveTo);

public:
  ezAiCmdCCMoveTo();
  ~ezAiCmdCCMoveTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(ezStringBuilder& inout_sText) override;
  virtual ezAiCmdResult Execute(ezGameObject* pOwner, ezTime tDiff) override;
  virtual void Cancel(ezGameObject* pOwner) override;

  ezVec3 m_vTargetPosition = ezVec3::ZeroVector();
  ezGameObjectHandle m_hTargetObject;
  float m_fSpeed = 0.0f;
  float m_fReachedDistSQR = 1.0f;
};
