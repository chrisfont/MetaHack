#include "ActionQuaff.h"
#include "Thing.h"
#include "ThingRef.h"

ActionQuaff::ActionQuaff(ThingRef subject)
  :
  Action(subject)
{}

ActionQuaff::ActionQuaff(ThingRef subject, ThingRef object)
  :
  Action(subject, object)
{}

ActionQuaff::~ActionQuaff()
{}

Action::StateResult ActionQuaff::do_prebegin_work(AnyMap& params)
{
  std::string message;
  auto subject = get_subject();
  auto object = get_object();

  // Check that it isn't US!
  if (subject == object)
  {
    message = YOU_TRY + " to drink " + YOURSELF + ".";
    the_message_log.add(message);

    /// @todo When drinking self, special message if caller is a liquid-based organism.
    message = "Needless to say, " + YOU_ARE + " not very successful in this endeavor.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that we're capable of drinking at all.
  if (subject->get_intrinsic<bool>("can_drink"))
  {
    message = YOU_TRY_TO("drink from") + THE_FOO + ".";
    the_message_log.add(message);
    message = "But, as " + getIndefArt(subject->get_display_name()) + subject->get_display_name() + "," + YOU_ARE + " not capable of drinking liquids.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that the thing is within reach.
  if (!subject->can_reach(object))
  {
    message = YOU_TRY_TO("drink from") + THE_FOO + ", but " + OBJ_PRO_FOO + FOO_IS + " out of " + YOUR + " reach.";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that it is something that contains a liquid.
  if (!object->get_intrinsic<bool>("liquid_carrier"))
  {
    message = YOU_TRY_TO("drink from") + THE_FOO + ".";
    the_message_log.add(message);
    message = YOU + " cannot drink from that!";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that it is not empty.
  Inventory& inv = object->get_inventory();
  if (inv.count() == 0)
  {
    message = YOU_TRY_TO("drink from") + THE_FOO + ".";
    the_message_log.add(message);
    message = "But " + THE_FOO + FOO_IS + " empty!";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  // Check that you can drink what is inside it.
  if (!inv.get(INVSLOT_ZERO)->is_drinkable_by(subject))
  {
    message = YOU + " can't drink that!";
    the_message_log.add(message);

    return Action::StateResult::Failure();
  }

  return Action::StateResult::Success();
}

Action::StateResult ActionQuaff::do_begin_work(AnyMap& params)
{
  std::string message;
  auto subject = get_subject();
  auto object = get_object();

  message = YOU + " drink from " + THE_FOO + ".";
  the_message_log.add(message);

  // Do the drinking action here.
  /// @todo Figure out drinking time.
  ActionResult result = object->perform_action_drank_by(subject);

  switch (result)
  {
    case ActionResult::Success:
    case ActionResult::SuccessDestroyed:
      return Action::StateResult::Success();

    case ActionResult::Failure:
      message = YOU + " stop drinking.";
      the_message_log.add(message);
      return Action::StateResult::Failure();

    default:
      MINOR_ERROR("Unknown ActionResult %d", result);
      return Action::StateResult::Failure();
  }
}

Action::StateResult ActionQuaff::do_finish_work(AnyMap& params)
{
  std::string message;
  auto object = get_object();

  message = YOU + " finish drinking.";
  the_message_log.add(message);
  object->get_inventory().get(INVSLOT_ZERO)->destroy();
  return Action::StateResult::Success();
}

Action::StateResult ActionQuaff::do_abort_work(AnyMap& params)
{
  std::string message;

  message = YOU + " stop drinking.";
  the_message_log.add(message);
  return Action::StateResult::Success();
}