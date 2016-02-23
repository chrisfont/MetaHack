#pragma once

#include "Action.h"
#include "ActionCRTP.h"
#include "ThingRef.h"

#include <string>
#include <vector>

class ActionUnlock
  :
  public Action, public ActionCRTP<ActionUnlock>
{
  ACTION_HDR_BOILERPLATE(ActionUnlock)
    ACTION_TRAIT(can_be_subject_verb_object_preposition_target)
    ACTION_TRAIT(can_be_subject_verb_object_preposition_direction)

protected:
  virtual StateResult do_prebegin_work_(AnyMap& params) override;
  virtual StateResult do_begin_work_(AnyMap& params) override;
  virtual StateResult do_finish_work_(AnyMap& params) override;
  virtual StateResult do_abort_work_(AnyMap& params) override;
};