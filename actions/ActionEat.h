#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

namespace Actions
{
  class ActionEat
    :
    public Action, public ActionCRTP<ActionEat>
  {
  private:
    ActionEat();
  public:
    explicit ActionEat(EntityId subject);
    virtual ~ActionEat();
    static ActionEat prototype;

    //virtual ReasonBool subjectIsCapable(GameState& gameState) const override;
    //virtual ReasonBool objectIsAllowed(GameState& gameState) const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState, AnyMap& params) override;

  private:
    /// The status of the beObjectOf() call, which needs to be saved
    /// for the "finish" portion of the action.
    bool m_last_eat_result;
  };

} // end namespace