#pragma once

#include "game/App.h"
#include "entity/EntityId.h"
#include "types/Direction.h"

// Forward declarations
class GameState;

namespace Systems
{
  class Manager;
}

#include "json.hpp"
using json = ::nlohmann::json;

namespace Actions
{
  // === FORWARD DECLARATIONS ===================================================
  class Action;
  class Entity;

  // === USING DECLARATIONS =====================================================
  using ActionCreator = std::function<std::unique_ptr<Action>(EntityId)>;
  using ActionMap = std::unordered_map<std::string, ActionCreator>;
  
  /// Struct containing the results of an action state.
  struct StateResult
  {
    static StateResult Success(unsigned int time = 0)
    {
      return{ true, time };
    }

    static StateResult Failure(unsigned int time = 0)
    {
      return{ false, time };
    }

    bool success;
    unsigned int elapsed_time;
  };

  /// Enum indicating the state of a particular action.
  enum class State
  {
    Pending,      ///< The action is waiting to be executed.
    PreBegin,     ///< Wait state prior to starting the action, to account for reaction time.
    InProgress,   ///< The action has been started and is currently in progress.
    Interrupted,  ///< The action was interrupted while in process and will be aborted.
    PostFinish,   ///< The action is finished and the entity is now in a recovery wait state.
    Processed     ///< The action is totally done and can be popped off the queue.
  };

  /// Possible action traits.
  enum class Trait
  {
    CanBeSubjectOnly,
    CanBeSubjectVerbObject,
    CanBeSubjectVerbBodypart,
    CanBeSubjectVerbDirection,
    CanBeSubjectVerbObjects,
    CanBeSubjectVerbObjectPrepositionTarget,
    CanBeSubjectVerbObjectsPrepositionTarget,
    CanBeSubjectVerbObjectPrepositionBodypart,
    CanBeSubjectVerbObjectPrepositionDirection,
    CanTakeAQuantity,
    SubjectCanBeInLimbo,
    SubjectCanNotBeInsideAnotherObject,
    SubjectMustBeAbleToMove,
    ObjectMustBeWielded,
    ObjectMustBeWorn,
    ObjectMustNotBeWielded,
    ObjectMustNotBeWorn,
    ObjectCanBeOutOfReach,
    ObjectCanBeSelf,
    ObjectMustBeEmpty,
    ObjectMustBeInInventory,
    ObjectMustNotBeEmpty,
    ObjectMustNotBeInInventory,
    ObjectMustBeLiquidCarrier,
    Last,
    First = CanBeSubjectOnly
  };

  /// Struct that contains a true/false value and a reason string.
  /// If the value is false, the string should contain the reason why.
  struct ReasonBool
  {
    bool value;
    std::string reason;
  };

  /// Class describing an action to execute.
  class Action
  {
  public:
    Action(EntityId subject, std::string type);
    virtual ~Action();

    json toJson();

    virtual std::unordered_set<Trait> const& getTraits() const;

    bool hasTrait(Trait trait) const;

    EntityId getSubject() const;

    void setObject(EntityId object);
    void setObjects(std::vector<EntityId> objects);

    std::vector<EntityId> const& getObjects() const;
    EntityId getObject() const;
    EntityId getSecondObject() const;

    bool process(GameState& gameState, Systems::Manager& systems);

    void setState(State state);
    State getState();

    void setTarget(EntityId entity);
    void setTarget(Direction direction);
    void setQuantity(unsigned int quantity);

    EntityId getTargetThing() const;
    Direction getTargetDirection() const;
    unsigned int getQuantity() const;

    std::string getType() const;

    /// A static method that registers an action subclass in a database.
    /// Required so that Lua scripts can instantiate Actions on Entities.
    static void registerActionAs(std::string key, ActionCreator creator);

    /// A static method that checks if a key exists.
    static bool exists(std::string key);

    /// A static method that returns an Action associated with a key.
    /// If the requested key does not exist, throws an exception.
    static std::unique_ptr<Action> create(std::string key, 
                                          EntityId subject,
                                          std::vector<EntityId> objects = std::vector<EntityId>(),
                                          EntityId targetThing = EntityId::Void,
                                          Direction targetDirection = Direction::None,
                                          unsigned int quantity = 0);

    /// Get a const reference to the action map.
    static ActionMap const& getMap();

  protected:
    /// An Action without a subject; used for prototype registration only.
    Action(std::string type, ActionCreator creator);

    /// Create the JSON object used for constructing messages.
    json makeJSONArgumentsObject() const;

