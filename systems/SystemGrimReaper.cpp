#include "stdafx.h"

#include "systems/SystemGrimReaper.h"

namespace Systems
{

  GrimReaper::GrimReaper(Components::ComponentGlobals & globals) :
    CRTP<GrimReaper>({ EventEntityDied::id,
                                   EventEntityMarkedForDeath::id }),
    m_globals{ globals }
  {}

  GrimReaper::~GrimReaper()
  {}

  void GrimReaper::doCycleUpdate()
  {
    // if (something_or_other) return GAME.lua().callEntityFunction("do_die", m_id, {}, true);
  }

  void GrimReaper::setMap_V(MapID newMap)
  {}

  bool GrimReaper::onEvent(Event const & event)
  {
    return false;
  }

} // end namespace Systems