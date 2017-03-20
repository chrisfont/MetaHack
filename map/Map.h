#ifndef MAP_H
#define MAP_H

#include "stdafx.h"

#include "Subject.h"
#include "map/MapFactory.h"
#include "types/IRenderable.h"
#include "entity/Entity.h"

// Forward declarations
class GameState;
class MapFeature;
class MapGenerator;
class MapTile;
class EntityId;

// VS compatibility
#ifdef WIN32   //WINDOWS
#define constexpr const
#endif

/// Class representing a map, which is a grid of locations for Entities.
class Map
  :
  public Subject
{
  friend class MapFactory;
  friend class MapView;

public:
  ~Map();

  /// The maximum length of one side.
  static constexpr int max_dimension = 128;

  /// The maximum area of a map.
  static constexpr int max_area = max_dimension * max_dimension;

  /// The default ambient light level.
  static const sf::Color ambient_light_level;

  /// Serialization function.
  template<class Archive>
  void serialize(Archive& archive)
  {
    archive(pImpl);
  }

  /// Process all Entities on this map.
  void process();

  void update_lighting();

  void add_light(EntityId source);

  MapTile const & get_tile(IntVec2 tile) const;

  MapTile& get_tile(IntVec2 tile);

  bool tile_is_opaque(IntVec2 tile);

  /// Get the map's size.
  IntVec2 const& getSize() const;

  /// Get player's starting location.
  IntVec2 const& get_start_coords() const;

  /// Set player's starting location.
  bool set_start_coords(IntVec2 start_coords);

  /// Get the index of a particular X/Y coordinate.
  int get_index(IntVec2 coords) const;

  /// Get whether a particular X/Y coordinate is in bounds.
  bool is_in_bounds(IntVec2 coords) const;

  /// Calculate coordinates corresponding to a direction, if possible.
  bool calc_coords(IntVec2 origin,
                   Direction direction,
                   IntVec2& result);

  /// Get Map ID.
  MapId get_map_id() const;

  /// @todo Not sure all the "feature" stuff should be public.
  ///       But not sure how to scope it better either.

  /// Clear the collection of map features.
  void clear_map_features();

  /// Get the collection of map features.
  boost::ptr_deque<MapFeature> const& get_map_features() const;

  /// Get a random map feature from the deque.
  MapFeature& get_random_map_feature();

  /// Add a map feature to this map.
  /// The feature must be dynamically generated, and the map takes ownership
  /// of it when it is passed in.
  MapFeature& add_map_feature(MapFeature* feature);

protected:
  Map(GameState& game, MapId mapId, int width, int height);

  /// Initialize a new Map.
  /// This unfortunately has to be separated from the constructor due to
  /// the fact that Lua scripts refer to Maps by ID. This means the Map
  /// must be inserted into the container of Maps before any Lua scripts
  /// can refer to it.
  void initialize();

  /// Recursively calculates lighting from the origin.
  /// Octant is an integer representing the following:
  /// \ 1|2 /  |
  ///  \ | /   |
  /// 8 \|/ 3  |
  /// ---+---  |
  /// 7 /|\ 4  |
  ///  / | \   |
  /// / 6|5 \  |

  void do_recursive_lighting(EntityId source,
                             IntVec2 const& origin,
                             sf::Color const& light_color,
                             int const max_depth_squared,
                             int octant,
                             int depth = 1,
                             float slope_A = 1,
                             float slope_B = 0);

private:
  /// Reference to game state.
  GameState& m_game;

  /// Map ID.
  MapId m_map_id;

  /// Map size.
  IntVec2 m_map_size;

  std::unique_ptr<MapGenerator> m_generator;

  struct Impl;
  std::unique_ptr<Impl> pImpl;

  /// Lua function to get the tile contents Entity at a specific location.
  /// Takes three parameters:
  ///   - The MapID of the map in question.
  ///   - x, y location of the tile contents to retrieve
  /// It returns:
  ///   - ID of the requested Entity, or nil if it does not exist.
  /// Notes:
  ///   - The Map that the tile is retrieved from is the one the player is on.
  static int LUA_get_tile_contents(lua_State* L);

  /// Lua function to get the start coords for a map.
  /// Takes one parameter:
  ///   - The MapID of the map in question.
  /// It returns:
  ///   - The x, y location of the map's starting position.
  static int LUA_get_start_coords(lua_State* L);

  /// Lua function to add a feature to a map.
  /// Takes two parameters:
  ///   - The MapID of the map in question.
  ///   - A string indicating the type of feature to add.
  /// It returns:
  ///   - A boolean indicating whether the feature could be added to the map.
  static int LUA_map_add_feature(lua_State* L);
};

#endif // MAP_H