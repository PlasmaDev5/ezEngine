#pragma once

#include <GameEngine/AI/AiCommand.h>

class EZ_GAMEENGINE_DLL ezAiCommandQueue
{
public:
  ezAiCommandQueue();
  ~ezAiCommandQueue();

  void Cancel(ezGameObject* pOwner);

  bool IsEmpty() const;

  void AddCommand(ezAiCommand* pCommand);

  void Execute(ezGameObject* pOwner, ezTime tDiff);

  void PrintDebugInfo(ezGameObject* pOwner);

private:
  void ClearQueue();

  ezHybridArray<ezAiCommand*, 16> m_Queue;
};
