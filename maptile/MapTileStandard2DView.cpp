#include "stdafx.h"

#include "maptile/MapTileStandard2DView.h"
#include "Service.h"
#include "services/IConfigSettings.h"
#include "tilesheet/TileSheet.h"
#include "types/ShaderEffect.h"
#include "utilities/RNGUtils.h"

std::string MapTileStandard2DView::getViewName()
{
  return "standard2D";
}

MapTileStandard2DView::MapTileStandard2DView(MapTile& map_tile, TileSheet& tile_sheet)
  :
  MapTileView(map_tile),
  m_tile_offset{ pick_uniform(0, 4) },
  m_tile_sheet{ tile_sheet }
{
}

UintVec2 MapTileStandard2DView::get_tile_sheet_coords() const
{
  /// @todo Deal with selecting one of the other tiles.
  UintVec2 start_coords = get_map_tile().getMetadata().get_tile_coords();
  UintVec2 tile_coords(start_coords.x + m_tile_offset, start_coords.y);
  return tile_coords;
}

/// @todo Instead of repeating this method in EntityStandard2DView, call the view from here.
UintVec2 MapTileStandard2DView::get_entity_tile_sheet_coords(Entity& entity, int frame) const
{
  /// Get tile coordinates on the sheet.
  UintVec2 start_coords = entity.getMetadata().get_tile_coords();

  /// Call the Lua function to get the offset (tile to choose).
  UintVec2 offset = entity.call_lua_function("get_tile_offset", { Property::from(frame) }).as<UintVec2>();

  /// Add them to get the resulting coordinates.
  UintVec2 tile_coords = start_coords + offset;

  return tile_coords;
}
void MapTileStandard2DView::add_tile_vertices(EntityId viewer,
                                              sf::VertexArray& seen_vertices,
                                              sf::VertexArray& memory_vertices)
{
  auto& tile = get_map_tile();
  auto coords = tile.getCoords();
  auto& x = coords.x;
  auto& y = coords.y;

  bool this_is_empty = tile.isEmptySpace();
  bool nw_is_empty = (viewer->canSee({ x - 1, y - 1 }) && tile.getAdjacentTile(Direction::Northwest).isEmptySpace());
  bool n_is_empty = (viewer->canSee({ x, y - 1 }) && tile.getAdjacentTile(Direction::North).isEmptySpace());
  bool ne_is_empty = (viewer->canSee({ x + 1, y - 1 }) && tile.getAdjacentTile(Direction::Northeast).isEmptySpace());
  bool e_is_empty = (viewer->canSee({ x + 1, y }) && tile.getAdjacentTile(Direction::East).isEmptySpace());
  bool se_is_empty = (viewer->canSee({ x + 1, y + 1 }) && tile.getAdjacentTile(Direction::Southeast).isEmptySpace());
  bool s_is_empty = (viewer->canSee({ x, y + 1 }) && tile.getAdjacentTile(Direction::South).isEmptySpace());
  bool sw_is_empty = (viewer->canSee({ x - 1, y + 1 }) && tile.getAdjacentTile(Direction::Southwest).isEmptySpace());
  bool w_is_empty = (viewer->canSee({ x - 1, y }) && tile.getAdjacentTile(Direction::West).isEmptySpace());

  if (viewer->canSee(coords))
  {
    if (this_is_empty)
    {
      add_tile_floor_vertices(seen_vertices);
    }
    else
    {
      add_wall_vertices_to(seen_vertices, true,
                           nw_is_empty, n_is_empty,
                           ne_is_empty, e_is_empty,
                           se_is_empty, s_is_empty,
                           sw_is_empty, w_is_empty);
    }
  }
  else
  {
    add_memory_vertices_to(memory_vertices, viewer);
  }
}

