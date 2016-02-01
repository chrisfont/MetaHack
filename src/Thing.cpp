#include "Thing.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>

#include <cinttypes>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "App.h"
#include "ConfigSettings.h"
#include "Direction.h"
#include "GameState.h"
#include "Gender.h"
#include "IntegerRange.h"
#include "Inventory.h"
#include "Map.h"
#include "MapTile.h"
#include "MapTileMetadata.h"
#include "MathUtils.h"
#include "MessageLog.h"
#include "Metadata.h"
#include "Ordinal.h"
#include "ThingManager.h"
#include "TileSheet.h"

// Local definitions to make reading/writing status info a bit easier.
#define YOU       (this->get_identifying_string())  // "you" or descriptive noun like "the goblin"
#define YOU_SUBJ  (this->get_subject_pronoun())     // "you/he/she/it/etc."
#define YOU_OBJ   (this->get_object_pronoun())      // "you/him/her/it/etc."
#define YOUR      (this->get_possessive())          // "your/his/her/its/etc."
#define YOURSELF  (this->get_reflexive_pronoun())   // "yourself/himself/herself/itself/etc."

#define CV(p12, p3)  (this->choose_verb(p12, p3))   // shortcut for "Choose Verb"

#define ARE   (this->choose_verb(" are", " is"))
#define WERE  (this->choose_verb(" were", " was"))
#define DO    (this->choose_verb(" do", " does"))
#define GET   (this->choose_verb(" get", " gets"))
#define HAVE  (this->choose_verb(" have", " has"))
#define SEEM  (this->choose_verb(" seem", " seems"))
#define TRY   (this->choose_verb(" try", " tries"))

#define IS_PLAYER (GAME.get_player() == pImpl->ref)

#define FOO       (thing->get_identifying_string())
#define FOO1      (thing1->get_identifying_string())
#define FOO2      (thing2->get_identifying_string())
#define LIQUID1      (liquid1->get_identifying_string())
#define LIQUID2      (liquid2->get_identifying_string())

#define YOU_ARE       (YOU + ARE)
#define YOU_WERE      (YOU + WERE)
#define YOU_SUBJ_ARE  (YOU_SUBJ + ARE)
#define YOU_DO        (YOU + DO)
#define YOU_GET       (YOU + GET)
#define YOU_HAVE      (YOU + HAVE)
#define YOU_SEEM      (YOU + SEEM)
#define YOU_TRY       (YOU + TRY)

#define YOU_TRY_TO(verb) (YOU_TRY + " to " + verb + " ")

// Static member initialization.
sf::Color const Thing::wall_outline_color_ = sf::Color(255, 255, 255, 64);

Thing::Thing(Metadata& metadata, ThingRef ref)
  : pImpl(metadata, ref)
{
  initialize();
}

Thing::Thing(MapTile* map_tile, Metadata& metadata, ThingRef ref)
  : pImpl(map_tile, metadata, ref)
{
  initialize();
}

Thing::Thing(Thing const& original, ThingRef ref)
  : pImpl(*(original.pImpl), ref)
{
  initialize();
}

void Thing::initialize()
{
  /// Get the max_hp IntegerRange for this Thing (if any).
  IntegerRange max_hp_range = pImpl->metadata.get_default<IntegerRange>("maxhp");

  /// Pick a number and set it as our maximum HP.
  int max_hp = max_hp_range.pick();
  set_property<int>("maxhp", max_hp);

  /// Also set our HP to that value.
  set_property<int>("hp", max_hp);
}

Thing::~Thing()
{
}

void Thing::queue_action(std::unique_ptr<Action> pAction)
{
  pImpl->pending_actions.push_back(std::move(pAction));
}

bool Thing::action_is_pending() const
{
  return !(pImpl->pending_actions.empty());
}

bool Thing::action_is_in_progress()
{
  return (get_property<int>("counter_busy") > 0);
}

bool Thing::is_wielding(ThingRef thing)
{
  unsigned int dummy;
  return is_wielding(thing, dummy);
}

bool Thing::is_wielding(ThingRef thing, unsigned int& hand)
{
  return pImpl->is_wielding(thing, hand);
}

bool Thing::has_equipped(ThingRef thing)
{
  WearLocation dummy;
  return has_equipped(thing, dummy);
}

bool Thing::has_equipped(ThingRef thing, WearLocation& location)
{
  return pImpl->is_wearing(thing, location);
}

bool Thing::can_reach(ThingRef thing)
{
  // Check if it is our location.
  auto our_location = get_location();
  if (our_location == thing)
  {
    return true;
  }

  // Check if it's at our location.
  auto thing_location = thing->get_location();
  if (our_location == thing_location)
  {
    return true;
  }

  // Check if it's in our inventory.
  if (this->get_inventory().contains(thing))
  {
    return true;
  }

  return false;
}

bool Thing::is_adjacent_to(ThingRef thing)
{
  // Get the coordinates we are at.
  MapTile* our_maptile = get_maptile();
  MapTile* thing_maptile = thing->get_maptile();
  if ((our_maptile == nullptr) || (thing_maptile == nullptr))
  {
    return false;
  }

  sf::Vector2i const& our_coords = our_maptile->get_coords();
  sf::Vector2i const& thing_coords = thing_maptile->get_coords();

  return adjacent(our_coords, thing_coords);
}

bool Thing::do_die()
{
  /// @todo Handle stuff like auto-activating life-saving items here.
  /// @todo Pass in the cause of death somehow.

  ActionResult result = perform_action_died();
  std::string message;

  switch (result)
  {
    case ActionResult::Success:
      if (this->is_player())
      {
        message = YOU + " die...";
        the_message_log.add(message);
      }
      else
      {
        bool living = get_property<bool>("living");
        if (living)
        {
          message = YOU_ARE + " killed!";
        }
        else
        {
          message = YOU_ARE + " destroyed!";
        }
        the_message_log.add(message);
      }

      // Set the property saying the entity is dead.
      set_property<bool>("dead", true);

      // Clear any pending actions.
      pImpl->pending_actions.clear();

      return true;
    case ActionResult::Failure:
    default:
      message = YOU + CV(" manage", " manages") + " to avoid dying.";
      the_message_log.add(message);
      return false;
  }
}

bool Thing::do_attack(Direction direction, unsigned int& action_time)
{
  /// @todo Update action time based on direction, speed, etc.

  std::string message;

  // First: check direction.
  if (direction == Direction::Self)
  {
    /// @todo Allow attacking yourself!
    message = YOU + " wisely" + CV(" refrain", " refrains") + " from pummelling " + YOURSELF + ".";
    the_message_log.add(message);
    return false;
  }

  ThingRef location = get_location();
  MapTile* current_tile = get_maptile();

  // Make sure we're not in limbo!
  if ((location == ThingManager::get_mu()) || (current_tile == nullptr))
  {
    message = YOU + " can't attack anything because " + YOU_DO + " not exist physically!";
    the_message_log.add(message);
    return false;
  }

  if (!IS_PLAYER)
  {
    message = YOU_TRY_TO("attack") + str(direction) + ".";
    the_message_log.add(message);
    message = "But ";
  }
  else
  {
    message = "";
  }

  // Make sure we CAN move.
  if (!get_intrinsic<bool>("can_attack", false))
  {
    message += YOU + CV(" aren't", " isn't") + " capable of attacking anything.";
    the_message_log.add(message);
    return false;
  }

  // Make sure we can move RIGHT NOW.
  if (!can_currently_move())
  {
    message += YOU + " can't move right now.";
    the_message_log.add(message);
    return false;
  }

  // Make sure we're not confined inside another thing.
  if (is_inside_another_thing())
  {
    /// @todo Attack the thing you are inside!
    message += YOU_ARE + " inside " + location->get_identifying_string(false) +
      " and " + ARE + " not going anywhere!";

    the_message_log.add(message);
    return false;
  }

  if (direction == Direction::Up)
  {
    /// @todo Write up/down attack code
    message = "Attacking the ceiling is not yet supported!";
    the_message_log.add(message);
    return false;
  }
  else if (direction == Direction::Down)
  {
    /// @todo Write up/down attack code
    message = "Attacking the floor is not yet supported!";
    the_message_log.add(message);
    return false;
  }
  else
  {
    // Figure out our target location.
    sf::Vector2i coords = current_tile->get_coords();
    sf::Vector2i offset = get_vector(direction);
    int x_new = coords.x + offset.x;
    int y_new = coords.y + offset.y;
    Map& current_map = GAME.get_map_factory().get(get_map_id());
    sf::Vector2i map_size = current_map.get_size();

    // Check boundaries.
    if ((x_new < 0) || (y_new < 0) ||
        (x_new >= map_size.x) || (y_new >= map_size.y))
    {
      message += YOU + " can't attack there; it is out of bounds!";
      the_message_log.add(message);
      return false;
    }

    auto& new_tile = current_map.get_tile(x_new, y_new);
    ThingRef new_floor = new_tile.get_tile_contents();

    // See if the tile to move into contains another creature.
    ThingRef creature = new_floor->get_inventory().get_entity();
    if (creature != ThingManager::get_mu())
    {
      return do_attack(creature, action_time);
    }
    else
    {
      /// @todo Deal with attacking other stuff, MapTiles, etc.
      message = "Attacking non-entity things is not yet supported!";
      the_message_log.add(message);
      return false;
    }
  } // end else if (other direction)
}

