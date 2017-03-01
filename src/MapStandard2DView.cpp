#include "stdafx.h"

#include "IConfigSettings.h"
#include "MapStandard2DView.h"
#include "MapTileStandard2DView.h"
#include "New.h"
#include "Service.h"

#include "ShaderEffect.h"

MapStandard2DView::MapStandard2DView(Map& map)
  :
  MapView(map)
{
  reset_cached_render_data();

  // Create a grid of tile views, each tied to a map tile.
  m_map_tile_views.reset(new Grid2D<MapTileStandard2DView>(map.get_size(), 
                                                           [&](Vec2i coords) -> MapTileStandard2DView*
  {
    return NEW MapTileStandard2DView(map.get_tile(coords));
  }));

}

void MapStandard2DView::update_tiles(ThingId viewer)
{
  auto& map = get_map();
  auto& map_size = map.get_size();

  static Vec2f position;

  // Loop through and draw tiles.
  m_map_seen_vertices.clear();
  m_map_memory_vertices.clear();

  for (int y = 0; y < map_size.y; ++y)
  {
    for (int x = 0; x < map_size.x; ++x)
    {
      m_map_tile_views->get({ x, y }).add_tile_vertices(viewer, m_map_seen_vertices, m_map_memory_vertices);
    }
  }
}

void MapStandard2DView::update_things(ThingId viewer, int frame)
{
  /// @todo Move this into MapTileStandard2DView.
  auto& map = get_map();
  auto& map_size = map.get_size();

  // Loop through and draw things.
  m_thing_vertices.clear();
  for (int y = 0; y < map_size.y; ++y)
  {
    for (int x = 0; x < map_size.x; ++x)
    {
      ThingId contents = map.get_tile({ x, y }).get_tile_contents();
      Inventory& inv = contents->get_inventory();

      if (viewer->can_see({ x, y }))
      {
        if (inv.count() > 0)
        {
          // Only draw the largest thing on that tile.
          ThingId biggest_thing = inv.get_largest_thing();
          if (biggest_thing != ThingId::Mu())
          {
            add_thing_floor_vertices(biggest_thing, true, frame);
          }
        }
      }

      // Always draw the viewer itself last, if it is present at that tile.
      if (inv.contains(viewer))
      {
        add_thing_floor_vertices(viewer, true, frame);
      }
    }
  }
}

bool MapStandard2DView::render(sf::RenderTexture& texture, int frame)
{
  the_shader.setParameter("texture", sf::Shader::CurrentTexture);

  sf::RenderStates render_states = sf::RenderStates::Default;
  render_states.shader = &the_shader;
  render_states.texture = &(the_tilesheet.getTexture());

  the_shader.setParameter("effect", ShaderEffect::Lighting);
  //the_shader.setParameter("effect", ShaderEffect::Default);
  texture.draw(m_map_seen_vertices, render_states);
  the_shader.setParameter("effect", ShaderEffect::Sepia);
  texture.draw(m_map_memory_vertices, render_states);
  the_shader.setParameter("effect", ShaderEffect::Lighting);
  //the_shader.setParameter("effect", ShaderEffect::Default);
  texture.draw(m_thing_vertices, render_states);

  return true;
}

void MapStandard2DView::draw_highlight(sf::RenderTarget& target,
                                       Vec2f location,
                                       sf::Color fgColor,
                                       sf::Color bgColor,
                                       int frame)
{
  auto& config = Service<IConfigSettings>::get();
  auto map_tile_size = config.get<float>("map_tile_size");

  float half_ts(map_tile_size * 0.5f);
  Vec2f vSW(location.x - half_ts, location.y + half_ts);
  Vec2f vSE(location.x + half_ts, location.y + half_ts);
  Vec2f vNW(location.x - half_ts, location.y - half_ts);
  Vec2f vNE(location.x + half_ts, location.y - half_ts);

  sf::RectangleShape box_shape;
  Vec2f box_position;
  Vec2f box_size(map_tile_size, map_tile_size);
  Vec2f box_half_size(box_size.x / 2, box_size.y / 2);
  box_position.x = (location.x - box_half_size.x);
  box_position.y = (location.y - box_half_size.y);
  box_shape.setPosition(box_position);
  box_shape.setSize(box_size);
  box_shape.setOutlineColor(fgColor);
  box_shape.setOutlineThickness(config.get<float>("tile_highlight_border_width"));
  box_shape.setFillColor(bgColor);

  target.draw(box_shape);
}


void MapStandard2DView::reset_cached_render_data()
{
  auto& map = get_map();
  auto map_size = map.get_size();

  // Create vertices:
  // 4 vertices * 4 quads for the floor
  // 4 vertices * 4 quads * 4 potential walls
  // = 16 + 64 = 80 possible vertices per tile.
  m_map_seen_vertices.resize(map_size.x * map_size.y * 80);
  m_map_memory_vertices.resize(map_size.x * map_size.y * 80);

  // Create the vertex arrays.
  m_map_seen_vertices.clear();
  m_map_seen_vertices.setPrimitiveType(sf::PrimitiveType::Quads);
  m_map_memory_vertices.clear();
  m_map_memory_vertices.setPrimitiveType(sf::PrimitiveType::Quads);
  m_thing_vertices.clear();
  m_thing_vertices.setPrimitiveType(sf::PrimitiveType::Quads);
}

void MapStandard2DView::add_thing_floor_vertices(ThingId thing,
                                                 bool use_lighting,
                                                 int frame)
{
  auto& config = Service<IConfigSettings>::get();
  sf::Vertex new_vertex;
  float ts = config.get<float>("map_tile_size");
  float ts2 = ts * 0.5f;

  MapTile* root_tile = thing->get_maptile();
  if (!root_tile)
  {
    // Item's root location isn't a MapTile, so it can't be rendered.
    return;
  }

  Vec2i const& coords = root_tile->get_coords();

  sf::Color thing_color;
  if (use_lighting)
  {
    thing_color = root_tile->get_light_level();
  }
  else
  {
    thing_color = sf::Color::White;
  }

  Vec2f location(coords.x * ts, coords.y * ts);
  Vec2f vSW(location.x - ts2, location.y + ts2);
  Vec2f vSE(location.x + ts2, location.y + ts2);
  Vec2f vNW(location.x - ts2, location.y - ts2);
  Vec2f vNE(location.x + ts2, location.y - ts2);
  Vec2u tile_coords = thing->get_tile_sheet_coords(frame);

  the_tilesheet.add_quad(m_thing_vertices,
                        tile_coords, thing_color,
                        vNW, vNE,
                        vSW, vSE);
}

void MapStandard2DView::notifyOfEvent_(Observable & observed, Event event)
{
  /// @todo WRITE ME
}
