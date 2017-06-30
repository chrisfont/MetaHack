#include "stdafx.h"

#include "map/MapRoom.h"

#include "entity/EntityFactory.h"
#include "game/App.h"
#include "maptile/MapTile.h"
#include "utilities/MathUtils.h"
#include "utilities/RNGUtils.h"

MapRoom::MapRoom(Map& m, PropertyDictionary const& s, GeoVector vec)
  :
  MapFeature{ m, s, vec }
{
  unsigned int num_tries = 0;

  UniformIntDist width_dist(s.get("min_width", 2),
                              s.get("max_width", 15));
  UniformIntDist height_dist(s.get("min_height", 2),
                               s.get("max_height", 15));
  unsigned int max_retries = s.get("max_retries", 100);
  std::string floorMaterial = s.get("floor_type", "Dirt");
  std::string wallMaterial = s.get("wall_type", "Stone");

  IntVec2& starting_coords = vec.start_point;
  Direction& direction = vec.direction;

  while (num_tries < max_retries)
  {
    sf::IntRect rect;

    rect.width = width_dist(the_RNG);
    rect.height = height_dist(the_RNG);

    if (direction == Direction::North)
    {
      UniformIntDist offset_dist(0, rect.width - 1);
      int offset = offset_dist(the_RNG);

      rect.top = starting_coords.y - rect.height;
      rect.left = starting_coords.x - offset;
    }
    else if (direction == Direction::South)
    {
      UniformIntDist offset_dist(0, rect.width - 1);
      int offset = offset_dist(the_RNG);

      rect.top = starting_coords.y + 1;
      rect.left = starting_coords.x - offset;
    }
    else if (direction == Direction::West)
    {
      UniformIntDist offset_dist(0, rect.height - 1);
      int offset = offset_dist(the_RNG);

      rect.top = starting_coords.y - offset;
      rect.left = starting_coords.x - rect.width;
    }
    else if (direction == Direction::East)
    {
      UniformIntDist offset_dist(0, rect.height - 1);
      int offset = offset_dist(the_RNG);

      rect.top = starting_coords.y - offset;
      rect.left = starting_coords.x + 1;
    }
    else if (direction == Direction::Self)
    {
      UniformIntDist height_offset_dist(0, rect.height - 1);
      UniformIntDist width_offset_dist(0, rect.width - 1);

      rect.top = starting_coords.y - height_offset_dist(the_RNG);
      rect.left = starting_coords.x - width_offset_dist(the_RNG);
    }
    else
    {
      throw MapFeatureException("Invalid direction passed to MapRoom constructor");
    }

    if ((getMap().isInBounds({ rect.left - 1, rect.top - 1 })) &&
        (getMap().isInBounds({ rect.left + rect.width, rect.top + rect.height })))
    {
      bool okay = true;

      // Verify that box and surrounding area are solid walls.
      okay = doesBoxPassCriterion({ rect.left - 1, rect.top - 1 },
      { rect.left + rect.width, rect.top + rect.height },
                                     [&](MapTile& tile) { return !tile.isPassable(); });

      if (okay)
      {
        // Clear out the box.
        /// @todo Handle wall material
        setBox(rect, { "Floor", floorMaterial }, { "OpenSpace" });
        setCoords(rect);

        // Add the surrounding walls as potential connection points.

        // Horizontal walls...
        for (int x_coord = rect.left + 1;
             x_coord <= rect.left + rect.width - 1;
             ++x_coord)
        {
          addGrowthVector(GeoVector(x_coord, rect.top - 1, Direction::North));
          addGrowthVector(GeoVector(x_coord, rect.top + rect.height, Direction::South));
        }
        // Vertical walls...
        for (int y_coord = rect.top + 1;
             y_coord <= rect.top + rect.height - 1;
             ++y_coord)
        {
          addGrowthVector(GeoVector(rect.left - 1, y_coord, Direction::West));
          addGrowthVector(GeoVector(rect.left + rect.width, y_coord, Direction::East));
        }

        /// @todo Put either a door or an open area at the starting coords.
        ///       Right now we just make it an open area.
        auto& startTile = getMap().getTile(starting_coords);
        startTile.setTileType({ "Floor", floorMaterial }, { "OpenSpace" });

        return;
      }
    }

    ++num_tries;
  }

  throw MapFeatureException("Out of tries attempting to make MapLRoom");
}