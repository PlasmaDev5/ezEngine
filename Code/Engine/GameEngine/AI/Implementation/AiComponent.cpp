#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AiComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezAiComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DebugInfo", m_bDebugInfo),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Components"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezAiComponent::ezAiComponent() = default;
ezAiComponent::~ezAiComponent() = default;

void ezAiComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bDebugInfo;
}

void ezAiComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bDebugInfo;
}

void ezAiComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void ezAiComponent::Update()
{
  if (m_CommandQueue.IsEmpty())
  {
    ezRandom& rng = GetWorld()->GetRandomNumberGenerator();

    m_CmdWait.m_Duration = ezTime::Seconds(rng.DoubleMinMax(0.1, 1.0));
    m_CommandQueue.AddCommand(&m_CmdWait);

    //m_CmdTurn.m_vTurnAxis.Set(0, 0, 1);
    //m_CmdTurn.m_TurnAnglesPerSec = ezAngle::Degree(rng.DoubleMinMax(20.0f, 90.0f));
    //m_CmdTurn.m_TurnAngle = ezAngle::Degree(rng.DoubleMinMax(-90.0f, 90.0f));
    //m_CommandQueue.AddCommand(&m_CmdTurn);

    m_CmdSlide.m_vLocalSpaceSlide.x = rng.DoubleMinMax(-1.0, 1.0);
    // m_CmdSlide.m_vLocalSpaceSlide.y = rng.DoubleMinMax(-0.5, 0.5);
    m_CmdSlide.m_fSpeed = rng.DoubleMinMax(0.25, 2.0);
    m_CommandQueue.AddCommand(&m_CmdSlide);

    ezGameObject* pPlayer;
    if (GetWorld()->TryGetObjectWithGlobalKey("Player", pPlayer))
    {
      m_CmdTurnTowards.m_hTargetObject = pPlayer->GetHandle();
      m_CmdTurnTowards.m_TurnAnglesPerSec = ezAngle::Degree(90);
      m_CommandQueue.AddCommand(&m_CmdTurnTowards);
    }
  }

  m_CommandQueue.Execute(GetOwner(), GetWorld()->GetClock().GetTimeDiff());

  if (m_bDebugInfo)
  {
    m_CommandQueue.PrintDebugInfo(GetOwner());
  }
}
