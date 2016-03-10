#include "stdafx.h"

#include "ActionDrop.h"
#include "ActionMove.h"
#include "Thing.h"
#include "ThingRef.h"

ACTION_SRC_BOILERPLATE(ActionDrop, "drop", "drop")

Action::StateResult ActionDrop::do_prebegin_work_(AnyMap& params)
{
  StringDisplay message;
  auto subject = get_subject();
  auto object = get_object();
  ThingRef location = subject->get_location();

  // If it's us, this is a special case. Return success.
  if (subject == object)
  {
    return Action::StateResult::Success();
  }

  // Check that it's in our inventory.
  if (!subject->get_inventory().contains(object))
  {
    print_message_try_();

    message = THE_FOO + object->choose_verb(" are ", " is ") +
      "not in " + YOUR + " inventory!";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that we're not wielding the item.
  if (subject->is_wielding(object))
  {
    print_message_try_();

    /// @todo Perhaps automatically try to unwield the item before dropping?
    message = YOU + " cannot drop something that is currently being wielded.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that we're not wearing the item.
  if (subject->has_equipped(object))
  {
    print_message_try_();

    message = YOU + " cannot drop something that is currently being worn.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that we can move the item.
  if (!object->can_have_action_done_by(subject, ActionMove::prototype))
  {
    print_message_try_();

    message = THE_FOO + " cannot be moved.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  return Action::StateResult::Success();
}

Action::StateResult ActionDrop::do_begin_work_(AnyMap& params)
{
  Action::StateResult result = StateResult::Failure();
  StringDisplay message;
  auto subject = get_subject();
  auto object = get_objects().front();
  ThingRef location = subject->get_location();

  /// @todo Handle dropping a certain quantity of an item.

  if (object == subject)
  {
    message = YOU + CV(" hurl ", " hurls ") + YOURSELF + " to the " +
      location->get_display_name() + "!";
    the_message_log.add(message);
    /// @todo Possible damage from hurling yourself to the ground!
    message = (IS_PLAYER ? "Fortunately, " : "") + YOU_SEEM + " unharmed.";
    the_message_log.add(message);
    message = YOU_GET + " up.";
    the_message_log.add(message);
  }
  else if (object != MU)
  {
    if (location->can_contain(object) == ActionResult::Success)
    {
      if (object->be_object_of(*this, subject) == ActionResult::Success)
      {
        print_message_do_();

        if (object->move_into(location))
        {
          /// @todo Figure out action time.
          result = StateResult::Success();
        }
        else
        {
          message = YOU + " could not drop " + THE_FOO + " for some inexplicable reason.";
          the_message_log.add(message);

          CLOG(WARNING, "Action") << "Could not drop Thing " << object <<
            " even though be_object_of returned Success";
        }
      }
      else // Drop failed
      {
        // be_object_of() will print any relevant messages
      }
    }
    else // can't contain the thing
    {
      // This is mighty strange, but I suppose there might be MapTiles in
      // the future that can't contain certain Things.
      print_message_try_();

      message = location->get_identifying_string() + " cannot hold " + THE_FOO + ".";
      the_message_log.add(message);
    }
  }

  return result;
}

Action::StateResult ActionDrop::do_finish_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionDrop::do_abort_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}