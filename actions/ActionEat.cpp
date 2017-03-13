#include "stdafx.h"

#include "ActionEat.h"
#include "IMessageLog.h"
#include "IStringDictionary.h"
#include "Service.h"
#include "Entity.h"
#include "EntityId.h"

namespace Actions
{
  ActionEat ActionEat::prototype;
  ActionEat::ActionEat() : Action("eat", "EAT", ActionEat::create_) {}
  ActionEat::ActionEat(EntityId subject) : Action(subject, "eat", "EAT") {}
  ActionEat::~ActionEat() {}

  std::unordered_set<Trait> const & ActionEat::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObject
    };

    return traits;
  }

  StateResult ActionEat::do_prebegin_work_(AnyMap& params)
  {
    std::string message;
    auto subject = get_subject();
    auto object = get_object();

    // Check that it isn't US!
    if (subject == object)
    {
      print_message_try_();

      /// @todo Handle "unusual" cases (e.g. zombies?)
      message = maketr("ACTION_YOU_WONT_EAT_SELF");
      Service<IMessageLog>::get().add(message);

      return StateResult::Failure();
    }

    // Check that we're capable of eating at all.
    if (subject->get_modified_property<bool>("can_eat"))
    {
      print_message_try_();
      message = maketr("ACTION_YOU_ARE_NOT_CAPABLE_OF_VERBING", { getIndefArt(subject->get_display_name()), subject->get_display_name() });
      Service<IMessageLog>::get().add(message);

      return StateResult::Failure();
    }

    return StateResult::Success();
  }

  StateResult ActionEat::do_begin_work_(AnyMap& params)
  {
    auto subject = get_subject();
    auto object = get_object();

    print_message_begin_();

    // Do the eating action here.
    /// @todo "Partially eaten" status for entities that were started to be eaten
    ///       but were interrupted.
    /// @todo Figure out eating time. This will obviously vary based on the
    ///       object being eaten.
    m_last_eat_result = object->be_object_of(*this, subject);

    switch (m_last_eat_result)
    {
      case ActionResult::Success:
      case ActionResult::SuccessDestroyed:
        return StateResult::Success();

      case ActionResult::Failure:
        print_message_stop_();
        return StateResult::Failure();

      default:
        CLOG(WARNING, "Action") << "Unknown ActionResult " << m_last_eat_result;
        return StateResult::Failure();
    }
  }

  StateResult ActionEat::do_finish_work_(AnyMap& params)
  {
    auto object = get_object();

    print_message_finish_();

    if (m_last_eat_result == ActionResult::SuccessDestroyed)
    {
      object->destroy();
    }

    return StateResult::Success();
  }

  StateResult ActionEat::do_abort_work_(AnyMap& params)
  {
    print_message_stop_();
    return StateResult::Success();
  }
} // end namespace