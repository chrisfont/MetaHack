#include "stdafx.h"

#include "ActionRead.h"
#include "services/IMessageLog.h"
#include "services/IStringDictionary.h"
#include "Service.h"
#include "entity/Entity.h"
#include "entity/EntityId.h"

namespace Actions
{
  ActionRead ActionRead::prototype;
  ActionRead::ActionRead() : Action("read", "READ", ActionRead::create_) {}
  ActionRead::ActionRead(EntityId subject) : Action(subject, "read", "READ") {}
  ActionRead::~ActionRead() {}

  std::unordered_set<Trait> const & ActionRead::getTraits() const
  {
    static std::unordered_set<Trait> traits =
    {
      Trait::CanBeSubjectVerbObject,
      Trait::CanBeSubjectVerbDirection
    };

    return traits;
  }

  StateResult ActionRead::doPreBeginWorkNVI(GameState& gameState, AnyMap& params)
  {
    std::string message;
    auto subject = getSubject();
    auto object = getObject();

    if (false) ///< @todo Intelligence tests for reading.
    {
      printMessageTry();
      putTr("READ_TOO_STUPID");
      return StateResult::Failure();
    }

    return StateResult::Success();
  }

  StateResult ActionRead::doBeginWorkNVI(GameState& gameState, AnyMap& params)
  {
    StateResult result = StateResult::Failure();
    std::string message;
    auto subject = getSubject();
    auto object = getObject();

    /// @todo Figure out read time.
    printMessageBegin();
    result = StateResult::Success(1);

    return result;
  }

  StateResult ActionRead::doFinishWorkNVI(GameState& gameState, AnyMap& params)
  {
    StateResult result = StateResult::Failure();
    std::string message;
    auto subject = getSubject();
    auto object = getObject();

    printMessageFinish();

    /// @todo Split read time into start/finish actions.
    if (object->beObjectOf(*this, subject))
    {
      /// @todo Handle destruction on read. (Technically the Lua script should be able to do this.)
      //putTr("THE_FOO_DISINTEGRATES_AFTER_YOU_VERB");
      //object->destroy();

      result = StateResult::Success();
    }
    else
    {
      result = StateResult::Failure();
    }

    return result;
  }

  StateResult ActionRead::doAbortWorkNVI(GameState& gameState, AnyMap& params)
  {
    auto subject = getSubject();
    auto object = getObject();

    printMessageStop();
    return StateResult::Success();
  }

  void ActionRead::printMessageCant() const
  {
    putMsg(makeTr("THE_FOO_HAS_NO_NOUN_TO_VERB", { "writing" }));
  }
} // end namespace
