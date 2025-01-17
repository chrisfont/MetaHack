#pragma once

#include "maptile/MapTile.h"

// Forward declarations
class EntityId;

/// Abstract class representing a view of a MapTile object.
/// Does NOT represent the Thing objects that might be on the tile.
class MapTileView
  :
  public Object
{
public:
  /// Destructor.
  virtual ~MapTileView() {}

  virtual std::string getViewName() = 0;

protected:
  /// Constructor.
  /// @param map	Reference to MapTile object to associate with this view.
  explicit MapTileView(MapTile& map_tile)
    :
    Object({}),
    m_map_tile(map_tile)
  {
    //startObserving(map_tile);
  }

  /// Get reference to MapTile associated with this view.
  MapTile& getMapTile()
  {
    return m_map_tile;
  }

  MapTile const& getMapTile() const
  {
    return m_map_tile;
  }

private:
  /// MapTile associated with this view.
  MapTile& m_map_tile;
};
