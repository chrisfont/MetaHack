#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

namespace Actions
{
  class ActionOpen
    :
    public Action, public ActionCRTP<ActionOpen>
  {
  private:
    ActionOpen();
  public:
    explicit ActionOpen(EntityId subject);
    virtual ~ActionOpen();
    static ActionOpen prototype;

    virtual ReasonBool subjectIsCapable(GameState const& gameState) const override;
    //virtual ReasonBool objectIsAllowed(GameState const& gameState) const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState) override;
  };
} // end namespace