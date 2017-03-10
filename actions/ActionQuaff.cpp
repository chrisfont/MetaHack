#include "stdafx.h"

#include "ActionQuaff.h"
#include "IMessageLog.h"
#include "IStringDictionary.h"
#include "Service.h"
#include "Entity.h"
#include "EntityId.h"

namespace Actions
{
  ActionQuaff ActionQuaff::prototype;
  ActionQuaff::ActionQuaff() : Action("quaff", "DRINK", ActionQuaff::create_) {}
  ActionQuaff::ActionQuaff(EntityId subject) : Action(subject, "quaff", "DRINK") {}
  ActionQuaff::~ActionQuaff() {}

  std::unordered_set<Action::Trait> const & ActionQuaff::getTraits() const
  {
    static std::unordered_set<Action::Trait> traits =
    {
      Trait::CanBeSubjectVerbObject,
      Trait::CanBeSubjectVerbDirection
    };

    return traits;
  }

  StateResult ActionQuaff::do_prebegin_work_(AnyMap& params)
  {
    std::string message;
    auto subject = get_subject();
    auto object = get_object();

    // Check that it isn't US!
    if (subject == object)
    {
      print_message_try_();

      /// @todo When drinking self, special message if caller is a liquid-based organism.
      message = "Ewwww... no.";
      Service<IMessageLog>::get().add(message);

      return StateResult::Failure();
    }

    // Check that we're capable of drinking at all.
    if (subject->get_modified_property<bool>("can_drink"))
    {
      print_message_try_();

      message = "But, as " + getIndefArt(subject->get_display_name()) + subject->get_display_name() + "," + YOU_ARE + " not capable of drinking liquids.";
      Service<IMessageLog>::get().add(message);

      return StateResult::Failure();
    }

    // Check that it is not empty.
    Inventory& inv = object->get_inventory();
    if (inv.count() == 0)
    {
      print_message_try_();

      message = "But " + THE_FOO + FOO_IS + " empty!";
      Service<IMessageLog>::get().add(message);

      return StateResult::Failure();
    }

    return StateResult::Success();
  }

  StateResult ActionQuaff::do_begin_work_(AnyMap& params)
  {
    auto subject = get_subject();
    auto object = get_object();
    auto contents = object->get_inventory()[INVSLOT_ZERO];

    print_message_begin_();

    // Do the drinking action here.
    /// @todo We drink from the object, but it's what is inside that is
    ///       actually being consumed. Do we call be_object_of() on the
    ///       object, or on the object's contents?
    ///       For now I am assuming we do it on the contents, since they
    ///       are what will affect the drinker.
    /// @todo Figure out drinking time. This will vary based on the contents
    ///       being consumed.
    ActionResult result = contents->be_object_of(*this, subject);

    switch (result)
    {
      case ActionResult::Success:
        return StateResult::Success();

      case ActionResult::SuccessDestroyed:
        contents->destroy();
        return StateResult::Success();

      case ActionResult::Failure:
        print_message_stop_();
        return StateResult::Failure();

      default:
        CLOG(WARNING, "Action") << "Unknown ActionResult " << result;
        return StateResult::Failure();
    }
  }

  StateResult ActionQuaff::do_finish_work_(AnyMap& params)
  {
    auto object = get_object();

    print_message_finish_();
    return StateResult::Success();
  }

  StateResult ActionQuaff::do_abort_work_(AnyMap& params)
  {
    print_message_stop_();
    return StateResult::Success();
  }
} // end namespace

