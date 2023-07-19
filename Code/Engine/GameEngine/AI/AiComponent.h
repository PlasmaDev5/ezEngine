#pragma once

#include <Core/World/World.h>
#include <GameEngine/AI/AiActionQueue.h>

using ezAiComponentManager = ezComponentManagerSimple<class ezAiComponent, ezComponentUpdateType::WhenSimulating>;

class EZ_GAMEENGINE_DLL ezAiComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiComponent, ezComponent, ezAiComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiComponent

  void Update();

public:
  ezAiComponent();
  ~ezAiComponent();

  bool m_bDebugInfo = false;

protected:
  void DoSensorCheck();
  void FillCmdQueue();

  ezAiActionQueue m_ActionQueue;
  ezTime m_LastAiUpdate;
};