    /// Perform work to be done at the start of the PreBegin state.
    /// This work typically consists of checking whether the action is possible.
    /// If StateResult::success is false, the action was not possible and is cancelled.
    /// If StateResult::success is true, the action is possible, and the target actor
    /// is busy for StateResult::elapsed_time before moving to the InProgress state.
    /// @note doPreBeginWork usually has multiple returns in order to avoid
    ///       the Dreaded Pyramid of Doom. It's just easier that way.
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating whether the Action continues.
    StateResult doPreBeginWork(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Perform work to be done at the start of the InProgress state.
    /// This is where the action begins.
    /// If StateResult::success is false, the action could not begin and is cancelled.
    /// (However, the PreBegin reaction time has already elapsed, so this is more
    ///  like a critical failure when attempting the action, unlike a failure in
    ///   doPreBeginWork which just means the action couldn't be started at all.)
    /// If StateResult::success is true, the action began, and the target actor is
    /// busy for StateResult::elapsed_time while the action is performed. It then
    /// moves to the PostFinish state.
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating whether the Action continues.
    StateResult doBeginWork(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Perform work to be done at the end of the InProgress state and the start
    /// of the PostFinish state.
    /// This is where the action ends, and is used for actions that are not
    /// instantaneous in game time, such as eating, drinking, reading, etc.
    /// StateResult::success is ignored.
    /// The target actor is busy for StateResult::elapsed_time as a "refractory"
    /// period.
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating the post-Action wait time.
    StateResult doFinishWork(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Perform work to be done when an action in the InProgress state is aborted.
    /// This is called when an action is aborted.
    /// StateResult::success is ignored.
    /// The target actor is busy for StateResult::elapsed_time as a "refractory"
    /// period.
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating the post-Action wait time.
    StateResult doAbortWork(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Check if this action can be performed at all by the subject.
    /// Called as part of doPreBeginWork, before the overridable portion is called.
    /// Default implementation returns false and logs a warning about a missing
    /// override of the function.
    virtual ReasonBool subjectIsCapable(GameState const& gameState) const;

    /// Check if this action can be performed right now by the subject.
    /// Called as part of doPreBeginWork, before the overridable portion is called.
    /// Default implementation just calls `subjectIsCapable()`.
    virtual ReasonBool subjectIsCapableNow(GameState const& gameState) const;

    /// Check if this action can be performed at all on the object.
    /// Called as part of doPreBeginWork, before the overridable portion is called.
    /// Default implementation returns false and logs a warning about a missing
    /// override of the function.
    virtual ReasonBool objectIsAllowed(GameState const& gameState) const;

    /// Check if this action can be performed right now on the object.
    /// Called as part of doPreBeginWork, before the overridable portion is called.
    /// Default implementation just calls `objectIsAllowed()`.
    virtual ReasonBool objectIsAllowedNow(GameState const& gameState) const;

    /// Overridable portion of doPreBeginWork().
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating whether the Action continues.
    virtual StateResult doPreBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Overridable portion of doBeginWork().
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating whether the Action continues.
    virtual StateResult doBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Overridable portion of doFinishWork().
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating the post-Action wait time.
    virtual StateResult doFinishWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Overridable portion of doAbortWork().
    /// @param params Map of parameters for the Action.
    /// @return StateResult indicating the post-Action wait time.
    virtual StateResult doAbortWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments);

    /// Print a "[SUBJECT] try to [VERB]" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// "Try to" will be conjugated for the subject as "tries to" if needed.
    ///
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageTry(Systems::Manager& systems, json& arguments) const;

    /// Print a "[SUBJECT] [VERB]" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// VERB will be conjugated for the subject as needed.
    ///
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageDo(Systems::Manager& systems, json& arguments) const;

    /// Print a "[SUBJECT] begin to [VERB]" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// "Begin to" will be conjugated for the subject as "begins to" if needed.
    ///
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageBegin(Systems::Manager& systems, json& arguments) const;

    /// Print a "[SUBJECT] stop [VERBING]" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// "Stop" will be conjugated for the subject as "stops" if needed.
    ///
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageStop(Systems::Manager& systems, json& arguments) const;

    /// Print a "[SUBJECT] finish [VERBING]" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// "Finish" will be conjugated for the subject as "finishes" if needed.
    ///
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageFinish(Systems::Manager& systems, json& arguments) const;

    /// Print a "[SUBJECT] can't [VERB] that!" message.
    /// The message will vary based on the presence of objects or a direction
    /// for the action, using get_object_string().
    /// "Finish" will be conjugated for the subject as "finishes" if needed.
    ///
    /// @todo Finish implementing me, right now the default implementation is too simple.
    /// This method can be overridden if necessary to customize the message for a
    /// particular action.
    virtual void printMessageCant(Systems::Manager& systems, json& arguments) const;

  private:
    /// State of this action.
    State m_state;

    /// The subject performing the action.
    EntityId m_subject;

    /// The objects of the action.
    std::vector<EntityId> m_objects;

    /// Target Entity for the action (if any).
    EntityId m_targetThing;

    /// Direction for the action (if any).
    Direction m_targetDirection;

    /// Quantity for the action (only used in drop/pickup).
    unsigned int m_quantity;

    /// This action's type name.
    std::string const m_type;

    static ActionMap s_actionMap;
  };

  inline std::ostream& operator<<(std::ostream& os, State const& s)
  {
    switch (s)
    {
      case State::Pending:      os << "Pending"; break;
      case State::PreBegin:     os << "PreBegin"; break;
      case State::InProgress:   os << "InProgress"; break;
      case State::Interrupted:  os << "Interrupted"; break;
      case State::PostFinish:   os << "PostFinish"; break;
      case State::Processed:    os << "Processed"; break;
      default:                  os << "??? (" << static_cast<unsigned int>(s) << ")"; break;
    }

    return os;
  }
} // end namespace

/* Header template for subclasses: 

namespace Actions
{
  class ActionXXXX
  :
  public Action, public ActionCRTP<ActionXXXX>
  {
  private:
    ActionXXXX();
  public:
    explicit ActionXXXX(EntityId subject);
    virtual ~ActionXXXX();
    static ActionXXXX prototype;

    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
  };

} // end namespace
*/

/* Source boilerplate to copy to subclasses: 

ActionXXXX ActionXXXX::prototype;
ActionXXXX::ActionXXXX() : Action("xxxx", "XXXX", ActionXXXX::create_) {}
ActionXXXX::ActionXXXX(EntityId subject) : Action(subject, "xxxx", "XXXX") {}
ActionXXXX::~ActionXXXX() {}

std::unordered_set<Trait> const & ActionXXXX::getTraits() const
{
  static std::unordered_set<Trait> traits =
  {
    Trait::XXXX,
    Trait::YYYY
  };

  return traits;
}

*/