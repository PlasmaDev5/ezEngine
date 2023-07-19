#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/AI/AiActionQueue.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiActionQueue::ezAiActionQueue() = default;

ezAiActionQueue::~ezAiActionQueue()
{
  ClearQueue();
}

void ezAiActionQueue::Cancel(ezGameObject* pOwner)
{
  for (ezUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    m_Queue[i]->Cancel(pOwner);
  }
}

void ezAiActionQueue::ClearQueue()
{
  for (auto pCmd : m_Queue)
  {
    pCmd->Destroy();
  }

  m_Queue.Clear();
}

bool ezAiActionQueue::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

void ezAiActionQueue::AddAction(ezAiAction* pAction)
{
  m_Queue.PushBack(pAction);
}

void ezAiActionQueue::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  EZ_LOG_BLOCK("AiActionExecution");
  EZ_PROFILE_SCOPE("AiActionExecution");

  while (!m_Queue.IsEmpty())
  {
    ezAiAction* pCmd = m_Queue[0];

    const ezAiActionResult res = pCmd->Execute(pOwner, tDiff);
    if (res == ezAiActionResult::Succeded)
      return;

    if (res == ezAiActionResult::Failed)
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

void ezAiActionQueue::PrintDebugInfo(ezGameObject* pOwner)
{
  ezStringBuilder str;

  if (m_Queue.IsEmpty())
  {
    str = "<AI action queue empty>";
    ezDebugRenderer::DrawInfoText(pOwner->GetWorld(), ezDebugRenderer::ScreenPlacement::BottomRight, "AI", str, ezColor::Orange);
  }
  else
  {
    m_Queue[0]->GetDebugDesc(str);
    ezDebugRenderer::DrawInfoText(pOwner->GetWorld(), ezDebugRenderer::ScreenPlacement::BottomRight, "AI", str);
  }
}
