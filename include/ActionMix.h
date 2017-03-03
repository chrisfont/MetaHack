#pragma once

#include "stdafx.h"

#include "Action.h"
#include "ActionCRTP.h"
#include "EntityId.h"

class ActionMix
  :
  public Action, public ActionCRTP<ActionMix>
{
  ACTION_HDR_BOILERPLATE(ActionMix)
    ACTION_TRAIT(can_be_subject_verb_objects)

public:
  std::string const get_verbable() const override
  {
    return "miscable";
  }

protected:
  virtual StateResult do_prebegin_work_(AnyMap& params) override;
  virtual StateResult do_begin_work_(AnyMap& params) override;
  virtual StateResult do_finish_work_(AnyMap& params) override;
  virtual StateResult do_abort_work_(AnyMap& params) override;
};
