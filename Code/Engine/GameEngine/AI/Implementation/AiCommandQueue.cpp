#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/AI/AiCommandQueue.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiCmdQueue::ezAiCmdQueue() = default;

ezAiCmdQueue::~ezAiCmdQueue()
{
  ClearQueue();
}

void ezAiCmdQueue::Cancel(ezGameObject* pOwner)
{
  for (ezUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    m_Queue[i]->Cancel(pOwner);
  }
}

void ezAiCmdQueue::ClearQueue()
{
  for (auto pCmd : m_Queue)
  {
    pCmd->Destroy();
  }

  m_Queue.Clear();
}

bool ezAiCmdQueue::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

void ezAiCmdQueue::AddCommand(ezAiCmd* pCommand)
{
  m_Queue.PushBack(pCommand);
}

void ezAiCmdQueue::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  EZ_LOG_BLOCK("AiCommandExecution");
  EZ_PROFILE_SCOPE("AiCommandExecution");

  while (!m_Queue.IsEmpty())
  {
    ezAiCmd* pCmd = m_Queue[0];

    const ezAiCmdResult res = pCmd->Execute(pOwner, tDiff);
    if (res == ezAiCmdResult::Succeded)
      return;

    if (res == ezAiCmdResult::Failed)
    {
      ezStringBuilder str;
      pCmd->GetDebugDesc(str);
      ezLog::Error("AI cmd failed: {}", str);

      Cancel(pOwner);
    }

    pCmd->Destroy();
    m_Queue.RemoveAtAndCopy(0);
  }
}

void ezAiCmdQueue::PrintDebugInfo(ezGameObject* pOwner)
{
  ezStringBuilder str;

  if (m_Queue.IsEmpty())
  {
    str = "<AI command queue empty>";
    ezDebugRenderer::DrawInfoText(pOwner->GetWorld(), ezDebugRenderer::ScreenPlacement::BottomRight, "AI", str, ezColor::Orange);
  }
  else
  {
    m_Queue[0]->GetDebugDesc(str);
    ezDebugRenderer::DrawInfoText(pOwner->GetWorld(), ezDebugRenderer::ScreenPlacement::BottomRight, "AI", str);
  }
}
