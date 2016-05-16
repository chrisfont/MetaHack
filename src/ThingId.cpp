#include "stdafx.h"

#include "GameState.h"
#include "ThingId.h"
#include "Thing.h"
#include "ThingManager.h"

Thing* ThingId::operator->()
{
  return &(GAME.get_thing_manager().get(m_id));
}

Thing const* ThingId::operator->() const
{
  return &(GAME.get_thing_manager().get(m_id));
}