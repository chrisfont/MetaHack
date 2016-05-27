#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "ThingId.h"

class ActionEat
  :
  public Action, public ActionCRTP<ActionEat>
{
  ACTION_HDR_BOILERPLATE(ActionEat)
    ACTION_TRAIT(can_be_subject_verb_object)

public:
  StringDisplay const get_verbed() const override
  {
    return L"ate";
  }

  StringDisplay const get_verb_pp() const override
  {
    return L"eaten";
  }

  StringDisplay const get_verbable() const override
  {
    return L"edible";
  }

protected:
  virtual StateResult do_prebegin_work_(AnyMap& params) override;
  virtual StateResult do_begin_work_(AnyMap& params) override;
  virtual StateResult do_finish_work_(AnyMap& params) override;
  virtual StateResult do_abort_work_(AnyMap& params) override;
};
