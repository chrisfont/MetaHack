#include "systems/SystemDirector.h"

#include "components/ComponentActivity.h"
#include "components/ComponentInventory.h"
#include "components/ComponentManager.h"
#include "game/GameState.h"
#include "map/Map.h"
#include "systems/SystemManager.h"

SystemDirector::SystemDirector(GameState& gameState,
                               SystemManager& systems) :
  SystemCRTP<SystemDirector>({}),
  m_gameState{ gameState },
  m_systems{ systems }
{}

SystemDirector::~SystemDirector()
{}

void SystemDirector::doCycleUpdate()
{

}

void SystemDirector::processMap(MapID mapID)
{
  auto& gameMap = m_gameState.maps().get(mapID);
  auto mapSize = gameMap.getSize();

  for (int y = 0; y < mapSize.y; ++y)
  {
    for (int x = 0; x < mapSize.x; ++x)
    {
      EntityId contents = gameMap.getTile({ x, y }).getTileContents();
      processEntityAndChildren(contents);
    }
  }

  //notifyObservers(Event::Updated);
}

void SystemDirector::processEntityAndChildren(EntityId entityID)
{
  // Get a copy of the Entity's inventory.
  // This is because entities can be deleted/removed from the inventory
  // over the course of processing them, and this could invalidate the
  // iterator.
  ComponentInventory tempInventoryCopy{ m_gameState.components().inventory.of(entityID) };

  // Process inventory.
  for (auto& inventoryPair : tempInventoryCopy)
  {
    EntityId child = inventoryPair.second;
    processEntityAndChildren(child);
  }

  // Process self last.
  processEntity(entityID);
}

void SystemDirector::processEntity(EntityId entityID)
{
  if (!m_gameState.components().activity.existsFor(entityID)) return;

  auto& activity = m_gameState.components().activity.of(entityID);

  /// @todo This all gets moved into the "GrimReaper" system.
  //// Is this an entity that is now dead?
  //if (COMPONENTS.health.existsFor(m_id) &&
  //    COMPONENTS.health[m_id].hasHpBelowZero())
  //{
  //  // Did the entity JUST die?
  //  if (!COMPONENTS.health[m_id].isDead())
  //  {
  //    // Perform the "die" action.
  //    // (This sets the "dead" property and clears out any pending actions.)
  //    std::unique_ptr<Actions::Action> dieAction(NEW Actions::ActionDie(getId()));
  //    this->queueInvoluntaryAction(std::move(dieAction));
  //  }
  //}

  // If there are pending actions...
  if (!activity.pendingActions().empty())
  {
    bool entity_updated = false;

    do
    {
      /// Process the front action until we are marked as busy, 
      /// or the action is done.
      /// @todo Find a way to update the entity_updated variable.
      Actions::Action& action = activity.pendingActions().front();
      bool action_done = action.process(m_gameState, m_systems);
      if (action_done)
      {
        CLOG(TRACE, "Entity") << "Entity " <<
          entityID << " (" <<
          m_gameState.components().category[entityID] << "): Action " <<
          action.getType() << " is done, popping";

        activity.pendingActions().pop();
      }
    } while (!activity.pendingActions().empty() && activity.busyTicks() == 0); // loop while (actions pending and not busy)

    if (entity_updated)
    {
      /// @todo This needs to be changed so it only is called if the Action
      ///       materially affected the entity in some way. Two ways to do this
      ///       that I can see:
      ///         1) The Action calls notifyObservers. Requires the Action
      ///            to have access to that method; right now it doesn't.
      ///         2) The Action returns some sort of indication that the Entity
      ///            was modified as a result. This could be done by changing
      ///            the return type from a bool to a struct of some sort.
      //notifyObservers(Event::Updated);
    }

  } // end if (actions pending)
    // Otherwise if there are no pending actions...
  else
  {
    // If entity is not the player, call the Lua process function on this 
    // Entity, which runs the AI and may queue new actions.
    if (m_gameState.components().globals.player() != entityID)
    {
      /// @todo Re-implement me
      //(void)call_lua_function("process", {}, true);
    }
  }
}

void SystemDirector::setMap_V(MapID newMap)
{}

bool SystemDirector::onEvent(Event const & event)
{
  return false;
}

