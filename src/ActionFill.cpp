#include "ActionFill.h"
#include "Thing.h"
#include "ThingRef.h"

ACTION_SRC_BOILERPLATE(ActionFill, "Fill")

Action::StateResult ActionFill::do_prebegin_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionFill::do_begin_work_(AnyMap& params)
{
  the_message_log.add("We're sorry, but that action has not yet been implemented.");

  return Action::StateResult::Failure();
}

Action::StateResult ActionFill::do_finish_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionFill::do_abort_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}