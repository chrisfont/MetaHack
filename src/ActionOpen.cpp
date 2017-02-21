#include "stdafx.h"

#include "ActionOpen.h"
#include "Thing.h"
#include "ThingId.h"
#include "IStringDictionary.h"
#include "Service.h"

ACTION_SRC_BOILERPLATE(ActionOpen, "open", "open")

Action::StateResult ActionOpen::do_prebegin_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionOpen::do_begin_work_(AnyMap& params)
{
  bool success = false;
  unsigned int action_time = 0;

  auto& dict = Service<IStringDictionary>::get();
  the_message_log.add(dict.get("NOT_IMPLEMENTED_MSG"));

#if 0
  if (thing != ThingId::Mu())
  {
    success = actor->do_open(thing, action_time);
  }
#endif

  return{ success, action_time };
}

Action::StateResult ActionOpen::do_finish_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionOpen::do_abort_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}