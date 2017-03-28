#include "stdafx.h"

#include "ActionMove.h"

#include "ActionAttack.h"
#include "game/GameState.h"
#include "services/IMessageLog.h"
#include "services/IStringDictionary.h"
#include "map/Map.h"
#include "Service.h"
#include "entity/Entity.h"
#include "entity/EntityId.h"

namespace Actions
{
  ActionMove ActionMove::prototype;
  ActionMove::ActionMove() : Action("move", "MOVE", ActionMove::create_) {}
  ActionMove::ActionMove(EntityId subject) : Action(subject, "move", "MOVE") {}
  ActionMove::~ActionMove() {}

  std::unordered_set<Trait> const & ActionMove::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbDirection,
      Trait::SubjectCanNotBeInsideAnotherObject
    };

    return traits;
  }

  StateResult ActionMove::doPreBeginWorkNVI(AnyMap& params)
  {
    // All checks handled in Action by traits.
    return StateResult::Success();
  }

  StateResult ActionMove::doBeginWorkNVI(AnyMap& params)
  {
    StateResult result = StateResult::Failure();

    std::string message;

    auto subject = getSubject();
    EntityId location = subject->getLocation();
    MapTile* current_tile = subject->getMapTile();
    Direction new_direction = getTargetDirection();

    if (new_direction == Direction::Up)
    {
      /// @todo Write up/down movement code
      message = maketr("ACTN_NOT_IMPLEMENTED");
      Service<IMessageLog>::get().add(message);
    }
    else if (new_direction == Direction::Down)
    {
      /// @todo Write up/down movement code
      message = maketr("ACTN_NOT_IMPLEMENTED");
      Service<IMessageLog>::get().add(message);
    }
    else
    {
      // Figure out our target location.
      IntVec2 coords = current_tile->getCoords();
      IntVec2 offset = (IntVec2)new_direction;
      int x_new = coords.x + offset.x;
      int y_new = coords.y + offset.y;
      Map& current_map = GAME.getMaps().get(subject->getMapId());
      IntVec2 map_size = current_map.getSize();

      // Check boundaries.
      if ((x_new < 0) || (y_new < 0) ||
        (x_new >= map_size.x) || (y_new >= map_size.y))
      {
        message += maketr("YOU_CANT_VERB_THERE");
        Service<IMessageLog>::get().add(message);
      }
      else
      {
        auto& new_tile = current_map.getTile({ x_new, y_new });
        EntityId new_floor = new_tile.getTileContents();

        // See if the tile to move into contains another creature.
        EntityId creature = new_floor->getInventory().getEntity();
        if (creature != EntityId::Mu())
        {
          /// @todo Setting choosing whether auto-attack is on.
          /// @todo Only attack hostiles.
          std::unique_ptr<ActionAttack> action_attack{ new ActionAttack(subject) };
          action_attack->setTarget(new_direction);

          subject->queueAction(std::move(action_attack));

          result = StateResult::Success();
        }
        else
        {
          if (new_tile.canBeTraversedBy(subject))
          {
            /// @todo Figure out elapsed movement time.
            result.success = subject->move_into(new_floor);
            result.elapsed_time = 1;
          }
          else
          {
            std::string tile_description = new_tile.getDisplayName();
            message += maketr("YOU_ARE_STOPPED_BY",
            {
              getIndefArt(tile_description),
              tile_description
            });

            Service<IMessageLog>::get().add(message);
          }
        } // end else if (tile does not contain creature)
      } // end else if (not out of bounds)
    } // end else if (other direction)

    return result;
  }

  StateResult ActionMove::doFinishWorkNVI(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionMove::doAbortWorkNVI(AnyMap& params)
  {
    return StateResult::Success();
  }
} // end namespace

