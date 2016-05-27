#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "ThingId.h"

class ActionAttire
  :
  public Action, public ActionCRTP<ActionAttire>
{
  ACTION_HDR_BOILERPLATE(ActionAttire)
    ACTION_TRAIT(can_be_subject_verb_object)
    ACTION_TRAIT(object_must_be_in_inventory)

public:
  StringDisplay const get_verbed() const override
  {
    return L"wore";
  }

  StringDisplay const get_verb_pp() const override
  {
    return L"worn";
  }

protected:
  virtual StateResult do_prebegin_work_(AnyMap& params) override;
  virtual StateResult do_begin_work_(AnyMap& params) override;
  virtual StateResult do_finish_work_(AnyMap& params) override;
  virtual StateResult do_abort_work_(AnyMap& params) override;
};
