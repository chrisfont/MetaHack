#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

namespace Actions
{
  class ActionDie
    :
    public Action, public ActionCRTP<ActionDie>
  {
  private:
    ActionDie();
  public:
    explicit ActionDie(EntityId subject);
    virtual ~ActionDie();
    static ActionDie prototype;

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