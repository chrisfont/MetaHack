#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

namespace Actions
{
  class ActionAttack
    :
    public Action, public ActionCRTP<ActionAttack>
  {
  private:
    ActionAttack();
  public:
    explicit ActionAttack(EntityId subject);
    virtual ~ActionAttack();
    static ActionAttack prototype;

    //virtual ReasonBool subjectIsCapable(GameState const& gameState) const override;
    //virtual ReasonBool objectIsAllowed(GameState const& gameState) const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState) override;
  };

} // end namespace
