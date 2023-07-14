#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/AI/AiCommandQueue.h>
#include <RendererCore/Debug/DebugRenderer.h>

ezAiCommandQueue::ezAiCommandQueue() = default;
ezAiCommandQueue::~ezAiCommandQueue() = default;

void ezAiCommandQueue::Cancel(ezGameObject* pOwner)
{
  for (ezUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    m_Queue[i]->Cancel(pOwner);
  }
}

void ezAiCommandQueue::ClearQueue()
{
  for (auto pCmd : m_Queue)
  {
    pCmd->Destroy();
  }

  m_Queue.Clear();
}

bool ezAiCommandQueue::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

void ezAiCommandQueue::AddCommand(ezAiCommand* pCommand)
{
  m_Queue.PushBack(pCommand);
}

void ezAiCommandQueue::Execute(ezGameObject* pOwner, ezTime tDiff)
{
  EZ_LOG_BLOCK("AiCommandExecution");
  EZ_PROFILE_SCOPE("AiCommandExecution");

  while (!m_Queue.IsEmpty())
  {
    ezAiCommand* pCmd = m_Queue[0];

    const ezAiCommandResult res = pCmd->Execute(pOwner, tDiff);
    if (res == ezAiCommandResult::Succeded)
      return;

    if (res == ezAiCommandResult::Failed)
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

void ezAiCommandQueue::PrintDebugInfo(ezGameObject* pOwner)
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
