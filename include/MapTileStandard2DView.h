#pragma once

#include "stdafx.h"

#include <SFML/Graphics.hpp>

#include "MapTileView.h"

#include "EntityStandard2DView.h"

// Forward declarations

/// Class representing the standard 2D (pseudo-3D) view of a MapTile object.
class MapTileStandard2DView : public MapTileView<EntityStandard2DView>
{
  friend class MapStandard2DView;

public:
  /// Constructor.
  /// @param map	Reference to Map object to associate with this view.
  MapTileStandard2DView(MapTile& map_tile);

  //virtual bool render(sf::RenderTexture& texture, int frame) override;

protected:

  /// Return the coordinates of the tile on the tilesheet.
  Vec2u get_tile_sheet_coords() const;


  /// Add the vertices for the maptile to the seen and memory vertices.
  /// @param viewer Entity that is viewing this tile.
  /// @param seen_vertices Array to add seen vertices to.
  /// @param memory_vertices Array to add memory vertices to.
  void add_tile_vertices(EntityId viewer, 
                         sf::VertexArray& seen_vertices,
                         sf::VertexArray& memory_vertices);

  /// Add the vertices from the viewer's memory.
  /// @param vertices Array to add vertices to.
  /// @param viewer Entity that is remembering this tile.
  void add_memory_vertices_to(sf::VertexArray & vertices,
                              EntityId viewer);

  /// Add the floor vertices for the maptile to a VertexArray to be drawn.
  /// @param vertices Array to add vertices to.
  void add_tile_floor_vertices(sf::VertexArray& vertices);

  /// Add the floor vertices for the things on this tile to a VertexArray to be drawn.
  void add_things_floor_vertices(EntityId viewer,
                                 sf::VertexArray& vertices, 
                                 bool use_lighting, int frame);

  /// Add the floor vertices for the thing specified.
  /// @todo Move into a Entity view.
  void add_thing_floor_vertices(EntityId thing, 
                                sf::VertexArray& vertices, 
                                bool use_lighting, int frame);

  /// Add this MapTile's walls to a VertexArray to be drawn.
  /// @param vertices Array to add vertices to.
  /// @param use_lighting If true, calculate lighting when adding.
  ///                     If false, store directly w/white bg color.
  /// @param nw_is_empty  Whether tile to the northwest is empty.
  /// @param n_is_empty   Whether tile to the north is empty.
  /// @param ne_is_empty  Whether tile to the northeast is empty.
  /// @param e_is_empty   Whether tile to the east is empty.
  /// @param se_is_empty  Whether tile to the southeast is empty.
  /// @param s_is_empty   Whether tile to the south is empty.
  /// @param sw_is_empty  Whether tile to the southwest is empty.
  /// @param w_is_empty   Whether tile to the west is empty.
  void add_wall_vertices_to(sf::VertexArray& vertices,
                            bool use_lighting,
                            bool nw_is_empty, bool n_is_empty,
                            bool ne_is_empty, bool e_is_empty,
                            bool se_is_empty, bool s_is_empty,
                            bool sw_is_empty, bool w_is_empty);

  virtual void notifyOfEvent_(Observable& observed, Event event) override;


private:
  /// Random tile offset.
  int m_tile_offset;

};