void MapTileStandard2DView::add_memory_vertices_to(sf::VertexArray& vertices,
                                                   EntityId viewer)
{
  auto& config = Service<IConfigSettings>::get();
  auto& tile = get_map_tile();
  auto coords = tile.getCoords();

  MapId map_id = viewer->getMapId();
  if (map_id == MapFactory::null_map_id)
  {
    return;
  }
  Map& game_map = GAME.getMaps().get(map_id);

  static sf::Vertex new_vertex;
  float ts = config.get("map_tile_size").as<float>();
  float ts2 = ts * 0.5f;

  RealVec2 location(coords.x * ts, coords.y * ts);
  RealVec2 vSW(location.x - ts2, location.y + ts2);
  RealVec2 vSE(location.x + ts2, location.y + ts2);
  RealVec2 vNW(location.x - ts2, location.y - ts2);
  RealVec2 vNE(location.x + ts2, location.y - ts2);

  std::string tile_type = viewer->getMemoryAt(coords).getType();
  if (tile_type == "") { tile_type = "MTUnknown"; }
  Metadata* tile_metadata = &(GAME.getMetadataCollection("maptile").get(tile_type));

  /// @todo Call a script to handle selecting a tile other than the one
  ///       in the upper-left corner.
  UintVec2 tile_coords = tile_metadata->get_tile_coords();

  m_tile_sheet.add_quad(vertices,
                        tile_coords, sf::Color::White,
                        vNW, vNE,
                        vSW, vSE);
}

void MapTileStandard2DView::add_tile_floor_vertices(sf::VertexArray& vertices)
{
  auto& config = Service<IConfigSettings>::get();

  auto& tile = get_map_tile();
  auto& coords = tile.getCoords();
  auto& tileN = tile.getAdjacentTile(Direction::North);
  auto& tileNE = tile.getAdjacentTile(Direction::Northeast);
  auto& tileE = tile.getAdjacentTile(Direction::East);
  auto& tileSE = tile.getAdjacentTile(Direction::Southeast);
  auto& tileS = tile.getAdjacentTile(Direction::South);
  auto& tileSW = tile.getAdjacentTile(Direction::Southwest);
  auto& tileW = tile.getAdjacentTile(Direction::West);
  auto& tileNW = tile.getAdjacentTile(Direction::Northwest);

  sf::Vertex new_vertex;
  float ts = config.get("map_tile_size").as<float>();
  float half_ts = ts * 0.5f;

  sf::Color colorN{ tileN.getLightLevel() };
  sf::Color colorNE{ tileNE.getLightLevel() };
  sf::Color colorE{ tileE.getLightLevel() };
  sf::Color colorSE{ tileSE.getLightLevel() };
  sf::Color colorS{ tileS.getLightLevel() };
  sf::Color colorSW{ tileSW.getLightLevel() };
  sf::Color colorW{ tileW.getLightLevel() };
  sf::Color colorNW{ tileNW.getLightLevel() };

  sf::Color light = tile.getLightLevel();
  sf::Color lightN = average(light, colorN);
  sf::Color lightNE = average(light, colorN, colorNE, colorE);
  sf::Color lightE = average(light, colorE);
  sf::Color lightSE = average(light, colorE, colorSE, colorS);
  sf::Color lightS = average(light, colorS);
  sf::Color lightSW = average(light, colorS, colorSW, colorW);
  sf::Color lightW = average(light, colorW);
  sf::Color lightNW = average(light, colorW, colorNW, colorN);

  RealVec2 location{ coords.x * ts, coords.y * ts };
  RealVec2 vNE{ location.x + half_ts, location.y - half_ts };
  RealVec2 vSE{ location.x + half_ts, location.y + half_ts };
  RealVec2 vSW{ location.x - half_ts, location.y + half_ts };
  RealVec2 vNW{ location.x - half_ts, location.y - half_ts };

  UintVec2 tile_coords = get_tile_sheet_coords();

  m_tile_sheet.add_gradient_quad(vertices, tile_coords,
                                 vNW, vNE,
                                 vSW, vSE,
                                 lightNW, lightN, lightNE,
                                 lightW, light, lightE,
                                 lightSW, lightS, lightSE);
}

