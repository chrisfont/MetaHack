#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "EntityId.h"

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

    virtual std::unordered_set<Action::Trait> const& getTraits() const override;

  protected:
    virtual StateResult do_prebegin_work_(AnyMap& params) override;
    virtual StateResult do_begin_work_(AnyMap& params) override;
    virtual StateResult do_finish_work_(AnyMap& params) override;
    virtual StateResult do_abort_work_(AnyMap& params) override;
  };

} // end namespace