bool Thing::do_attack(ThingRef thing, unsigned int& action_time)
{
  std::string message;

  bool reachable = this->is_adjacent_to(thing);
  /// @todo deal with Entities in your Inventory -- WTF do you do THEN?

  if (reachable)
  {
    /// @todo Write attack code.
    std::string message = YOU_TRY + " to attack " + FOO + ", but are stopped by the programmer's procrastination!";
    the_message_log.add(message);
  }

  return false;
}

ActionResult Thing::can_mix(ThingRef thing1, ThingRef thing2, unsigned int& action_time)
{
  action_time = 1;

  // Check that they aren't both the same thing.
  if (thing1 == thing2)
  {
    return ActionResult::FailureCircularReference;
  }

  // Check that neither of them is us.
  if (thing1 == pImpl->ref || thing2 == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that both are within reach.
  if (!this->can_reach(thing1) || !this->can_reach(thing2))
  {
    return ActionResult::FailureThingOutOfReach;
  }

  // Check that both are liquid containers.
  if (!thing1->get_intrinsic<bool>("liquid_carrier") || !thing2->get_intrinsic<bool>("liquid_carrier"))
  {
    return ActionResult::FailureNotLiquidCarrier;
  }

  // Check that neither is empty.
  Inventory& inv1 = thing1->get_inventory();
  Inventory& inv2 = thing2->get_inventory();
  if (inv1.count() == 0 || inv2.count() == 0)
  {
    return ActionResult::FailureContainerIsEmpty;
  }

  /// @todo Anything else needed here?
  ///       You need some sort of limbs to mix substances, right?

  return ActionResult::Success;
}

bool Thing::do_mix(ThingRef thing1, ThingRef thing2, unsigned int& action_time)
{
  std::string message;

  ActionResult mix_try = this->can_mix(thing1, thing2, action_time);
  ThingRef liquid1, liquid2;

  switch (mix_try)
  {
    case ActionResult::Success:
      liquid1 = thing1->get_inventory().get(INVSLOT_ZERO);
      liquid2 = thing2->get_inventory().get(INVSLOT_ZERO);

      message = YOU + CV(" mix ", " mixes ") + LIQUID1 + " with " + LIQUID2 + ".";
      the_message_log.add(message);

      /// @todo IMPLEMENT ME
      //thing1->perform_action_mixed_with_by(thing2, pImpl->ref);
      return true;

    case ActionResult::FailureCircularReference:
      liquid1 = thing1->get_inventory().get(INVSLOT_ZERO);
      liquid2 = thing2->get_inventory().get(INVSLOT_ZERO);

      message = YOU_TRY_TO("mix") + "the contents of " + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "Those are both the same container!";
      the_message_log.add(message);
      break;

    case ActionResult::FailureSelfReference:
      message = YOU_TRY_TO("mix") + "the contents of " + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "But that makes absolutely no sense.";
      the_message_log.add(message);
      break;

    case ActionResult::FailureThingOutOfReach:
      message = YOU_TRY_TO("mix") + "the contents of " + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "But at least one of them is out of " + YOUR + " reach.";
      the_message_log.add(message);
      break;

    case ActionResult::FailureNotLiquidCarrier:
      message = YOU_TRY_TO("mix") + "the contents of " + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "But at least one of them doesn't hold liquid!";
      the_message_log.add(message);
      break;

    case ActionResult::FailureContainerIsEmpty:
      message = YOU_TRY_TO("mix") + "the contents of " + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "But at least one of them is empty!";
      the_message_log.add(message);
      break;

    case ActionResult::FailureActorCantPerform:
      message = YOU_TRY_TO("mix") + FOO1 + " and " + FOO2 + ".";
      the_message_log.add(message);
      message = "But, as a " + get_display_name() + "," + YOU_HAVE + " no way to mix anything.";
      the_message_log.add(message);
      break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", mix_try);
      break;
  }
  return false;
}

ActionResult Thing::can_put_into(ThingRef thing, ThingRef container,
                                 unsigned int& action_time)
{
  action_time = 1;

  // Check that the thing and container aren't the same thing.
  if (thing == container)
  {
    return ActionResult::FailureCircularReference;
  }

  // Check that the thing is not US!
  if (thing == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that the container is not US!
  if (container == pImpl->ref)
  {
    return ActionResult::FailureContainerCantBeSelf;
  }

  // Check that the container actually IS a container.
  if (container->get_intrinsic<int>("inventory_size") == 0)
  {
    return ActionResult::FailureTargetNotAContainer;
  }

  // Check that the thing's location isn't already the container.
  if (thing->get_location() == container)
  {
    return ActionResult::FailureAlreadyPresent;
  }

  // Check that the thing is within reach.
  if (!this->can_reach(thing))
  {
    return ActionResult::FailureThingOutOfReach;
  }

  // Check that the container is within reach.
  if (!this->can_reach(container))
  {
    return ActionResult::FailureContainerOutOfReach;
  }

  // Check that we're not wielding the item.
  if (this->is_wielding(thing))
  {
    return ActionResult::FailureItemWielded;
  }

  /// @todo Check that we're not wearing the item.
  if (this->has_equipped(thing))
  {
    return ActionResult::FailureItemEquipped;
  }

  // Finally, make sure the container can hold this thing.
  return container->can_contain(pImpl->ref);
}

bool Thing::do_put_into(ThingRef thing, ThingRef container,
                        unsigned int& action_time)
{
  std::string message;

  ActionResult put_try = this->can_put_into(thing, container, action_time);

  switch (put_try)
  {
    case ActionResult::Success:
    {
      if (thing->perform_action_put_into_by(container, pImpl->ref))
      {
        message = YOU + choose_verb(" place ", "places ") +
          thing->get_identifying_string() + " into " +
          container->get_identifying_string() + ".";
        the_message_log.add(message);
        if (!thing->move_into(container))
        {
          MAJOR_ERROR("Could not move Thing into Container");
        }
        else
        {
          return true;
        }
      }
    }
    break;

    case ActionResult::FailureSelfReference:
    {
      if (is_player())
      {
        /// @todo Possibly allow player to voluntarily enter a container?
        message = "I'm afraid you can't do that.  "
          "(At least, not in this version...)";
      }
      else
      {
        message = YOU_TRY + " to store " + YOURSELF +
          "into the " + thing->get_identifying_string() +
          ", which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to store self!?");
      }
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureContainerCantBeSelf:
    {
      if (is_player())
      {
        message = "Store something in yourself?  What do you think you are, a drug mule?";
      }
      else
      {
        message = YOU_TRY + " to store " + thing->get_identifying_string() +
          "into " + YOURSELF +
          ", which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to store into self!?");
      }
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureAlreadyPresent:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = thing->get_identifying_string() + " is already in " +
        container->get_identifying_string() + "!";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureTargetNotAContainer:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = container->get_identifying_string() + " is not a container!";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureThingOutOfReach:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = YOU + " cannot reach " + thing->get_identifying_string() + ".";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureContainerOutOfReach:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = YOU + " cannot reach " + container->get_identifying_string() + ".";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureCircularReference:
    {
      if (is_player())
      {
        message = "That would be an interesting topological exercise.";
      }
      else
      {
        message = YOU_TRY + " to store " + thing->get_identifying_string() +
          "in itself, which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to store a container in itself!?");
      }
    }
    break;

    case ActionResult::FailureItemEquipped:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = YOU + " cannot store something that is currently being worn.";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureItemWielded:
    {
      message = YOU_TRY + " to store " + thing->get_identifying_string() + " in " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      /// @todo Perhaps automatically try to unwield the item before dropping?
      message = YOU + "cannot store something that is currently being wielded.";
      the_message_log.add(message);
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", put_try);
      break;
  }

  return false;
}

ActionResult Thing::can_take_out(ThingRef thing, unsigned int& action_time)
{
  action_time = 1;

  // Check that the thing isn't US!
  if (thing != pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that the container is not a MapTile or Entity.
  if (!thing->is_inside_another_thing())
  {
    return ActionResult::FailureNotInsideContainer;
  }

  ThingRef container = thing->get_location();

  // Check that the container is within reach.
  if (!can_reach(container))
  {
    return ActionResult::FailureContainerOutOfReach;
  }

  return ActionResult::Success;
}

bool Thing::do_take_out(ThingRef thing, unsigned int& action_time)
{
  std::string message;

  ActionResult takeout_try = this->can_take_out(thing, action_time);

  ThingRef container = thing->get_location();

  ThingRef new_location = container->get_location();

  switch (takeout_try)
  {
    case ActionResult::Success:
    {
      if (thing->perform_action_taken_out_by(pImpl->ref))
      {
        if (!thing->move_into(new_location))
        {
          MAJOR_ERROR("Could not move Thing out of Container");
        }
        else
        {
          message = YOU + choose_verb(" remove ", "removes ") +
            thing->get_identifying_string() + " from " +
            container->get_identifying_string() + ".";
          the_message_log.add(message);
        }
      }
    }
    break;

    case ActionResult::FailureSelfReference:
    {
      if (is_player())
      {
        /// @todo Maybe allow player to voluntarily exit a container?
        message = "I'm afraid you can't do that.  "
          "(At least, not in this version...)";
      }
      else
      {
        message = YOU_TRY + " to take " + YOURSELF +
          "out, which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to take self out!?");
      }
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureNotInsideContainer:
    {
      message = YOU_TRY + " to remove " + thing->get_identifying_string() +
        " from its container.";
      the_message_log.add(message);

      message = "But " + thing->get_identifying_string() + " is not inside a container!";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureContainerOutOfReach:
    {
      message = YOU_TRY + " to remove " + thing->get_identifying_string() + " from " +
        container->get_identifying_string() + ".";
      the_message_log.add(message);

      message = YOU + " cannot reach " + container->get_identifying_string() + ".";
      the_message_log.add(message);
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", takeout_try);
      break;
  }

  return false;
}

ActionResult Thing::can_throw(ThingRef thing, unsigned int& action_time)
{
  action_time = 1;

  // Check that it isn't US!
  if (thing == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that it's in our inventory.
  if (!this->get_inventory().contains(thing))
  {
    return ActionResult::FailureNotPresent;
  }

  /// @todo Check that we're not wearing the item.

  return ActionResult::Success;
}

bool Thing::do_throw(ThingRef thing, Direction& direction, unsigned int& action_time)
{
  std::string message;
  ActionResult throw_try = this->can_throw(thing, action_time);

  switch (throw_try)
  {
    case ActionResult::Success:
    {
      ThingRef new_location = get_location();
      if (thing->is_movable_by(pImpl->ref))
      {
        if (thing->perform_action_thrown_by(pImpl->ref, direction))
        {
          if (thing->move_into(new_location))
          {
            message = YOU + choose_verb(" throw ", " throws ") +
              thing->get_identifying_string();
            the_message_log.add(message);

            /// @todo When throwing, set Thing's direction and velocity
            return true;
          }
          else
          {
            MAJOR_ERROR("Could not move Thing even though "
                        "is_movable returned Success");
          }
        }
      }
      else
      {
        /// @todo Tried to throw something immovable? That doesn't make sense.
      }
    }
    break;

    case ActionResult::FailureSelfReference:
    {
      if (is_player())
      {
        message = "Throw yourself?  Throw yourself what, a party?";
      }
      else
      {
        message = YOU_TRY + " to throw " + YOURSELF +
          ", which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to throw self!?");
      }
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureItemEquipped:
    {
      message = YOU + " cannot throw something " + YOU_ARE + "wearing.";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureNotPresent:
    {
      message = YOU_TRY + " to throw " + thing->get_identifying_string() + ".";
      the_message_log.add(message);

      message = "But " + thing->get_identifying_string() +
        thing->choose_verb(" are", " is") +
        " not actually in " + YOUR +
        " inventory!";
      the_message_log.add(message);
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", throw_try);
      break;
  }

  return false;
}

ActionResult Thing::can_use(ThingRef thing, unsigned int& action_time)
{
  action_time = 1;

  // Check that it isn't US!
  if (thing == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that the thing is within reach.
  if (!this->can_reach(thing))
  {
    return ActionResult::FailureThingOutOfReach;
  }

  return ActionResult::Success;
}

bool Thing::do_use(ThingRef thing, unsigned int& action_time)
{
  std::string message;

  ActionResult use_try = this->can_use(thing, action_time);

  switch (use_try)
  {
    case ActionResult::Success:
      if (thing->is_usable_by(pImpl->ref))
      {
        if (thing->perform_action_used_by(pImpl->ref))
        {
          return true;
        }
      }
      else
      {
        message = YOU_TRY + " to use " + thing->get_identifying_string() + ".";
        the_message_log.add(message);

        message = YOU + " can't use that!";
        the_message_log.add(message);
      }
      break;

    case ActionResult::FailureSelfReference:
      message = YOU_TRY + " to use " + thing->get_identifying_string() + ".";
      the_message_log.add(message);

      if (GAME.get_player() == pImpl->ref)
      {
        message = YOU_ARE + " already using " + YOURSELF + " to the best of " + YOUR + " ability.";
        the_message_log.add(message);
      }
      else
      {
        message = "That seriously shouldn't happen!";
        the_message_log.add(message);

        MINOR_ERROR("Non-player Entity tried to use self!?");
      }
      break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", use_try);
      break;
  }
  return false;
}

ActionResult Thing::can_deequip(ThingRef thing, unsigned int& action_time)
{
  action_time = 1;

  // Check that it isn't US!
  if (thing == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that it's already being worn.
  if (!this->has_equipped(thing))
  {
    return ActionResult::FailureItemNotEquipped;
  }

  /// @todo Finish Thing::can_deequip code
  return ActionResult::Success;
}

bool Thing::do_deequip(ThingRef thing, unsigned int& action_time)
{
  std::string message;
  ActionResult deequip_try = this->can_deequip(thing, action_time);
  std::string thing_name = thing->get_identifying_string();

  message = YOU_TRY + " to take off " + thing_name;
  the_message_log.add(message);

  switch (deequip_try)
  {
    case ActionResult::Success:
    {
      // Get the body part this item is equipped on.
      WearLocation location;
      this->has_equipped(thing, location);

      if (thing->perform_action_deequipped_by(pImpl->ref, location))
      {
        pImpl->do_deequip(thing);

        std::string wear_desc = get_bodypart_description(location.part, location.number);
        message = YOU_ARE + " no longer wearing " + thing_name +
          " on " + YOUR + " " + wear_desc + ".";
        the_message_log.add(message);
        return true;
      }
    }
    break;

    case ActionResult::FailureItemNotEquipped:
    {
      message = YOU_ARE + " not wearing " + thing_name + ".";
      the_message_log.add(message);
      return true;
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", deequip_try);
      break;
  }

  return false;
}

ActionResult Thing::can_equip(ThingRef thing, unsigned int& action_time)
{
  action_time = 1;

  // Check that it isn't US!
  if (thing == pImpl->ref)
  {
    return ActionResult::FailureSelfReference;
  }

  // Check that it's in our inventory.
  if (!this->get_inventory().contains(thing))
  {
    return ActionResult::FailureThingOutOfReach;
  }

  std::string message;
  BodyPart part = thing->is_equippable_on();

  if (part == BodyPart::Count)
  {
    return ActionResult::FailureItemNotEquippable;
  }
  else
  {
    /// @todo Check that entity has free body part(s) to equip item on.
  }

  /// @todo Finish Thing::can_equip code
  return ActionResult::Success;
}

bool Thing::do_equip(ThingRef thing, unsigned int& action_time)
{
  std::string message;

  ActionResult equip_try = this->can_equip(thing, action_time);
  std::string thing_name = thing->get_identifying_string();

  switch (equip_try)
  {
    case ActionResult::Success:
    {
      WearLocation location;

      if (thing->perform_action_equipped_by(pImpl->ref, location))
      {
        pImpl->do_equip(thing, location);
        std::string wear_desc = get_bodypart_description(location.part,
                                                         location.number);
        message = YOU_ARE + " now wearing " + thing_name +
          " on " + YOUR + " " + wear_desc + ".";
        the_message_log.add(message);
        return true;
      }
    }
    break;

    case ActionResult::FailureSelfReference:
      if (is_player())
      {
        message = "To equip yourself, choose what you want to equip first.";
      }
      else
      {
        message = YOU_TRY + " to equip " + YOURSELF +
          ", which seriously shouldn't happen.";
        MINOR_ERROR("Non-player Entity tried to equip self!?");
      }
      the_message_log.add(message);
      break;

    case ActionResult::FailureThingOutOfReach:
    {
      message = YOU_TRY + " to equip " + thing_name + ".";
      the_message_log.add(message);

      message = thing_name + " is not in " + YOUR + " inventory.";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureItemNotEquippable:
    {
      message = YOU_TRY + " to equip " + thing_name + ".";
      the_message_log.add(message);

      message = thing_name + " is not an equippable item.";
      the_message_log.add(message);
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", equip_try);
      break;
  }

  return false;
}

ActionResult Thing::can_wield(ThingRef thing, unsigned int hand, unsigned int& action_time)
{
  action_time = 1;

  // If it is us, it means to unwield whatever is wielded.
  if (thing == pImpl->ref)
  {
    return ActionResult::SuccessSelfReference;
  }

  // Check that it's in our inventory.
  if (!this->get_inventory().contains(thing))
  {
    return ActionResult::FailureThingOutOfReach;
  }

  // Check that we have hands capable of wielding anything.
  if (this->get_bodypart_number(BodyPart::Hand) == 0)
  {
    return ActionResult::FailureNotEnoughHands;
  }

  return ActionResult::Success;
}

bool Thing::do_wield(ThingRef thing, unsigned int hand, unsigned int& action_time)
{
  std::string message;
  std::string bodypart_desc =
    this->get_bodypart_description(BodyPart::Hand, hand);

  ThingRef currently_wielded = pImpl->wielding_in(hand);

  std::string thing_name = (thing != ThingManager::get_mu()) ? thing->get_identifying_string() : "nothing";

  bool was_wielding = false;

  // First, check if we're already wielding something.
  if (currently_wielded != ThingManager::get_mu())
  {
    // Now, check if the thing we're already wielding is THIS thing.
    if (currently_wielded == thing)
    {
      message = YOU_ARE + " already wielding " + thing_name + " with " +
        YOUR + " " + bodypart_desc + ".";
      the_message_log.add(message);
      return true;
    }
    else
    {
      // Try to unwield the old item.
      if (currently_wielded->perform_action_unwielded_by(pImpl->ref))
      {
        pImpl->do_unwield(currently_wielded);
        was_wielding = true;
      }
      else
      {
        return false;
      }
    }
  }

  // If we HAVE a new item, try to wield it.
  ActionResult wield_try = (thing != ThingManager::get_mu())
    ? this->can_wield(thing, hand, action_time)
    : ActionResult::SuccessSelfReference;

  switch (wield_try)
  {
    case ActionResult::Success:
    case ActionResult::SuccessSwapHands:
    {
      if (thing->perform_action_wielded_by(pImpl->ref))
      {
        pImpl->do_wield(thing, hand);
        message = YOU_ARE + " now wielding " + thing_name +
          " with " + YOUR + " " + bodypart_desc + ".";
        the_message_log.add(message);
        return true;
      }
    }
    break;

    case ActionResult::SuccessSelfReference:
    {
      if (was_wielding)
      {
        message = YOU_ARE + " no longer wielding any weapons with " +
          YOUR + " " +
          this->get_bodypart_description(BodyPart::Hand, hand) + ".";
        the_message_log.add(message);
      }
      else
      {
        message = YOU_WERE + " not wielding any weapon to begin with.";
        the_message_log.add(message);
      }
      return true;
    }
    break;

    case ActionResult::FailureThingOutOfReach:
    {
      message = YOU_TRY + " to wield " + thing_name + ".";
      the_message_log.add(message);

      message = thing_name + " is not in " + YOUR + " inventory.";
      the_message_log.add(message);
    }
    break;

    case ActionResult::FailureNotEnoughHands:
    {
      message = YOU_TRY + " to wield " + thing_name;
      the_message_log.add(message);

      message = YOU + choose_verb(" don't", " doesn't") +
        " have enough free " +
        this->get_bodypart_plural(BodyPart::Hand) + ".";
      the_message_log.add(message);
    }
    break;

    default:
      MINOR_ERROR("Unknown ActionResult %d", wield_try);
      break;
  }
  return false;
}

bool Thing::can_currently_see()
{
  return get_intrinsic<bool>("can_see", false) && (get_property<int>("counters.blind", 0) == 0);
}

bool Thing::can_currently_move()
{
  return get_intrinsic<bool>("can_move", false) && (get_property<int>("counters.paralyzed", 0) == 0);
}

void Thing::set_gender(Gender gender)
{
  pImpl->gender = gender;
}

Gender Thing::get_gender() const
{
  return pImpl->gender;
}

Gender Thing::get_gender_or_you() const
{
  if (is_player())
  {
    return Gender::SecondPerson;
  }
  else
  {
    return pImpl->gender;
  }
}

/// Get the number of a particular body part the Entity has.
unsigned int Thing::get_bodypart_number(BodyPart part) const
{
  switch (part)
  {
    case BodyPart::Body:  return get_intrinsic<int>("bodypart_body_count");
    case BodyPart::Skin:  return get_intrinsic<int>("bodypart_skin_count");
    case BodyPart::Head:  return get_intrinsic<int>("bodypart_head_count");
    case BodyPart::Ear:   return get_intrinsic<int>("bodypart_ear_count");
    case BodyPart::Eye:   return get_intrinsic<int>("bodypart_eye_count");
    case BodyPart::Nose:  return get_intrinsic<int>("bodypart_nose_count");
    case BodyPart::Mouth: return get_intrinsic<int>("bodypart_mouth_count");
    case BodyPart::Neck:  return get_intrinsic<int>("bodypart_neck_count");
    case BodyPart::Chest: return get_intrinsic<int>("bodypart_chest_count");
    case BodyPart::Arm:   return get_intrinsic<int>("bodypart_arm_count");
    case BodyPart::Hand:  return get_intrinsic<int>("bodypart_hand_count");
    case BodyPart::Leg:   return get_intrinsic<int>("bodypart_leg_count");
    case BodyPart::Foot:  return get_intrinsic<int>("bodypart_foot_count");
    case BodyPart::Wing:  return get_intrinsic<int>("bodypart_wing_count");
    case BodyPart::Tail:  return get_intrinsic<int>("bodypart_tail_count");
    default: return 0;
  }
}

/// Get the appropriate body part name for the Entity.
std::string Thing::get_bodypart_name(BodyPart part) const
{
  switch (part)
  {
    case BodyPart::Body:  return get_intrinsic<std::string>("bodypart_body_name");
    case BodyPart::Skin:  return get_intrinsic<std::string>("bodypart_skin_name");
    case BodyPart::Head:  return get_intrinsic<std::string>("bodypart_head_name");
    case BodyPart::Ear:   return get_intrinsic<std::string>("bodypart_ear_name");
    case BodyPart::Eye:   return get_intrinsic<std::string>("bodypart_eye_name");
    case BodyPart::Nose:  return get_intrinsic<std::string>("bodypart_nose_name");
    case BodyPart::Mouth: return get_intrinsic<std::string>("bodypart_mouth_name");
    case BodyPart::Neck:  return get_intrinsic<std::string>("bodypart_neck_name");
    case BodyPart::Chest: return get_intrinsic<std::string>("bodypart_chest_name");
    case BodyPart::Arm:   return get_intrinsic<std::string>("bodypart_arm_name");
    case BodyPart::Hand:  return get_intrinsic<std::string>("bodypart_hand_name");
    case BodyPart::Leg:   return get_intrinsic<std::string>("bodypart_leg_name");
    case BodyPart::Foot:  return get_intrinsic<std::string>("bodypart_foot_name");
    case BodyPart::Wing:  return get_intrinsic<std::string>("bodypart_wing_name");
    case BodyPart::Tail:  return get_intrinsic<std::string>("bodypart_tail_name");
    default: return "squeedlyspooch (unknown BodyPart)";
  }
}

/// Get the appropriate body part plural for the Entity.
std::string Thing::get_bodypart_plural(BodyPart part) const
{
  switch (part)
  {
    case BodyPart::Body:  return get_intrinsic<std::string>("bodypart_body_plural", get_bodypart_name(BodyPart::Body) + "s");
    case BodyPart::Skin:  return get_intrinsic<std::string>("bodypart_skin_plural", get_bodypart_name(BodyPart::Skin) + "s");
    case BodyPart::Head:  return get_intrinsic<std::string>("bodypart_head_plural", get_bodypart_name(BodyPart::Head) + "s");
    case BodyPart::Ear:   return get_intrinsic<std::string>("bodypart_ear_plural", get_bodypart_name(BodyPart::Ear) + "s");
    case BodyPart::Eye:   return get_intrinsic<std::string>("bodypart_eye_plural", get_bodypart_name(BodyPart::Eye) + "s");
    case BodyPart::Nose:  return get_intrinsic<std::string>("bodypart_nose_plural", get_bodypart_name(BodyPart::Nose) + "s");
    case BodyPart::Mouth: return get_intrinsic<std::string>("bodypart_mouth_plural", get_bodypart_name(BodyPart::Mouth) + "s");
    case BodyPart::Neck:  return get_intrinsic<std::string>("bodypart_neck_plural", get_bodypart_name(BodyPart::Neck) + "s");
    case BodyPart::Chest: return get_intrinsic<std::string>("bodypart_chest_plural", get_bodypart_name(BodyPart::Chest) + "s");
    case BodyPart::Arm:   return get_intrinsic<std::string>("bodypart_arm_plural", get_bodypart_name(BodyPart::Arm) + "s");
    case BodyPart::Hand:  return get_intrinsic<std::string>("bodypart_hand_plural", get_bodypart_name(BodyPart::Hand) + "s");
    case BodyPart::Leg:   return get_intrinsic<std::string>("bodypart_leg_plural", get_bodypart_name(BodyPart::Leg) + "s");
    case BodyPart::Foot:  return get_intrinsic<std::string>("bodypart_foot_plural", get_bodypart_name(BodyPart::Foot) + "s");
    case BodyPart::Wing:  return get_intrinsic<std::string>("bodypart_wing_plural", get_bodypart_name(BodyPart::Wing) + "s");
    case BodyPart::Tail:  return get_intrinsic<std::string>("bodypart_tail_plural", get_bodypart_name(BodyPart::Tail) + "s");
    default: return "squeedlyspooches (unknown BodyParts)";
  }
}

bool Thing::is_player() const
{
  return (GAME.get_player() == pImpl->ref);
}

std::string const& Thing::get_type() const
{
  return pImpl->metadata.get_type();
}

std::string const& Thing::get_parent_type() const
{
  return pImpl->metadata.get_intrinsic<std::string>("parent");
}

unsigned int Thing::get_quantity()
{
  return get_property<unsigned int>("quantity", 1);
}

void Thing::set_quantity(unsigned int quantity)
{
  set_property<unsigned int>("quantity", quantity);
}

ThingRef Thing::get_ref() const
{
  return pImpl->ref;
}

ThingRef Thing::get_location() const
{
  return pImpl->location;
}

bool Thing::can_see(ThingRef thing)
{
  // Make sure we are able to see at all.
  if (!can_currently_see())
  {
    return false;
  }

  // Are we on a map?  Bail out if we aren't.
  MapId entity_map_id = this->get_map_id();
  MapId thing_map_id = thing->get_map_id();

  if ((entity_map_id == MapFactory::null_map_id) ||
      (thing_map_id == MapFactory::null_map_id) ||
      (entity_map_id != thing_map_id))
  {
    return false;
  }

  auto thing_location = thing->get_maptile();
  if (thing_location == nullptr)
  {
    return false;
  }

  sf::Vector2i thing_coords = thing_location->get_coords();

  return can_see(thing_coords.x, thing_coords.y);
}

bool Thing::can_see(sf::Vector2i coords)
{
  return this->can_see(coords.x, coords.y);
}

bool Thing::can_see(int xTile, int yTile)
{
  // Make sure we are able to see at all.
  if (!can_currently_see())
  {
    return false;
  }

  MapId map_id = get_map_id();

  // Are we on a map?  Bail out if we aren't.
  if (map_id == MapFactory::null_map_id)
  {
    return false;
  }

  // If the coordinates are where we are, then yes, we can indeed see the tile.
  auto tile = get_maptile();
  if (tile == nullptr)
  {
    return false;
  }

  sf::Vector2i tile_coords = tile->get_coords();

  if ((tile_coords.x == xTile) && (tile_coords.y == yTile))
  {
    return true;
  }

  Map& game_map = GAME.get_map_factory().get(map_id);

  if (can_currently_see())
  {
    // Return seen data.
    return pImpl->tiles_currently_seen[game_map.get_index(xTile, yTile)];
  }
  else
  {
    return false;
  }
}

void Thing::find_seen_tiles()
{
  //sf::Clock elapsed;

  //elapsed.restart();

  // Are we on a map?  Bail out if we aren't.
  ThingRef location = get_location();
  if (location == ThingManager::get_mu())
  {
    return;
  }

  // Clear the "tile seen" bitset.
  pImpl->tiles_currently_seen.reset();

  // Hang on, can we actually see?
  // If not, bail out.
  if (can_currently_see() == false)
  {
    return;
  }

  for (int n = 1; n <= 8; ++n)
  {
    do_recursive_visibility(n);
  }

  //TRACE("find_seen_tiles took %d ms", elapsed.getElapsedTime().asMilliseconds());
}

std::string Thing::get_memory_at(int x, int y) const
{
  if (this->get_map_id() == MapFactory::null_map_id)
  {
    return "???";
  }

  Map& game_map = GAME.get_map_factory().get(this->get_map_id());
  return pImpl->map_memory[game_map.get_index(x, y)];
}

std::string Thing::get_memory_at(sf::Vector2i coords) const
{
  return this->get_memory_at(coords.x, coords.y);
}

void Thing::add_memory_vertices_to(sf::VertexArray& vertices,
                                   int x, int y)
{
  MapId map_id = this->get_map_id();
  if (map_id == MapFactory::null_map_id)
  {
    return;
  }
  Map& game_map = GAME.get_map_factory().get(map_id);

  static sf::Vertex new_vertex;
  float ts = Settings.get<float>("map_tile_size");
  float ts2 = ts * 0.5f;

  sf::Vector2f location(x * ts, y * ts);
  sf::Vector2f vSW(location.x - ts2, location.y + ts2);
  sf::Vector2f vSE(location.x + ts2, location.y + ts2);
  sf::Vector2f vNW(location.x - ts2, location.y - ts2);
  sf::Vector2f vNE(location.x + ts2, location.y - ts2);

  std::string tile_type = pImpl->map_memory[game_map.get_index(x, y)];
  if (tile_type == "") tile_type = "MTUnknown";
  Metadata* tile_metadata = &(MDC::get_collection("maptile").get(tile_type));

  /// @todo Call a script to handle selecting a tile other than the one
  ///       in the upper-left corner.
  sf::Vector2u tile_coords = tile_metadata->get_tile_coords();

  TileSheet::add_quad(vertices,
                      tile_coords, sf::Color::White,
                      vNW, vNE,
                      vSW, vSE);
}

bool Thing::move_into(ThingRef new_location)
{
  MapId old_map_id = this->get_map_id();
  ThingRef old_location = pImpl->location;

  if (new_location == old_location)
  {
    // We're already there!
    return true;
  }

  ActionResult can_contain = new_location->can_contain(pImpl->ref);

  switch (can_contain)
  {
    case ActionResult::Success:
      if (new_location->get_inventory().add(pImpl->ref))
      {
        // Try to lock our old location.
        if (pImpl->location != ThingManager::get_mu())
        {
          pImpl->location->get_inventory().remove(pImpl->ref);
        }

        // Set the location to the new location.
        pImpl->location = new_location;

        MapId new_map_id = this->get_map_id();
        if (old_map_id != new_map_id)
        {
          if (old_map_id != MapFactory::null_map_id)
          {
            /// @todo Save old map memory.
          }
          pImpl->map_memory.clear();
          pImpl->tiles_currently_seen.clear();
          if (new_map_id != MapFactory::null_map_id)
          {
            Map& new_map = GAME.get_map_factory().get(new_map_id);
            sf::Vector2i new_map_size = new_map.get_size();
            pImpl->map_memory.resize(new_map_size.x * new_map_size.y);
            pImpl->tiles_currently_seen.resize(new_map_size.x * new_map_size.y);
            /// @todo Load new map memory if it exists somewhere.
          }
        }
        this->find_seen_tiles();
        return true;
      } // end if (add to new inventory was successful)

      break;
    default:
      break;
  } // end switch (ActionResult)

  return false;
}

Inventory& Thing::get_inventory()
{
  return pImpl->inventory;
}

bool Thing::is_inside_another_thing() const
{
  ThingRef location = pImpl->location;
  if (location == ThingManager::get_mu())
  {
    // Thing is a part of the MapTile such as the floor.
    return false;
  }

  ThingRef location2 = location->get_location();
  if (location2 == ThingManager::get_mu())
  {
    // Thing is directly on the floor.
    return false;
  }
  return true;
}

MapTile* Thing::get_maptile() const
{
  ThingRef location = pImpl->location;

  if (location == ThingManager::get_mu())
  {
    return _get_maptile();
  }
  else
  {
    return location->get_maptile();
  }
}

MapId Thing::get_map_id() const
{
  ThingRef location = pImpl->location;

  if (location == ThingManager::get_mu())
  {
    MapTile* maptile = _get_maptile();
    if (maptile != nullptr)
    {
      return maptile->get_map_id();
    }
    else
    {
      return static_cast<MapId>(0);
    }
  }
  else
  {
    return location->get_map_id();
  }
}

std::string Thing::get_display_name() const
{
  /// @todo Implement adding adjectives.
  return pImpl->metadata.get_intrinsic<std::string>("name");
}

std::string Thing::get_display_plural() const
{
  return pImpl->metadata.get_intrinsic<std::string>("plural");
}

std::string Thing::get_proper_name()
{
  return get_property<std::string>("proper_name");
}

void Thing::set_proper_name(std::string name)
{
  set_property<std::string>("proper_name", name);
}

std::string Thing::get_identifying_string_without_possessives(bool definite)
{
  ThingRef location = this->get_location();
  unsigned int quantity = this->get_quantity();

  std::string name;

  std::string article;
  std::string adjectives;
  std::string noun;
  std::string suffix;

  // If the thing is YOU, use YOU.
  if (is_player())
  {
    if (get_property<int>("hp") > 0)
    {
      return "you";
    }
    else
    {
      return "your corpse";
    }
  }

  if (quantity == 1)
  {
    noun = get_display_name();

    if (definite)
    {
      article = "the ";
    }
    else
    {
      article = getIndefArt(noun) + " ";
    }

    if (get_proper_name().empty() == false)
    {
      suffix = " named " + get_proper_name();
    }
  }
  else
  {
    noun = get_display_plural();

    if (definite)
    {
      article = "the ";
    }
    article += boost::lexical_cast<std::string>(get_quantity()) + " ";
  }

  if (get_intrinsic<bool>("is_entity") && get_property<int>("hp") <= 0)
  {
    adjectives += "dead ";
  }

  name = article + adjectives + noun + suffix;

  return name;
}

std::string Thing::get_you_or_identifying_string(bool definite)
{
  // If the thing is YOU, use YOU.
  if (is_player())
  {
    if (get_property<int>("hp") > 0)
    {
      return "you";
    }
    else
    {
      return "your corpse";
    }
  }

  return get_identifying_string(definite);
}

std::string Thing::get_self_or_identifying_string(ThingRef other, bool definite)
{
  if (other == get_ref())
  {
    return get_reflexive_pronoun();
  }

  return get_identifying_string(definite);
}

std::string Thing::get_identifying_string(bool definite)
{
  ThingRef location = this->get_location();
  unsigned int quantity = this->get_quantity();

  std::string name;

  bool owned;

  std::string article;
  std::string adjectives;
  std::string noun;
  std::string suffix;

  owned = location->get_intrinsic<bool>("is_entity");

  if (quantity == 1)
  {
    noun = get_display_name();

    if (owned)
    {
      article = location->get_possessive() + " ";
    }
    else
    {
      if (definite)
      {
        article = "the ";
      }
      else
      {
        article = getIndefArt(noun) + " ";
      }
    }

    if (get_proper_name().empty() == false)
    {
      suffix = " named " + get_proper_name();
    }
  }
  else
  {
    noun = get_display_plural();

    if (owned)
    {
      article = location->get_possessive() + " ";
    }
    else
    {
      if (definite)
      {
        article = "the ";
      }

      article += boost::lexical_cast<std::string>(get_quantity()) + " ";
    }
  }

  if (get_intrinsic<bool>("is_entity") && get_property<int>("hp") <= 0)
  {
    adjectives += "dead ";
  }

  name = article + adjectives + noun + suffix;

  return name;
}

std::string const& Thing::choose_verb(std::string const& verb12,
                                      std::string const& verb3)
{
  if ((GAME.get_player() == pImpl->ref) || (get_property<unsigned int>("quantity") > 1))
  {
    return verb12;
  }
  else
  {
    return verb3;
  }
}

int Thing::get_mass()
{
  return get_intrinsic<int>("physical_mass") * get_property<unsigned int>("quantity");
}

std::string const& Thing::get_subject_pronoun() const
{
  return getSubjPro(get_gender_or_you());
}

std::string const& Thing::get_object_pronoun() const
{
  return getObjPro(get_gender_or_you());
}

std::string const& Thing::get_reflexive_pronoun() const
{
  return getRefPro(get_gender_or_you());
}

std::string const& Thing::get_possessive_adjective() const
{
  return getPossAdj(get_gender_or_you());
}

std::string const& Thing::get_possessive_pronoun() const
{
  return getPossPro(get_gender_or_you());
}

std::string Thing::get_possessive()
{
  static std::string const your = std::string("your");

  if (GAME.get_player() == pImpl->ref)
  {
    return your;
  }
  else
  {
    return get_identifying_string_without_possessives(true) + "'s";
  }
}

sf::Vector2u Thing::get_tile_sheet_coords(int frame)
{
  /// Get tile coordinates on the sheet.
  sf::Vector2u start_coords = pImpl->metadata.get_tile_coords();

  /// Call the Lua function to get the offset (tile to choose).
  sf::Vector2u offset = call_lua_function_v2u("get_tile_offset", { frame });

  /// Add them to get the resulting coordinates.
  sf::Vector2u tile_coords = start_coords + offset;

  return tile_coords;
}

void Thing::add_floor_vertices_to(sf::VertexArray& vertices,
                                  bool use_lighting,
                                  int frame)
{
  sf::Vertex new_vertex;
  float ts = Settings.get<float>("map_tile_size");
  float ts2 = ts * 0.5f;

  MapTile* root_tile = this->get_maptile();
  if (!root_tile)
  {
    // Item's root location isn't a MapTile, so it can't be rendered.
    return;
  }

  sf::Vector2i const& coords = root_tile->get_coords();

  sf::Color thing_color;
  if (use_lighting)
  {
    thing_color = root_tile->get_light_level();
  }
  else
  {
    thing_color = sf::Color::White;
  }

  sf::Vector2f location(coords.x * ts, coords.y * ts);
  sf::Vector2f vSW(location.x - ts2, location.y + ts2);
  sf::Vector2f vSE(location.x + ts2, location.y + ts2);
  sf::Vector2f vNW(location.x - ts2, location.y - ts2);
  sf::Vector2f vNE(location.x + ts2, location.y - ts2);
  sf::Vector2u tile_coords = this->get_tile_sheet_coords(frame);

  TileSheet::add_quad(vertices,
                      tile_coords, thing_color,
                      vNW, vNE,
                      vSW, vSE);
}

void Thing::draw_to(sf::RenderTarget& target,
                    sf::Vector2f target_coords,
                    unsigned int target_size,
                    bool use_lighting,
                    int frame)
{
  MapTile* root_tile = this->get_maptile();

  if (!root_tile)
  {
    // Item's root location isn't a MapTile, so it can't be rendered.
    return;
  }

  sf::RectangleShape rectangle;
  sf::IntRect texture_coords;

  if (target_size == 0)
  {
    target_size = Settings.get<unsigned int>("map_tile_size");
  }

  unsigned int tile_size = Settings.get<unsigned int>("map_tile_size");

  sf::Vector2u tile_coords = this->get_tile_sheet_coords(frame);
  texture_coords.left = tile_coords.x * tile_size;
  texture_coords.top = tile_coords.y * tile_size;
  texture_coords.width = tile_size;
  texture_coords.height = tile_size;

  sf::Color thing_color;
  if (use_lighting)
  {
    thing_color = root_tile->get_light_level();
  }
  else
  {
    thing_color = sf::Color::White;
  }

  rectangle.setPosition(target_coords);
  rectangle.setSize(sf::Vector2f(static_cast<float>(target_size),
                                 static_cast<float>(target_size)));
  rectangle.setTexture(&(TS.getTexture()));
  rectangle.setTextureRect(texture_coords);
  rectangle.setFillColor(thing_color);

  target.draw(rectangle);
}

bool Thing::is_opaque() const
{
  return
    (get_intrinsic<int>("opacity_red") >= 255) &&
    (get_intrinsic<int>("opacity_green") >= 255) &&
    (get_intrinsic<int>("opacity_blue") >= 255);
}

void Thing::light_up_surroundings()
{
  if (get_intrinsic<int>("inventory_size") != 0)
  {
    /// @todo Figure out how we want to handle light sources.
    ///       If we want to be more accurate, the light should only
    ///       be able to be seen when a character is wielding or has
    ///       equipped it. If we want to be easier, the light should
    ///       shine simply if it's in the player's inventory.

    //if (!is_opaque() || is_wielding(light) || has_equipped(light))
    if (!is_opaque() || get_intrinsic<bool>("is_entity"))
    {
      auto& things = get_inventory().get_things();
      for (auto& thing_pair : things)
      {
        ThingRef thing = thing_pair.second;
        thing->light_up_surroundings();
      }
    }
  }

  ThingRef location = get_location();

  // Use visitor pattern.
  if ((location != ThingManager::get_mu()) && this->get_property<bool>("light_lit"))
  {
    location->be_lit_by(this->get_ref());
  }
}

void Thing::be_lit_by(ThingRef light)
{
  call_lua_function("on_lit_by", { light });

  if (get_location() == ThingManager::get_mu())
  {
    GAME.get_map_factory().get(get_map_id()).add_light(light);
  }

  /// @todo Figure out how we want to handle light sources.
  ///       If we want to be more accurate, the light should only
  ///       be able to be seen when a character is wielding or has
  ///       equipped it. If we want to be easier, the light should
  ///       shine simply if it's in the player's inventory.

  //if (!is_opaque() || is_wielding(light) || has_equipped(light))
  if (!is_opaque() || get_intrinsic<bool>("is_entity"))
  {
    ThingRef location = get_location();
    if (location != ThingManager::get_mu())
    {
      location->be_lit_by(light);
    }
  }
}

void Thing::spill()
{
  Inventory& inventory = get_inventory();
  ThingMap const& things = inventory.get_things();
  std::string message;
  bool success = false;

  // Step through all contents of this Thing.
  for (ThingPair thing_pair : things)
  {
    ThingRef thing = thing_pair.second;
    if (pImpl->location != ThingManager::get_mu())
    {
      ActionResult can_contain = pImpl->location->can_contain(thing);

      switch (can_contain)
      {
        case ActionResult::Success:

          // Try to move this into the Thing's location.
          success = thing->move_into(pImpl->location);
          if (success)
          {
            auto container_string = this->get_identifying_string();
            auto thing_string = thing->get_identifying_string();
            message = thing_string + CV(" tumble", " tumbles") + " out of " + container_string + ".";
            the_message_log.add(message);
          }
          else
          {
            // We couldn't move it, so just destroy it.
            auto container_string = this->get_identifying_string();
            auto thing_string = thing->get_identifying_string();
            message = thing_string + CV(" vanish", " vanishes") + " in a puff of logic.";
            the_message_log.add(message);
            thing->destroy();
          }

          break;

        case ActionResult::FailureInventoryFull:
          /// @todo Handle the situation where the Thing's container can't hold the Thing.
          break;

        default:
          break;
      } // end switch (can_contain)
    } // end if (container location is not Mu)
    else
    {
      // Container's location is Mu, so just destroy it without a message.
      thing->destroy();
    }
  } // end for (contents of Thing)
}

void Thing::destroy()
{
  auto old_location = pImpl->location;

  if (get_intrinsic<int>("inventory_size") != 0)
  {
    // Spill the contents of this Thing into the Thing's location.
    spill();
  }

  if (old_location != ThingManager::get_mu())
  {
    old_location->get_inventory().remove(pImpl->ref);
  }
}

std::string Thing::get_bodypart_description(BodyPart part,
                                            unsigned int number)
{
  unsigned int total_number = this->get_bodypart_number(part);
  std::string part_name = this->get_bodypart_name(part);
  std::string result;

  ASSERT_CONDITION(number < total_number);
  switch (total_number)
  {
    case 0: // none of them!?  shouldn't occur!
      result = "non-existent " + part_name;
      MINOR_ERROR("Request for description of %s!?", result.c_str());
      break;

    case 1: // only one of them
      result = part_name;
      break;

    case 2: // assume a right and left one.
      switch (number)
      {
        case 0: result = "right " + part_name; break;
        case 1: result = "left " + part_name; break;
        default: break;
      }
      break;

    case 3: // assume right, center, and left.
      switch (number)
      {
        case 0: result = "right " + part_name; break;
        case 1: result = "center " + part_name; break;
        case 2: result = "left " + part_name; break;
        default: break;
      }
      break;

    case 4: // Legs/feet assume front/rear, others assume upper/lower.
      if ((part == BodyPart::Leg) || (part == BodyPart::Foot))
      {
        switch (number)
        {
          case 0: result = "front right " + part_name; break;
          case 1: result = "front left " + part_name; break;
          case 2: result = "rear right " + part_name; break;
          case 3: result = "rear left " + part_name; break;
          default: break;
        }
      }
      else
      {
        switch (number)
        {
          case 0: result = "upper right " + part_name; break;
          case 1: result = "upper left " + part_name; break;
          case 2: result = "lower right " + part_name; break;
          case 3: result = "lower left " + part_name; break;
          default: break;
        }
      }
      break;

    case 6: // Legs/feet assume front/middle/rear, others upper/middle/lower.
      if ((part == BodyPart::Leg) || (part == BodyPart::Foot))
      {
        switch (number)
        {
          case 0: result = "front right " + part_name; break;
          case 1: result = "front left " + part_name; break;
          case 2: result = "middle right " + part_name; break;
          case 3: result = "middle left " + part_name; break;
          case 4: result = "rear right " + part_name; break;
          case 5: result = "rear left " + part_name; break;
          default: break;
        }
      }
      else
      {
        switch (number)
        {
          case 0: result = "upper right " + part_name; break;
          case 1: result = "upper left " + part_name; break;
          case 2: result = "middle right " + part_name; break;
          case 3: result = "middle left " + part_name; break;
          case 4: result = "lower right " + part_name; break;
          case 5: result = "lower left " + part_name; break;
          default: break;
        }
      }
      break;

    default:
      break;
  }

  // Anything else and we just return the ordinal name.
  if (result.empty())
  {
    result = Ordinal::get(number) + " " + part_name;
  }

  return result;
}

bool Thing::is_movable_by(ThingRef thing)
{
  return call_lua_function_bool("is_movable_by", { thing }, true);
}

bool Thing::is_usable_by(ThingRef thing)
{
  return call_lua_function_bool("is_usable_by", { thing }, false);
}

bool Thing::is_drinkable_by(ThingRef thing)
{
  ThingRef contents = pImpl->inventory.get(INVSLOT_ZERO);
  return call_lua_function_bool("is_drinkable_by", { thing, contents }, false);
}

bool Thing::is_edible_by(ThingRef thing)
{
  return call_lua_function_bool("is_edible_by", { thing }, false);
}

bool Thing::is_readable_by(ThingRef thing)
{
  return call_lua_function_bool("is_readable_by", { thing }, false);
}

bool Thing::is_miscible_with(ThingRef thing)
{
  return call_lua_function_bool("is_miscible_with", { thing }, false);
}

BodyPart Thing::is_equippable_on() const
{
  return BodyPart::Count;
}

bool Thing::process()
{
  // Get a copy of the inventory's list of things.
  // This is because things can be deleted/removed from the inventory
  // over the course of processing them, and this might screw up the
  // iterator.
  auto things = pImpl->inventory.get_things();

  // Process inventory.
  for (auto iter = std::begin(things);
  iter != std::end(things);
    ++iter)
  {
    ThingRef thing = iter->second;
    bool dead = thing->process();
  }

  // Process self last.
  return _process_self();
}

ActionResult Thing::perform_action_died()
{
  ActionResult result = call_lua_function("perform_action_died", {});
  return result;
}

bool Thing::perform_action_activated_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_activated_by", { actor });
  return was_successful(result);
}

void Thing::perform_action_collided_with(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_collided_with", { actor });
  return;
}

void Thing::perform_action_collided_with_wall(Direction d, std::string tile_type)
{
  /// @todo Implement me; right now there's no way to pass one enum and one string to a Lua function.
  return;
}

ActionResult Thing::perform_action_drank_by(ThingRef actor)
{
  ThingRef contents = actor->get_inventory().get(INVSLOT_ZERO);
  ActionResult result = call_lua_function("perform_action_drank_by", { actor, contents });
  return result;
}

bool Thing::perform_action_dropped_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_dropped_by", { actor });
  return was_successful(result);
}

ActionResult Thing::perform_action_eaten_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_eaten_by", { actor });
  return result;
}

bool Thing::perform_action_used_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_used_by", { actor });

  return was_successful(result);
}

bool Thing::perform_action_picked_up_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_picked_up_by", { actor });
  return was_successful(result);
}

bool Thing::perform_action_put_into_by(ThingRef container, ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_put_into", { container, actor });
  return was_successful(result);
}

bool Thing::perform_action_taken_out_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_taken_out_by", { actor });
  return was_successful(result);
}

ActionResult Thing::perform_action_read_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_read_by", { actor });
  return result;
}

void Thing::perform_action_attack_hits(ThingRef target)
{
  ActionResult result = call_lua_function("perform_action_read_by", { target });
  return;
}

bool Thing::perform_action_thrown_by(ThingRef actor, Direction direction)
{
  ActionResult result = call_lua_function("perform_action_thrown_by", { actor, static_cast<lua_Integer>(direction) });
  return was_successful(result);
}

bool Thing::perform_action_deequipped_by(ThingRef actor, WearLocation& location)
{
  if (this->get_property<bool>("bound"))
  {
    std::string message;
    message = actor->get_identifying_string() + " cannot take off " + this->get_identifying_string() +
      "; it is magically bound to " +
      actor->get_possessive_adjective() + " " +
      actor->get_bodypart_description(location.part,
                                      location.number) + "!";
    the_message_log.add(message);
    return false;
  }
  else
  {
    ActionResult result = call_lua_function("perform_action_deequipped_by", { actor });
    return was_successful(result);
  }
}

bool Thing::perform_action_equipped_by(ThingRef actor, WearLocation& location)
{
  ActionResult result = call_lua_function("perform_action_equipped_by", { actor });
  bool subclass_result = was_successful(result);

  if (subclass_result == true)
  {
    if (this->get_property<bool>("autobinds"))
    {
      this->set_property<bool>("bound", true);
      std::string message;
      message = this->get_identifying_string() + " magically binds itself to " +
        actor->get_possessive() + " " +
        actor->get_bodypart_description(location.part,
                                        location.number) + "!";
      the_message_log.add(message);
    }
  }

  return subclass_result;
}

bool Thing::perform_action_unwielded_by(ThingRef actor)
{
  if (this->get_property<bool>("bound"))
  {
    std::string message;
    message = actor->get_identifying_string() + " cannot unwield " + this->get_identifying_string() +
      "; it is magically bound to " +
      actor->get_possessive_adjective() + " " +
      actor->get_bodypart_name(BodyPart::Hand) + "!";
    the_message_log.add(message);
    return false;
  }
  else
  {
    ActionResult result = call_lua_function("perform_action_unwielded_by", { actor });
    return was_successful(result);
  }
}

bool Thing::perform_action_wielded_by(ThingRef actor)
{
  ActionResult result = call_lua_function("perform_action_wielded_by", { actor });
  bool subclass_result = was_successful(result);

  if (subclass_result == true)
  {
    if (this->get_property<bool>("autobinds"))
    {
      this->set_property<bool>("bound", true);
      std::string message;
      message = this->get_identifying_string() + " magically binds itself to " +
        actor->get_possessive() + " " +
        actor->get_bodypart_name(BodyPart::Hand) + "!";
      the_message_log.add(message);
    }
  }

  return subclass_result;
}

bool Thing::perform_action_fired_by(ThingRef actor, Direction direction)
{
  ActionResult result = call_lua_function("perform_action_fired_by", { actor, static_cast<lua_Integer>(direction) });
  return was_successful(result);
}

bool Thing::can_merge_with(ThingRef other) const
{
  // Things with different types can't merge (obviously).
  if (other->get_type() != get_type())
  {
    return false;
  }

  // Things with inventories can never merge.
  if ((get_intrinsic<int>("inventory_size") != 0) ||
      (other->get_intrinsic<int>("inventory_size") != 0))
  {
    return false;
  }

  // If the things have the same properties, merge is okay.
  /// @todo Implement this. It's going to be a major pain in the ass.
  ///       We'll need a Lua function that compares the property arrays
  ///       of the two Things, and returns true or false.
  //if ((this->pImpl->properties) == (other->pImpl->properties))
  {
    return true;
  }

  return false;
}

ActionResult Thing::can_contain(ThingRef thing)
{
  unsigned int inventory_size = get_intrinsic<unsigned int>("inventory_size");
  if (inventory_size == 0)
  {
    return ActionResult::FailureTargetNotAContainer;
  }
  else if (get_inventory().count() >= inventory_size)
  {
    return ActionResult::FailureInventoryFull;
  }
  else
  {
    return call_lua_function("can_contain", { thing });
  }
}

void Thing::set_location(ThingRef target)
{
  pImpl->location = target;
}

// *** PROTECTED METHODS ******************************************************

bool Thing::_process_self()
{
  unsigned int action_time = 0;
  bool success = false;
  int counter_busy = get_property<int>("counter_busy");

  // Is this an entity that is now dead?
  if ((get_property<bool>("is_entity") == true) && (get_property<int>("hp") <= 0))
  {
    // Did the entity JUST die?
    if (get_property<bool>("dead") != true)
    {
      // Perform the "die" action.
      // (This sets the "dead" property and clears out any pending actions.)
      if (this->do_die() == true)
      {
        /// @todo Handle player death by transitioning to game end state.
      }
    }
  }
  // Otherwise if this entity is busy...
  else if (counter_busy > 0)
  {
    // Decrement busy counter.
    add_to_property<int>("counter_busy", -1);
  }
  // Otherwise if actions are pending...
  else if (!pImpl->pending_actions.empty())
  {
    // Process the front action.
    std::unique_ptr<Action>& action = pImpl->pending_actions.front();
    bool action_done = action->process(get_ref(), {});
    if (action_done)
    {
      TRACE("Thing %s (%s): Action %s is done, popping",
            get_ref().get_id().to_string().c_str(),
            get_type().c_str(),
            action->get_type().c_str());
      pImpl->pending_actions.pop_front();
    }
  } // end if (actions pending)

  return true;
}

std::vector<std::string>& Thing::get_map_memory()
{
  return pImpl->map_memory;
}

void Thing::do_recursive_visibility(int octant,
                                    int depth,
                                    float slope_A,
                                    float slope_B)
{
  sf::Vector2i new_coords;
  //int x = 0;
  //int y = 0;

  // Are we on a map?  Bail out if we aren't.
  if (is_inside_another_thing())
  {
    return;
  }

  MapTile* tile = get_maptile();
  sf::Vector2i tile_coords = tile->get_coords();
  Map& game_map = GAME.get_map_factory().get(get_map_id());

  static const int mv = 128;
  static constexpr int mw = (mv * mv);

  std::function< bool(sf::Vector2f, sf::Vector2f, float) > loop_condition;
  Direction dir;
  std::function< float(sf::Vector2f, sf::Vector2f) > recurse_slope;
  std::function< float(sf::Vector2f, sf::Vector2f) > loop_slope;

  switch (octant)
  {
    case 1:
      new_coords.x = static_cast<int>(rint(static_cast<float>(tile_coords.x) - (slope_A * static_cast<float>(depth))));
      new_coords.y = tile_coords.y - depth;
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_slope(a, b) >= c; };
      dir = Direction::West;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Southwest), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Northwest), b); };
      break;

    case 2:
      new_coords.x = static_cast<int>(rint(static_cast<float>(tile_coords.x) + (slope_A * static_cast<float>(depth))));
      new_coords.y = tile_coords.y - depth;
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_slope(a, b) <= c; };
      dir = Direction::East;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Southeast), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return -calc_slope(a + halfunit(Direction::Northeast), b); };
      break;

    case 3:
      new_coords.x = tile_coords.x + depth;
      new_coords.y = static_cast<int>(rint(static_cast<float>(tile_coords.y) - (slope_A * static_cast<float>(depth))));
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_inv_slope(a, b) <= c; };
      dir = Direction::North;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Northwest), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return -calc_inv_slope(a + halfunit(Direction::Northeast), b); };
      break;

    case 4:
      new_coords.x = tile_coords.x + depth;
      new_coords.y = static_cast<int>(rint(static_cast<float>(tile_coords.y) + (slope_A * static_cast<float>(depth))));
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_inv_slope(a, b) >= c; };
      dir = Direction::South;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Southwest), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Southeast), b); };
      break;

    case 5:
      new_coords.x = static_cast<int>(rint(static_cast<float>(tile_coords.x) + (slope_A * static_cast<float>(depth))));
      new_coords.y = tile_coords.y + depth;
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_slope(a, b) >= c; };
      dir = Direction::East;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Northeast), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Southeast), b); };
      break;

    case 6:
      new_coords.x = static_cast<int>(rint(static_cast<float>(tile_coords.x) - (slope_A * static_cast<float>(depth))));
      new_coords.y = tile_coords.y + depth;
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_slope(a, b) <= c; };
      dir = Direction::West;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_slope(a + halfunit(Direction::Northwest), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return -calc_slope(a + halfunit(Direction::Southwest), b); };
      break;

    case 7:
      new_coords.x = tile_coords.x - depth;
      new_coords.y = static_cast<int>(rint(static_cast<float>(tile_coords.y) + (slope_A * static_cast<float>(depth))));
      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_inv_slope(a, b) <= c; };
      dir = Direction::South;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Southeast), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return -calc_inv_slope(a + halfunit(Direction::Southwest), b); };
      break;

    case 8:
      new_coords.x = tile_coords.x - depth;
      new_coords.y = static_cast<int>(rint(static_cast<float>(tile_coords.y) - (slope_A * static_cast<float>(depth))));

      loop_condition = [](sf::Vector2f a, sf::Vector2f b, float c) { return calc_inv_slope(a, b) >= c; };
      dir = Direction::North;
      recurse_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Northeast), b); };
      loop_slope = [](sf::Vector2f a, sf::Vector2f b) { return calc_inv_slope(a + halfunit(Direction::Northwest), b); };
      break;

    default:
      MAJOR_ERROR("Octant passed to do_recursive_visibility was %d (not 1 to 8)!", octant);
      break;
  }

  while (loop_condition(to_v2f(new_coords), to_v2f(tile_coords), slope_B))
  {
    if (calc_vis_distance(new_coords, tile_coords) <= mw)
    {
      if (game_map.tile_is_opaque(new_coords))
      {
        if (!game_map.tile_is_opaque(new_coords + unit(dir)))
        {
          do_recursive_visibility(octant, depth + 1, slope_A, recurse_slope(to_v2f(new_coords), to_v2f(tile_coords)));
        }
      }
      else
      {
        if (game_map.tile_is_opaque(new_coords + unit(dir)))
        {
          slope_A = loop_slope(to_v2f(new_coords), to_v2f(tile_coords));
        }
      }
      pImpl->tiles_currently_seen[game_map.get_index(new_coords.x, new_coords.y)] = true;
      pImpl->map_memory[game_map.get_index(new_coords.x, new_coords.y)] = game_map.get_tile(new_coords.x, new_coords.y).get_tile_type();
    }
    new_coords -= unit(dir);
  }
  new_coords += unit(dir);

  if ((depth < mv) && (!game_map.get_tile(new_coords.x, new_coords.y).is_opaque()))
  {
    do_recursive_visibility(octant, depth + 1, slope_A, slope_B);
  }
}

