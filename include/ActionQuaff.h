#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "ThingId.h"

class ActionQuaff
  :
  public Action, public ActionCRTP<ActionQuaff>
{
  ACTION_HDR_BOILERPLATE(ActionQuaff)
    ACTION_TRAIT(can_be_subject_verb_object)
    ACTION_TRAIT(can_be_subject_verb_direction)

public:
  StringDisplay const get_verbed() const override
  {
    return L"drank from";
  }

  StringDisplay const get_verbing() const override
  {
    return L"drinking from";
  }

  StringDisplay const get_verbable() const override
  {
    return L"potable";
  }

protected:
  virtual StateResult do_prebegin_work_(AnyMap& params) override;
  virtual StateResult do_begin_work_(AnyMap& params) override;
  virtual StateResult do_finish_work_(AnyMap& params) override;
  virtual StateResult do_abort_work_(AnyMap& params) override;
};
