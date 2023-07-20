#pragma once

#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezAiPerception
{
public:
  ezAiPerception();
  ~ezAiPerception();
};



class EZ_GAMEENGINE_DLL ezAiPerceptionPOI : public ezAiPerception
{
public:
  ezAiPerceptionPOI();
  ~ezAiPerceptionPOI();

  ezVec3 m_vGlobalPosition = ezVec3::ZeroVector();
};


class EZ_GAMEENGINE_DLL ezAiPerceptionWander : public ezAiPerception
{
public:
  ezAiPerceptionWander();
  ~ezAiPerceptionWander();

  ezVec3 m_vGlobalPosition = ezVec3::ZeroVector();
};
