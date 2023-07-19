#pragma once

#include <GameEngine/GameEngineDLL.h>

class EZ_GAMEENGINE_DLL ezAiGoal
{
public:
  ezAiGoal();
  ~ezAiGoal();
};



class EZ_GAMEENGINE_DLL ezAiGoalPOI : public ezAiGoal
{
public:
  ezAiGoalPOI();
  ~ezAiGoalPOI();

  ezVec3 m_vGlobalPosition = ezVec3::ZeroVector();
};


class EZ_GAMEENGINE_DLL ezAiGoalWander : public ezAiGoal
{
public:
  ezAiGoalWander();
  ~ezAiGoalWander();

  ezVec3 m_vGlobalPosition = ezVec3::ZeroVector();
};
