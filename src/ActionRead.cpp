#include "ActionRead.h"
#include "Thing.h"
#include "ThingRef.h"

ActionRead::ActionRead(ThingRef subject, ThingRef object)
  :
  Action(subject, object)
{}

ActionRead::~ActionRead()
{}

Action::StateResult ActionRead::do_prebegin_work(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionRead::do_begin_work(AnyMap& params)
{
  the_message_log.add("We're sorry, but that action has not yet been implemented.");

  return Action::StateResult::Failure();
}

Action::StateResult ActionRead::do_finish_work(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionRead::do_abort_work(AnyMap& params)
{
  return Action::StateResult::Success();
}