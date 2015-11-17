#ifndef MAPROOM_H
#define MAPROOM_H

#include <SFML/Graphics.hpp>

#include "MapFeature.h"

// Forward declarations
class PropertyDictionary;

class MapRoom : public MapFeature
{
  public:
    MapRoom(Map& m, PropertyDictionary const& settings);
    virtual ~MapRoom();

    /// Create a rectangular room of random size adjacent to starting
    /// coordinates in the direction indicated.
    /// Checks to make sure the box does not intersect any existing features.
    /// Attempts to create the box max_retries times before giving up.
    virtual bool create(GeoVector vec) override;

  protected:
  private:
    static unsigned int max_width;
    static unsigned int min_width;
    static unsigned int max_height;
    static unsigned int min_height;
    static unsigned int max_retries;
};

#endif // MAPROOM_H
