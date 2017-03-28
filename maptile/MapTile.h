#ifndef MAPTILE_H
#define MAPTILE_H

#include "stdafx.h"

#include "types/Direction.h"
#include "types/GameObject.h"
#include "inventory/Inventory.h"
#include "types/LightInfluence.h"
#include "map/MapFactory.h"
#include "Subject.h"
#include "entity/Entity.h"
#include "entity/EntityId.h"

// Forward declarations
class DynamicEntity;
class Floor;
class Metadata;

/// Class representing one tile of the map.
/// @todo Add notifyObservers calls where needed
class MapTile
  :
  public GameObject,
  public Subject
{
  friend class Map;

public:
  virtual ~MapTile();

  /// Get the tile's contents object.
  EntityId getTileContents() const;

  /// Return this tile's description.
  virtual std::string getDisplayName() const override final;

  /// Sets the tile type, without doing gameplay checks.
  /// Used to set up the map before gameplay begins.
  /// @param type Type of the tile.
  /// @return None.
  void setTileType(std::string type);

  /// Gets the current tile type.
  /// @return Type of the tile.
  std::string getTileType() const;

  /// Returns whether a tile is empty space, e.g. no wall in the way.
  bool isEmptySpace() const;

  /// Returns whether a tile can be traversed by a certain DynamicEntity.
  bool canBeTraversedBy(EntityId entity) const;

  /// Set the current tile's location.
  void setCoords(IntVec2 coords);

  /// Get the current tile's location.
  IntVec2 const& getCoords() const;

  /// Get a reference to the map this tile belongs to.
  MapId getMapId() const;

  /// Set the current tile's light level.
  void setAmbientLightLevel(sf::Color level);

  /// Receive light from the specified LightSource.
  /// Gets the Map this tile belongs to and does a recursive
  /// raycasting algorithm on it.
  virtual void beLitBy(EntityId light);

  /// Clear light influences.
  void clearLightInfluences();

  /// Add a light influence to the tile.
  void addLightInfluence(EntityId source,
                         LightInfluence influence);

  /// Get the light shining on a tile.
  /// Syntactic sugar for getWallLightLevel(Direction::Self).
  sf::Color getLightLevel() const;

  /// Get the light shining on a tile wall.
  sf::Color getWallLightLevel(Direction direction) const;

  /// Get the opacity of this tile.
  sf::Color getOpacity() const;

  /// Get whether the tile is opaque or not.
  bool isOpaque() const;

  /// Get the coordinates associated with a tile.
  static RealVec2 getPixelCoords(IntVec2 tile);

  /// Get a reference to an adjacent tile.
  MapTile const & getAdjacentTile(Direction direction) const;

  /// Get a const reference to this tile's metadata.
  Metadata const & getMetadata() const;

protected:
  /// Constructor, callable only by Map class.
  MapTile(IntVec2 coords, Metadata& metadata, MapId map_id);

private:
  static bool initialized;

  /// The ID of the Map this MapTile belongs to.
  MapId m_map_id;

  /// This MapTile's coordinates on the map.
  IntVec2 m_coords;

  /// Pointer to this MapTile's metadata.
  /// This has to be a pointer rather than a reference because it can be
  /// modified after MapTile construction.
  Metadata* m_p_metadata;

  /// Reference to the Entity that represents this tile's contents.
  EntityId m_tile_contents;

  /// Tile's light level.
  /// Levels for the various color channels are interpreted as such:
  /// 0 <= value <= 128: result = (original * (value / 128))
  /// 128 < value <= 255: result = max(original + (value - 128), 255)
  /// The alpha channel is ignored.
  sf::Color m_ambient_light_color;

  /// The calculated light levels of this tile and all of its walls.
  /// Mapping to an int is horribly hacky but I see no other alternative
  /// right now.
  std::map<unsigned int, sf::Color> m_calculated_light_colors;

  /// A map of LightInfluences, representing the amount of light that
  /// each entity is contributing to this map tile.
  /// Levels for the various color channels are interpreted as such:
  /// 0 <= value <= 128: result = (original * (value / 128))
  /// 128 < value <= 255: result = max(original + (value - 128), 255)
  /// The alpha channel is ignored.
  std::map<EntityId, LightInfluence> m_lights;
};

#endif // MAPTILE_H
