#pragma once

#include "stdafx.h"

#include <SFML/Graphics.hpp>

#include "map/MapView.h"

/// Class representing a null view of a Map object.
class MapNullView : public MapView
{
  friend class NullGraphicViews;

public:
  virtual void update_tiles(EntityId viewer, SystemLighting& lighting) override;
  virtual void update_things(EntityId viewer, SystemLighting& lighting, int frame) override;

  virtual bool render_map(sf::RenderTexture& texture, int frame) override;

  virtual void draw_highlight(sf::RenderTarget& target,
                              RealVec2 location,
                              Color fgColor,
                              Color bgColor,
                              int frame) override;

  virtual std::string getViewName() override;

protected:
  /// Constructor.
  /// Private because map views should be obtained via the IGraphicViews service.
  /// @param name   The name of this view.
  /// @param map	Reference to Map object to associate with this view.
  MapNullView(std::string name, Map& map, UintVec2 size);

  virtual bool onEvent_NVI_PreChildren(Event const& event) override;

};
