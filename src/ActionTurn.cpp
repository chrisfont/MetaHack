#include "stdafx.h"

#include "ActionTurn.h"

#include "ActionAttack.h"
#include "GameState.h"
#include "Map.h"
#include "Thing.h"
#include "ThingId.h"

ACTION_SRC_BOILERPLATE(ActionTurn, "turn", L"turn")

Action::StateResult ActionTurn::do_prebegin_work_(AnyMap& params)
{
  StringDisplay message;

  auto subject = get_subject();
  auto location = subject->get_location();
  auto new_direction = get_target_direction();

  if (!IS_PLAYER)
  {
    print_message_try_();

    message = L"But ";
  }
  else
  {
    message = L"";
  }

  // Make sure we CAN move.
  /// @todo Split moving/turning up? It seems reasonable that a creature
  ///       might be able to swivel in place without being able to move.
  if (!subject->get_intrinsic<bool>("can_move", false))
  {
    message += YOU + CV(L" don't", L" doesn't") + L" have the capability of movement.";
    the_message_log.add(message);
    return Action::StateResult::Failure();
  }

  // Make sure we can move RIGHT NOW.
  if (!subject->can_currently_move())
  {
    message += YOU + L" can't move right now.";
    the_message_log.add(message);
    return Action::StateResult::Failure();
  }
  
  return Action::StateResult::Success();
}

/// @todo Implement me.
Action::StateResult ActionTurn::do_begin_work_(AnyMap& params)
{
  StateResult result = StateResult::Failure();

  StringDisplay message;

  auto subject = get_subject();
  ThingId location = subject->get_location();
  MapTile* current_tile = subject->get_maptile();
  Direction new_direction = get_target_direction();

  if ((new_direction != Direction::Up) &&
      (new_direction != Direction::Down))
  {
    /// @todo Change facing direction.
    result = StateResult::Success();

  } // end else if (other direction)

  return result;
}

Action::StateResult ActionTurn::do_finish_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionTurn::do_abort_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}