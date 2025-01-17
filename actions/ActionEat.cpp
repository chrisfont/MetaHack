#include "ActionEat.h"

#include "config/Strings.h"
#include "lua/LuaObject.h"
#include "systems/SystemJanitor.h"
#include "systems/Manager.h"
#include "systems/SystemNarrator.h"
#include "utilities/Shortcuts.h"

namespace Actions
{
  ActionEat ActionEat::prototype;
  ActionEat::ActionEat() : Action("EAT", ActionEat::create_) {}
  ActionEat::ActionEat(EntityId subject) : Action(subject, "EAT") {}
  ActionEat::~ActionEat() {}

  std::unordered_set<Trait> const & ActionEat::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObject,
      Trait::ObjectCanBeSelf
    };

    return traits;
  }

  StateResult ActionEat::doPreBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments)
  {
    auto& narrator = systems.narrator();
    std::string message;
    auto subject = getSubject();
    auto object = getObject();

    // Check that it isn't US!
    if (subject == object)
    {
      printMessageTry(systems, arguments);

      /// @todo Handle "unusual" cases (e.g. zombies?)
      putMsg(narrator.makeTr("YOU_TRY_TO_EAT_YOURSELF_HUMOROUS", arguments));
      return StateResult::Failure();
    }

    return StateResult::Success();
  }

  StateResult ActionEat::doBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments)
  {
    auto subject = getSubject();
    auto object = getObject();
    auto& lua = gameState.lua();

    printMessageBegin(systems, arguments);

    // Do the eating action here.
    /// @todo "Partially eaten" status for entities that were started to be eaten
    ///       but were interrupted.
    /// @todo Figure out eating time. This will obviously vary based on the
    ///       object being eaten.
    m_last_eat_result = lua.doSubjectActionObject(subject, *this, object);

    if (m_last_eat_result)
    {
      return StateResult::Success();
    }
    else
    {
      printMessageStop(systems, arguments);
      return StateResult::Failure();
    }
  }

  StateResult ActionEat::doFinishWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments)
  {
    auto object = getObject();

    printMessageFinish(systems, arguments);

    if (m_last_eat_result)
    {
      systems.janitor().markForDeletion(object);
    }

    return StateResult::Success();
  }

  StateResult ActionEat::doAbortWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments)
  {
    printMessageStop(systems, arguments);
    return StateResult::Success();
  }
} // end namespace
