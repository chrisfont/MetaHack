#include "systems/SystemTimekeeper.h"

SystemTimekeeper::SystemTimekeeper(ComponentGlobals & globals) :
  SystemCRTP<SystemTimekeeper>({ EventClockChanged::id }),
  m_globals{ globals }  
{}

SystemTimekeeper::~SystemTimekeeper()
{}

void SystemTimekeeper::doCycleUpdate()
{
  incrementClock(ElapsedTicks(1));
}

ElapsedTicks SystemTimekeeper::clock() const
{
  return m_clock;
}

void SystemTimekeeper::setClock(ElapsedTicks clock)
{
  m_clock = clock;
  broadcast(EventClockChanged(clock));
}

void SystemTimekeeper::incrementClock(ElapsedTicks addedTime)
{
  /// @todo Check for the unlikely, but not impossible, chance of rollover.
  setClock(clock() + addedTime);
}

void SystemTimekeeper::setMapNVO(MapID newMap)
{}

bool SystemTimekeeper::onEvent(Event const & event)
{
  return false;
}
