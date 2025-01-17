#include "stdafx.h"

#include "map/MapDiamond.h"

#include "entity/EntityFactory.h"
#include "game/App.h"
#include "maptile/MapTile.h"
#include "utilities/RNGUtils.h"

MapDiamond::MapDiamond(Map& m, PropertyDictionary const& s, GeoVector vec)
  : MapFeature{ m, s, vec }
{
  unsigned int numTries = 0;
  unsigned int minHsDist = s.get("min_half_size", 2);
  unsigned int maxHsDist = s.get("max_half_size", 4);
  unsigned int max_retries = s.get("max_retries", 100);
  std::string floorMaterial = s.get("floor_type", "Dirt");
  std::string wallMaterial = s.get("wall_type", "Stone");

  IntVec2& startingCoords = vec.start_point;
  Direction& direction = vec.direction;

  while (numTries < max_retries)
  {
    int diamondHalfSize = the_RNG.pick_uniform(minHsDist, maxHsDist);

    int xCenter, yCenter;

    if (direction == Direction::North)
    {
      xCenter = startingCoords.x;
      yCenter = startingCoords.y - (diamondHalfSize + 1);
    }
    else if (direction == Direction::South)
    {
      xCenter = startingCoords.x;
      yCenter = startingCoords.y + (diamondHalfSize + 1);
    }
    else if (direction == Direction::West)
    {
      xCenter = startingCoords.x - (diamondHalfSize + 1);
      yCenter = startingCoords.y;
    }
    else if (direction == Direction::East)
    {
      xCenter = startingCoords.x + (diamondHalfSize + 1);
      yCenter = startingCoords.y;
    }
    else if (direction == Direction::Self)
    {
      xCenter = startingCoords.x;
      yCenter = startingCoords.y;
    }
    else
    {
      throw MapFeatureException("Invalid direction passed to MapDiamond constructor");
    }

    if ((getMap().isInBounds({ xCenter - (diamondHalfSize + 1), yCenter - (diamondHalfSize + 1) })) &&
        (getMap().isInBounds({ xCenter + (diamondHalfSize + 1), yCenter + (diamondHalfSize + 1) })))
    {
      bool okay = true;

      // Verify that box and surrounding area are solid walls.
      /// @todo: Constrain this to only check around the edges of the
      ///        diamond, instead of the entire enclosing box.

      okay = doesBoxPassCriterion({ xCenter - (diamondHalfSize + 1), yCenter - (diamondHalfSize + 1) },
      { xCenter + (diamondHalfSize + 1), yCenter + (diamondHalfSize + 1) },
                                     [&](MapTile& tile) { return !tile.isPassable(); });

      if (okay)
      {
        // Clear out a diamond.
        for (int xCounter = -diamondHalfSize;
             xCounter <= diamondHalfSize;
             ++xCounter)
        {
          for (int yCounter = -(diamondHalfSize - abs(xCounter));
               yCounter <= diamondHalfSize - abs(xCounter);
               ++yCounter)
          {
            int xCoord = xCenter + xCounter;
            int yCoord = yCenter + yCounter;
            auto& tile = getMap().getTile({ xCoord, yCoord });
            tile.setTileType({ "Floor", floorMaterial }, { "OpenSpace" });
          }
        }

        setCoords(sf::IntRect(xCenter - diamondHalfSize,
                               yCenter - diamondHalfSize,
                               (diamondHalfSize * 2) + 1,
                               (diamondHalfSize * 2) + 1));

        // Add the four points as potential connection points.
        addGrowthVector(GeoVector(xCenter, yCenter - (diamondHalfSize + 1),
                                    Direction::North));
        addGrowthVector(GeoVector(xCenter, yCenter + (diamondHalfSize + 1),
                                    Direction::South));
        addGrowthVector(GeoVector(xCenter - (diamondHalfSize + 1), yCenter,
                                    Direction::West));
        addGrowthVector(GeoVector(xCenter + (diamondHalfSize + 1), yCenter,
                                    Direction::East));

        /// @todo Put either a door or an open area at the starting coords.
        ///       Right now we just make it an open area.
        auto& startTile = getMap().getTile(startingCoords);
        startTile.setTileType({ "Floor", floorMaterial }, { "OpenSpace" });

        return;
      }
    }

    ++numTries;
  }

  throw MapFeatureException("Out of tries attempting to make MapDiamond");
}
