#pragma once

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

namespace Actions
{
  class ActionDrop
    :
    public Action, public ActionCRTP<ActionDrop>
  {
  private:
    ActionDrop();
  public:
    explicit ActionDrop(EntityId subject);
    virtual ~ActionDrop();
    static ActionDrop prototype;

    virtual ReasonBool subjectIsCapable(GameState const& gameState) const override;
    virtual ReasonBool objectIsAllowed(GameState const& gameState) const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doBeginWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doFinishWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
    virtual StateResult doAbortWorkNVI(GameState& gameState, Systems::Manager& systems, json& arguments) override;
  };

} // end namespace

