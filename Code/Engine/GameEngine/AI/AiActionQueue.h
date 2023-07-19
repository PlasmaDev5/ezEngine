#pragma once

#include <GameEngine/AI/AiAction.h>

class EZ_GAMEENGINE_DLL ezAiActionQueue
{
public:
  ezAiActionQueue();
  ~ezAiActionQueue();

  void Cancel(ezGameObject* pOwner);

  bool IsEmpty() const;

  void AddAction(ezAiAction* pAction);

  void Execute(ezGameObject* pOwner, ezTime tDiff);

  void PrintDebugInfo(ezGameObject* pOwner);

  void ClearQueue();

private:

  ezHybridArray<ezAiAction*, 16> m_Queue;
};