// *** PRIVATE METHODS ********************************************************

MapTile* Thing::_get_maptile() const
{
  return pImpl->map_tile;
}

int Thing::LUA_thing_create(lua_State* L)
{
  int num_args = lua_gettop(L);

  if ((num_args < 2) || (num_args > 3))
  {
    MINOR_ERROR("expected 2 or 3 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  std::string new_thing_type = lua_tostring(L, 2);

  ThingRef new_thing = GAME.get_thing_manager().create(new_thing_type);
  bool success = new_thing->move_into(thing);

  if (num_args > 2)
  {
    int quantity = lua_tointeger(L, 3);
    new_thing->set_quantity(static_cast<unsigned int>(quantity));
  }

  if (success)
  {
    lua_pushinteger(L, new_thing);
  }
  else
  {
    lua_pushnil(L);
  }

  return 1;
}

int Thing::LUA_thing_destroy(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 1)
  {
    MINOR_ERROR("expected 1 argument, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  thing->destroy();

  return 0;
}

int Thing::LUA_thing_get_player(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 0)
  {
    MINOR_ERROR("expected 0 arguments, got %d", num_args);
    return 0;
  }

  ThingRef player = GAME.get_player();
  lua_pushinteger(L, player);

  return 1;
}

int Thing::LUA_thing_get_coords(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 1)
  {
    MINOR_ERROR("expected 1 argument, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));

  MapId map_id = GAME.get_player()->get_map_id();
  auto maptile = thing->get_maptile();

  if (maptile != nullptr)
  {
    auto coords = maptile->get_coords();
    lua_pushinteger(L, coords.x);
    lua_pushinteger(L, coords.y);
  }
  else
  {
    lua_pushnil(L);
    lua_pushnil(L);
  }

  return 2;
}

int Thing::LUA_thing_get_type(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 1)
  {
    MINOR_ERROR("expected 1 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  std::string result = thing->get_type();
  lua_pushstring(L, result.c_str());

  return 1;
}

int Thing::LUA_thing_get_property_flag(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 2)
  {
    MINOR_ERROR("expected 2 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  bool result = thing->get_property<bool>(key);
  lua_pushboolean(L, result);

  return 1;
}

int Thing::LUA_thing_get_property_value(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 2)
  {
    MINOR_ERROR("expected 2 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  int result = thing->get_property<int>(key);
  lua_pushinteger(L, result);

  return 1;
}

int Thing::LUA_thing_get_property_string(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 2)
  {
    MINOR_ERROR("expected 2 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  const char* result = thing->get_property<std::string>(key).c_str();
  lua_pushstring(L, result);

  return 1;
}

int Thing::LUA_thing_set_property_flag(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 3)
  {
    MINOR_ERROR("expected 3 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  bool value = (lua_toboolean(L, 3) != 0);
  thing->set_property<bool>(key, value);

  return 0;
}

int Thing::LUA_thing_set_property_value(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 3)
  {
    MINOR_ERROR("expected 3 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  int value = static_cast<int>(lua_tointeger(L, 3));
  thing->set_property<int>(key, value);

  return 0;
}

int Thing::LUA_thing_set_property_string(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 3)
  {
    MINOR_ERROR("expected 3 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing = ThingRef(lua_tointeger(L, 1));
  const char* key = lua_tostring(L, 2);
  const char* value = lua_tostring(L, 3);
  std::string svalue = std::string(value);
  thing->set_property<std::string>(key, svalue);

  return 0;
}

int Thing::LUA_thing_move_into(lua_State* L)
{
  int num_args = lua_gettop(L);

  if (num_args != 2)
  {
    MINOR_ERROR("expected 2 arguments, got %d", num_args);
    return 0;
  }

  ThingRef thing_to_move = ThingRef(lua_tointeger(L, 1));
  ThingRef thing_destination = ThingRef(lua_tointeger(L, 2));

  bool result = thing_to_move->move_into(thing_destination);

  lua_pushboolean(L, static_cast<int>(result));

  return 1;
}

ActionResult Thing::call_lua_function(std::string function_name, std::vector<lua_Integer> const& args, ActionResult default_result)
{
  return pImpl->metadata.call_lua_function(function_name, get_ref(), args, default_result);
}

bool Thing::call_lua_function_bool(std::string function_name, std::vector<lua_Integer> const& args, bool default_result)
{
  return pImpl->metadata.call_lua_function_bool(function_name, get_ref(), args, default_result);
}

sf::Vector2u Thing::call_lua_function_v2u(std::string function_name, std::vector<lua_Integer> const& args, sf::Vector2u default_result)
{
  return pImpl->metadata.call_lua_function_v2u(function_name, get_ref(), args, default_result);
}