#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "entity/EntityId.h"

/// Action class for taking an item out of another item.
/// Why does this item have the "object can be out of reach" trait? Well, the
/// object is inside a container, so it is technically "out of reach" as far
/// as the game engine is concerned, until it is removed from the container.
/// We DO check if the CONTAINER is within reach, which is what really matters.
namespace Actions
{
  class ActionTakeOut
    :
    public Action, public ActionCRTP<ActionTakeOut>
  {
  private:
    ActionTakeOut();
  public:
    explicit ActionTakeOut(EntityId subject);
    virtual ~ActionTakeOut();
    static ActionTakeOut prototype;

    virtual ReasonBool subjectIsCapable() const override;
    //virtual ReasonBool objectIsAllowed() const override;
    virtual std::unordered_set<Trait> const& getTraits() const override;

  protected:
    virtual StateResult doPreBeginWorkNVI(AnyMap& params) override;
    virtual StateResult doBeginWorkNVI(AnyMap& params) override;
    virtual StateResult doFinishWorkNVI(AnyMap& params) override;
    virtual StateResult doAbortWorkNVI(AnyMap& params) override;

    virtual void printMessageTry() const override;
    virtual void printMessageDo() const override;
  };

} // end namespace