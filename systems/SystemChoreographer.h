#pragma once

#include "components/ComponentGlobals.h"
#include "entity/EntityId.h"
#include "systems/CRTP.h"

namespace Systems
{

  /// System that handles which entity is the player.
  /// May also eventually be in charge of entity AI and/or spawning -- we'll see.
  class Choreographer : public CRTP<Choreographer>
  {
  public:
    struct EventPlayerChanged : public ConcreteEvent<EventPlayerChanged>
    {
      EventPlayerChanged(EntityId player_) :
        player{ player_ }
      {}

      EntityId const player;

      void printToStream(std::ostream& os) const
      {
        Event::printToStream(os);
        os << " | current player: " << player;
      }
    };

    Choreographer(Components::ComponentGlobals& globals);

    virtual ~Choreographer();

    /// Recalculate whatever needs recalculating.
    virtual void doCycleUpdate() override;

    /// Get current player.
    EntityId player() const;

    /// Set current player.
    void setPlayer(EntityId entity);

  protected:
    virtual void setMap_V(MapID newMap) override;

    virtual bool onEvent(Event const& event) override;

  private:
    // Components used by this system.
    Components::ComponentGlobals& m_globals;
  };

} // end namespace Systems
