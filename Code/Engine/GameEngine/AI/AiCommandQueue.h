#pragma once

#include <GameEngine/AI/AiCommand.h>

class EZ_GAMEENGINE_DLL ezAiCmdQueue
{
public:
  ezAiCmdQueue();
  ~ezAiCmdQueue();

  void Cancel(ezGameObject* pOwner);

  bool IsEmpty() const;

  void AddCommand(ezAiCmd* pCommand);

  void Execute(ezGameObject* pOwner, ezTime tDiff);

  void PrintDebugInfo(ezGameObject* pOwner);

  void ClearQueue();

private:

  ezHybridArray<ezAiCmd*, 16> m_Queue;
};