void MapTileStandard2DView::add_things_floor_vertices(EntityId viewer,
                                                      sf::VertexArray & vertices,
                                                      bool use_lighting, int frame)
{
  auto& tile = get_map_tile();
  auto coords = tile.getCoords();
  EntityId contents = tile.getTileContents();
  Inventory& inv = contents->getInventory();

  if (viewer->canSee(coords))
  {
    if (inv.count() > 0)
    {
      // Only draw the largest entity on that tile.
      EntityId biggest_thing = inv.get_largest_thing();
      if (biggest_thing != EntityId::Mu())
      {
        add_thing_floor_vertices(biggest_thing, vertices, use_lighting, frame);
      }
    }
  }

  // Always draw the viewer itself last, if it is present at that tile.
  if (inv.contains(viewer))
  {
    add_thing_floor_vertices(viewer, vertices, use_lighting, frame);
  }
}

void MapTileStandard2DView::add_thing_floor_vertices(EntityId entityId, sf::VertexArray & vertices, bool use_lighting, int frame)
{
  auto& entity = GAME.getEntities().get(entityId);
  auto& config = Service<IConfigSettings>::get();
  sf::Vertex new_vertex;
  float ts = config.get("map_tile_size").as<float>();
  float ts2 = ts * 0.5f;

  MapTile* root_tile = entityId->getMapTile();
  if (!root_tile)
  {
    // Item's root location isn't a MapTile, so it can't be rendered.
    return;
  }

  IntVec2 const& coords = root_tile->getCoords();

  sf::Color thing_color;
  if (use_lighting)
  {
    thing_color = root_tile->getLightLevel();
  }
  else
  {
    thing_color = sf::Color::White;
  }

  RealVec2 location(coords.x * ts, coords.y * ts);
  RealVec2 vSW(location.x - ts2, location.y + ts2);
  RealVec2 vSE(location.x + ts2, location.y + ts2);
  RealVec2 vNW(location.x - ts2, location.y - ts2);
  RealVec2 vNE(location.x + ts2, location.y - ts2);
  UintVec2 tile_coords = get_entity_tile_sheet_coords(entity, frame);

  m_tile_sheet.add_quad(vertices,
                        tile_coords, thing_color,
                        vNW, vNE,
                        vSW, vSE);
}


