#include "stdafx.h"

#include "ActionWear.h"
#include "IMessageLog.h"
#include "IStringDictionary.h"
#include "Service.h"
#include "Entity.h"
#include "EntityId.h"

namespace Actions
{
  ActionWear ActionWear::prototype;
  ActionWear::ActionWear() : Action("wear", "WEAR", ActionWear::create_) {}
  ActionWear::ActionWear(EntityId subject) : Action(subject, "wear", "WEAR") {}
  ActionWear::~ActionWear() {}

  std::unordered_set<Trait> const & ActionWear::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObjectPrepositionBodypart,
      Trait::ObjectMustBeInInventory
    };

    return traits;
  }

  StateResult ActionWear::do_prebegin_work_(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionWear::do_begin_work_(AnyMap& params)
  {
    auto& dict = Service<IStringDictionary>::get();
    Service<IMessageLog>::get().add(dict.get("ACTION_NOT_IMPLEMENTED"));

    return StateResult::Failure();
  }

  StateResult ActionWear::do_finish_work_(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionWear::do_abort_work_(AnyMap& params)
  {
    return StateResult::Success();
  }
}
