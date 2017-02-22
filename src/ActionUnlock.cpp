#include "stdafx.h"

#include "ActionUnlock.h"
#include "IMessageLog.h"
#include "IStringDictionary.h"
#include "Service.h"
#include "Thing.h"
#include "ThingId.h"

ACTION_SRC_BOILERPLATE(ActionUnlock, "unlock", "unlock")

Action::StateResult ActionUnlock::do_prebegin_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionUnlock::do_begin_work_(AnyMap& params)
{
  bool success = false;
  unsigned int action_time = 0;

  auto& dict = Service<IStringDictionary>::get();
  Service<IMessageLog>::get().add(dict.get("ACTION_NOT_IMPLEMENTED"));

  return{ success, action_time };
}

Action::StateResult ActionUnlock::do_finish_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}

Action::StateResult ActionUnlock::do_abort_work_(AnyMap& params)
{
  return Action::StateResult::Success();
}