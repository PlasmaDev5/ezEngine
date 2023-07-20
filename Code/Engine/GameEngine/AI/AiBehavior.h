#pragma once

#include <GameEngine/AI/AiPerceptionGenerator.h>
#include <GameEngine/GameEngineDLL.h>

class ezAiBehavior;

struct ezAiBehaviorScore
{
  ezAiBehavior* m_pBehavior = nullptr;
  const ezAiPerception* m_pPerception = nullptr;
  float m_fScore = 0.0f;
};

class EZ_GAMEENGINE_DLL ezAiBehavior
{
public:
  ezAiBehavior();
  virtual ~ezAiBehavior();

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) = 0;
  virtual void SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiBehaviorGoToPOI : public ezAiBehavior
{
public:
  ezAiBehaviorGoToPOI();
  ~ezAiBehaviorGoToPOI();

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiBehaviorWander : public ezAiBehavior
{
public:
  ezAiBehaviorWander();
  ~ezAiBehaviorWander();

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_GAMEENGINE_DLL ezAiBehaviorShoot : public ezAiBehavior
{
public:
  ezAiBehaviorShoot();
  ~ezAiBehaviorShoot();

  virtual ezAiBehaviorScore DetermineBehaviorScore(ezGameObject& owner, const ezAiPerceptionManager& perceptionManager) override;
  virtual void SetUpActions(ezGameObject& owner, const ezAiPerception* pPerception, ezAiActionQueue& inout_ActionQueue) override;
};
