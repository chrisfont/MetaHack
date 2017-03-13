#include "stdafx.h"

#include "ActionGet.h"
#include "ActionMove.h"
#include "IMessageLog.h"
#include "Service.h"
#include "Entity.h"
#include "EntityId.h"

namespace Actions
{
  ActionGet ActionGet::prototype;
  ActionGet::ActionGet() : Action("get", "GET", ActionGet::create_) {}
  ActionGet::ActionGet(EntityId subject) : Action(subject, "get", "GET") {}
  ActionGet::~ActionGet() {}

  std::unordered_set<Trait> const & ActionGet::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObject,
      Trait::CanTakeAQuantity
    };

    return traits;
  }

  StateResult ActionGet::do_prebegin_work_(AnyMap& params)
  {
    std::string message;
    auto subject = get_subject();
    auto object = get_object();
    EntityId location = subject->getLocation();

    // Verify that the Action has an object.
    if (object == EntityId::Mu())
    {
      return StateResult::Failure();
    }

    // Check that it isn't US!
    if (object == subject)
    {
      if (IS_PLAYER)
      {
        message = maketr("ACTION_YOU_TRY_TO_PICKUP_YOURSELF_HUMOROUS");
      }
      else
      {
        message = maketr("ACTION_YOU_TRY_TO_VERB_YOURSELF_INVALID");
        CLOG(WARNING, "Action") << "NPC tried to pick self up!?";
      }
      Service<IMessageLog>::get().add(message);
      return StateResult::Failure();
    }

    // Check if it's already in our inventory.
    if (subject->get_inventory().contains(object))
    {
      message = maketr("ACTION_YOU_TRY_TO_VERB_THE_FOO");
      Service<IMessageLog>::get().add(message);

      message = maketr("THE_FOO_IS_ALREADY_IN_YOUR_INVENTORY");
      Service<IMessageLog>::get().add(message);
      return StateResult::Failure();
    }

    /// @todo When picking up, check if our inventory is full-up.
    if (false)
    {
      message = maketr("ACTION_YOU_TRY_TO_VERB_THE_FOO");
      Service<IMessageLog>::get().add(message);

      message = maketr("YOUR_INVENTORY_CANT_HOLD_THE_FOO");
      Service<IMessageLog>::get().add(message);
      return StateResult::Failure();
    }

    if (!object->can_have_action_done_by(subject, ActionMove::prototype))
    {
      message = maketr("ACTION_YOU_TRY_TO_VERB_THE_FOO");
      Service<IMessageLog>::get().add(message);

      message = maketr("ACTION_YOU_CANT_MOVE_THE_FOO");
      Service<IMessageLog>::get().add(message);
      return StateResult::Failure();
    }

    return StateResult::Success();
  }

  StateResult ActionGet::do_begin_work_(AnyMap& params)
  {
    /// @todo Handle getting a certain quantity of an item.
    StateResult result = StateResult::Failure();
    std::string message;
    auto subject = get_subject();
    auto object = get_object();

    if (object->be_object_of(*this, subject) == ActionResult::Success)
    {
      message = maketr("ACTION_YOU_CVERB_THE_FOO");
      Service<IMessageLog>::get().add(message);
      if (object->move_into(subject))
      {
        /// @todo Figure out action time.
        result = StateResult::Success();
      }
      else // could not add to inventory
      {
        message = maketr("ACTION_YOU_CANT_VERB_FOO_UNKNOWN");
        Service<IMessageLog>::get().add(message);

        CLOG(WARNING, "Action") << "Could not move Entity " << object <<
          " even though be_object_of returned Success";
      }
    }

    return result;
  }

  StateResult ActionGet::do_finish_work_(AnyMap& params)
  {
    return StateResult::Success();
  }

  StateResult ActionGet::do_abort_work_(AnyMap& params)
  {
    return StateResult::Success();
  }
} // end namespace