void MapTileStandard2DView::add_wall_vertices_to(sf::VertexArray& vertices,
                                                 bool use_lighting,
                                                 bool nw_is_empty, bool n_is_empty,
                                                 bool ne_is_empty, bool e_is_empty,
                                                 bool se_is_empty, bool s_is_empty,
                                                 bool sw_is_empty, bool w_is_empty)
{
  auto& config = Service<IConfigSettings>::get();
  auto map_tile_size = config.get("map_tile_size").as<float>();

  // This tile.
  MapTile& tile = get_map_tile();

  // Checks to see N/S/E/W walls.
  bool player_sees_n_wall{ false };
  bool player_sees_s_wall{ false };
  bool player_sees_e_wall{ false };
  bool player_sees_w_wall{ false };

  // Player.
  EntityId player = GAME.getPlayer();

  if (player != EntityId::Mu())
  {
    auto player_tile = player->getMapTile();
    auto player_coords = player_tile->getCoords();
    auto tile_coords = tile.getCoords();
    if (player_tile != nullptr)
    {
      player_sees_n_wall = (player_coords.y <= tile_coords.y);
      player_sees_s_wall = (player_coords.y >= tile_coords.y);
      player_sees_e_wall = (player_coords.x >= tile_coords.x);
      player_sees_w_wall = (player_coords.x <= tile_coords.x);
    }
  }

  // Tile color.
  sf::Color tile_color{ sf::Color::White };

  // Wall colors.
  sf::Color wall_n_color_w{ sf::Color::White };
  sf::Color wall_n_color{ sf::Color::White };
  sf::Color wall_n_color_e{ sf::Color::White };

  sf::Color wall_e_color_n{ sf::Color::White };
  sf::Color wall_e_color{ sf::Color::White };
  sf::Color wall_e_color_s{ sf::Color::White };

  sf::Color wall_s_color_w{ sf::Color::White };
  sf::Color wall_s_color{ sf::Color::White };
  sf::Color wall_s_color_e{ sf::Color::White };

  sf::Color wall_w_color_n{ sf::Color::White };
  sf::Color wall_w_color{ sf::Color::White };
  sf::Color wall_w_color_s{ sf::Color::White };

  // Full tile size.
  float ts(map_tile_size);

  // Half of the tile size.
  float half_ts(map_tile_size * 0.5f);

  // Wall size (configurable).
  float ws(map_tile_size * 0.4f);

  // Adjacent tiles
  MapTile const& adjacent_tile_n = tile.getAdjacentTile(Direction::North);
  MapTile const& adjacent_tile_e = tile.getAdjacentTile(Direction::East);
  MapTile const& adjacent_tile_s = tile.getAdjacentTile(Direction::South);
  MapTile const& adjacent_tile_w = tile.getAdjacentTile(Direction::West);

  // Tile vertices.
  auto tile_coords = tile.getCoords();
  RealVec2 location(tile_coords.x * ts,
                 tile_coords.y * ts);
  RealVec2 vTileN(location.x, location.y - half_ts);
  RealVec2 vTileNE(location.x + half_ts, location.y - half_ts);
  RealVec2 vTileE(location.x + half_ts, location.y);
  RealVec2 vTileSE(location.x + half_ts, location.y + half_ts);
  RealVec2 vTileS(location.x, location.y + half_ts);
  RealVec2 vTileSW(location.x - half_ts, location.y + half_ts);
  RealVec2 vTileW(location.x - half_ts, location.y);
  RealVec2 vTileNW(location.x - half_ts, location.y - half_ts);

  UintVec2 tile_sheet_coords = this->get_tile_sheet_coords();

  if (use_lighting)
  {
    tile_color = tile.getLightLevel();

    if (player_sees_n_wall)
    {
      wall_n_color = tile.getWallLightLevel(Direction::North);
      wall_n_color_w = average(wall_n_color, adjacent_tile_w.getWallLightLevel(Direction::North));
      wall_n_color_e = average(wall_n_color, adjacent_tile_e.getWallLightLevel(Direction::North));
    }
    else
    {
      wall_n_color = sf::Color::Black;
      wall_n_color_w = sf::Color::Black;
      wall_n_color_e = sf::Color::Black;
    }

    if (player_sees_e_wall)
    {
      wall_e_color = tile.getWallLightLevel(Direction::East);
      wall_e_color_n = average(wall_e_color, adjacent_tile_n.getWallLightLevel(Direction::East));
      wall_e_color_s = average(wall_e_color, adjacent_tile_s.getWallLightLevel(Direction::East));
    }
    else
    {
      wall_e_color = sf::Color::Black;
      wall_e_color_n = sf::Color::Black;
      wall_e_color_s = sf::Color::Black;
    }

    if (player_sees_s_wall)
    {
      wall_s_color = tile.getWallLightLevel(Direction::South);
      wall_s_color_w = average(wall_s_color, adjacent_tile_w.getWallLightLevel(Direction::South));
      wall_s_color_e = average(wall_s_color, adjacent_tile_e.getWallLightLevel(Direction::South));
    }
    else
    {
      wall_s_color = sf::Color::Black;
      wall_s_color_w = sf::Color::Black;
      wall_s_color_e = sf::Color::Black;
    }

    if (player_sees_w_wall)
    {
      wall_w_color = tile.getWallLightLevel(Direction::West);
      wall_w_color_n = average(wall_w_color, adjacent_tile_n.getWallLightLevel(Direction::West));
      wall_w_color_s = average(wall_w_color, adjacent_tile_s.getWallLightLevel(Direction::West));
    }
    else
    {
      wall_w_color = sf::Color::Black;
      wall_w_color_n = sf::Color::Black;
      wall_w_color_s = sf::Color::Black;
    }
  }

  // NORTH WALL
  if (n_is_empty)
  {
    RealVec2 vS(vTileN.x, vTileN.y + ws);
    RealVec2 vSW(vTileNW.x, vTileNW.y + ws);
    if (w_is_empty)
    {
      vSW.x += ws;
    }
    else if (!nw_is_empty)
    {
      vSW.x -= ws;
    }
    RealVec2 vSE(vTileNE.x, vTileNE.y + ws);
    if (e_is_empty)
    {
      vSE.x -= ws;
    }
    else if (!ne_is_empty)
    {
      vSE.x += ws;
    }

    m_tile_sheet.add_gradient_quad(vertices, tile_sheet_coords,
                                   vTileNW, vTileNE,
                                   vSW, vSE,
                                   wall_n_color_w, wall_n_color, wall_n_color_e,
                                   wall_n_color_w, wall_n_color, wall_n_color_e,
                                   wall_n_color_w, wall_n_color, wall_n_color_e);
  }

  // EAST WALL
  if (e_is_empty)
  {
    RealVec2 vW(vTileE.x - ws, vTileE.y);
    RealVec2 vNW(vTileNE.x - ws, vTileNE.y);
    if (n_is_empty)
    {
      vNW.y += ws;
    }
    else if (!ne_is_empty)
    {
      vNW.y -= ws;
    }
    RealVec2 vSW(vTileSE.x - ws, vTileSE.y);
    if (s_is_empty)
    {
      vSW.y -= ws;
    }
    else if (!se_is_empty)
    {
      vSW.y += ws;
    }

    m_tile_sheet.add_gradient_quad(vertices, tile_sheet_coords,
                                   vNW, vTileNE,
                                   vSW, vTileSE,
                                   wall_e_color_n, wall_e_color_n, wall_e_color_n,
                                   wall_e_color, wall_e_color, wall_e_color,
                                   wall_e_color_s, wall_e_color_s, wall_e_color_s);
  }

  // SOUTH WALL
  if (s_is_empty)
  {
    RealVec2 vN(vTileS.x, vTileS.y - ws);
    RealVec2 vNW(vTileSW.x, vTileSW.y - ws);
    if (w_is_empty)
    {
      vNW.x += ws;
    }
    else if (!sw_is_empty)
    {
      vNW.x -= ws;
    }
    RealVec2 vNE(vTileSE.x, vTileSE.y - ws);
    if (e_is_empty)
    {
      vNE.x -= ws;
    }
    else if (!se_is_empty)
    {
      vNE.x += ws;
    }

    m_tile_sheet.add_gradient_quad(vertices, tile_sheet_coords,
                                   vNW, vNE,
                                   vTileSW, vTileSE,
                                   wall_s_color_w, wall_s_color, wall_s_color_e,
                                   wall_s_color_w, wall_s_color, wall_s_color_e,
                                   wall_s_color_w, wall_s_color, wall_s_color_e);
  }

  // WEST WALL
  if (w_is_empty)
  {
    RealVec2 vE(vTileW.x + ws, vTileW.y);
    RealVec2 vNE(vTileNW.x + ws, vTileNW.y);
    if (n_is_empty)
    {
      vNE.y += ws;
    }
    else if (!nw_is_empty)
    {
      vNE.y -= ws;
    }
    RealVec2 vSE(vTileSW.x + ws, vTileSW.y);
    if (s_is_empty)
    {
      vSE.y -= ws;
    }
    else if (!sw_is_empty)
    {
      vSE.y += ws;
    }

    m_tile_sheet.add_gradient_quad(vertices, tile_sheet_coords,
                                   vTileNW, vNE,
                                   vTileSW, vSE,
                                   wall_w_color_n, wall_w_color_n, wall_w_color_n,
                                   wall_w_color, wall_w_color, wall_w_color,
                                   wall_w_color_s, wall_w_color_s, wall_w_color_s);
  }
}

EventResult MapTileStandard2DView::onEvent_NVI(Event const & event)
{
  return{ EventHandled::Yes, ContinueBroadcasting::Yes };
}

