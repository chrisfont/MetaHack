#include "stdafx.h"

#include "systems/Manager.h"

#include "components/ComponentManager.h"
#include "systems/SystemChoreographer.h"
#include "systems/SystemDirector.h"
#include "systems/SystemEditor.h"
#include "systems/SystemFluidics.h"
#include "systems/SystemGeometry.h"
#include "systems/SystemGrimReaper.h"
#include "systems/SystemJanitor.h"
#include "systems/SystemLighting.h"
#include "systems/SystemLuaLiaison.h"
#include "systems/SystemMechanics.h"
#include "systems/SystemNarrator.h"
#include "systems/SystemSenseSight.h"
#include "systems/SystemThermodynamics.h"
#include "systems/SystemTimekeeper.h"
#include "utilities/New.h"

namespace Systems
{

  Manager* Manager::s_instance = nullptr;

  Manager::Manager(GameState& gameState) :
    m_gameState{ gameState }
  {
    auto& components = m_gameState.components();

    // Set instance pointer.
    Assert("Systems", s_instance == nullptr, "tried to create more than one Manager instance at a time");
    s_instance = this;


    // Initialize primary systems.

    // These systems come first since others may need to reference them.
    /// @todo I don't like one System coupled to another, but I don't know how
    ///       else to architect this nicely.
    m_janitor.reset(NEW Janitor(components));
    m_luaLiaison.reset(NEW LuaLiaison(m_gameState, *this));
    m_narrator.reset(NEW Narrator(components));

    // Initialize the remaining systems.
    m_choreographer.reset(NEW Choreographer(components.globals));

    m_director.reset(NEW Director(m_gameState, *this));

    m_editor.reset(NEW Editor(components.globals,
                              components.modifiers));

    m_fluidics.reset(NEW Fluidics());

    m_geometry.reset(NEW Geometry(*m_janitor,
                                  *m_narrator,
                                  components.globals,
                                  components.inventory,
                                  components.position));

    m_grimReaper.reset(NEW GrimReaper(components.globals));

    m_lighting.reset(NEW Lighting(m_gameState,
                                  components.appearance,
                                  components.health,
                                  components.lightSource,
                                  components.position));

    m_mechanics.reset(NEW Mechanics());

    m_senseSight.reset(NEW SenseSight(m_gameState,
                                      components.inventory,
                                      components.position,
                                      components.senseSight,
                                      components.spacialMemory));

    m_thermodynamics.reset(NEW Thermodynamics());

    m_timekeeper.reset(NEW Timekeeper(components.globals));

    // Link system events.
    m_geometry->subscribeTo(m_janitor.get(), Janitor::EventEntityMarkedForDeletion::id);

    m_director->subscribeTo(m_geometry.get(), Geometry::EventEntityChangedMaps::id);

    m_lighting->subscribeTo(m_geometry.get(), Geometry::EventEntityChangedMaps::id);
    m_lighting->subscribeTo(m_geometry.get(), Geometry::EventEntityMoved::id);

    m_senseSight->subscribeTo(m_geometry.get(), Geometry::EventEntityChangedMaps::id);
    m_senseSight->subscribeTo(m_geometry.get(), Geometry::EventEntityMoved::id);

  }

  Manager::~Manager()
  {
    // Dissolve links.
    m_geometry->removeAllObservers();
    m_janitor->removeAllObservers();

    s_instance = nullptr;
  }

  void Manager::runOneCycle()
  {
    m_director->doCycleUpdate();
    m_choreographer->doCycleUpdate();
    m_editor->doCycleUpdate();

    m_lighting->doCycleUpdate();
    m_senseSight->doCycleUpdate();
    //m_senseHearing->doCycleUpdate();
    //m_senseSmell->doCycleUpdate();
    //m_senseTouch->doCycleUpdate();
    m_geometry->doCycleUpdate();
    m_mechanics->doCycleUpdate();
    m_fluidics->doCycleUpdate();
    m_thermodynamics->doCycleUpdate();

    m_grimReaper->doCycleUpdate();
    m_janitor->doCycleUpdate();
    m_timekeeper->doCycleUpdate();
  }

  Manager & Manager::instance()
  {
    Assert("Systems", s_instance != nullptr, "tried to get non-existent Manager instance");

    return *(s_instance);
  }

} // end namespace Systems
