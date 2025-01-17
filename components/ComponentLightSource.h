#pragma once

#include "json.hpp"
using json = ::nlohmann::json;

#include "types/Color.h"

namespace Components
{

  /// Component that defines a light-emitting source.
  /// @todo Should there be more than one of these allowed for a single Entity?
  class ComponentLightSource
  {
  public:

    friend void from_json(json const& j, ComponentLightSource& obj);
    friend void to_json(json& j, ComponentLightSource const& obj);

    bool& lit();
    bool const& lit() const;

    Color& color();
    Color const& color() const;

    int& strength();
    int const& strength() const;

  protected:

  private:
    bool m_lit = false;
    Color m_lightColor = Color(64, 64, 64);
    int m_lightStrength = 64;

  };

} // end namespace Components