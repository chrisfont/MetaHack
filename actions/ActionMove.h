#pragma once

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

#include <string>
#include <vector>

namespace Actions
{
  class ActionMove
    :
    public Action, public ActionCRTP<ActionMove>
  {
  private:
    ActionMove();
  public:
    explicit ActionMove(EntityId subject);
    virtual ~ActionMove();
    static ActionMove prototype;

    virtual ReasonBool subjectIsCapable(GameState& gameState) const override;
    //virtual ReasonBool objectIsAllowed(GameState& gameState) const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState, AnyMap& params) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState, AnyMap& params) override;
  };

} // end namespace