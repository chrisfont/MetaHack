#include "stdafx.h"

#include "ActionFill.h"
#include "IMessageLog.h"
#include "IStringDictionary.h"
#include "Service.h"
#include "Entity.h"
#include "EntityId.h"

namespace Actions
{
  ActionFill ActionFill::prototype;
  ActionFill::ActionFill() : Action("fill", "FILL", ActionFill::create_) {}
  ActionFill::ActionFill(EntityId subject) : Action(subject, "fill", "FILL") {}
  ActionFill::~ActionFill() {}

  std::unordered_set<Trait> const & ActionFill::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObjectPrepositionTarget
    };

    return traits;
  }

  StateResult ActionFill::do_prebegin_work_(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionFill::do_begin_work_(AnyMap& params)
  {
    auto& dict = Service<IStringDictionary>::get();
    Service<IMessageLog>::get().add(dict.get("ACTION_NOT_IMPLEMENTED"));

    return StateResult::Failure();
  }

  StateResult ActionFill::do_finish_work_(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionFill::do_abort_work_(AnyMap& params)
  {
    return StateResult::Success();
  }
} // end namespace
