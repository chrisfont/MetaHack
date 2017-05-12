#pragma once

#include <boost/ptr_container/ptr_deque.hpp>
#include <deque>

#include "actions/Action.h"
#include "actions/ActionQueue.h"

#include "json.hpp"
using json = ::nlohmann::json;

/// Component that represents an entity's ability to reason.
class ComponentSapience
{
public:

  friend void from_json(json const& j, ComponentSapience& obj);
  friend void to_json(json& j, ComponentSapience const& obj);

  Actions::ActionQueue& pendingActions();
  Actions::ActionQueue const& pendingActions() const;

protected:

private:
  /// Queue of pending voluntary actions to be performed.
  Actions::ActionQueue m_pendingActions;

};
