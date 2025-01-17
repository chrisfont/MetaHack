#include "components/ComponentPhysical.h"

#include "components/ComponentManager.h"
#include "game/GameState.h"
#include "utilities/JSONUtils.h"

namespace Components
{

  void from_json(json const& j, ComponentPhysical& obj)
  {
    obj = ComponentPhysical();
    JSONUtils::doIfPresent(j, "mass", [&](auto& value) { obj.m_mass = value; });
    JSONUtils::doIfPresent(j, "volume", [&](auto& value) { obj.m_volume = value; });
  }

  void to_json(json& j, ComponentPhysical const& obj)
  {
    j = json::object();
    j["mass"] = obj.m_mass;
    j["volume"] = obj.m_volume;
  }

  int& ComponentPhysical::mass()
  {
    return m_mass;
  }

  int const& ComponentPhysical::mass() const
  {
    return m_mass;
  }

  int& ComponentPhysical::volume()
  {
    return m_volume;
  }

  int const& ComponentPhysical::volume() const
  {
    return m_volume;
  }

} // end namespace
