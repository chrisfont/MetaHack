#include "tilesheet/TileSheet.h"

#include <boost/filesystem.hpp>

#include "config/Bible.h"
#include "config/Paths.h"
#include "types/common.h"
#include "types/Color.h"
#include "utilities/MathUtils.h"
#include "utilities/StringTransforms.h"

// Namespace aliases
namespace fs = boost::filesystem;

TileSheet::TileSheet(UintVec2 tileSize, UintVec2 textureSize)
  :
  m_textureSize{ textureSize },
  m_tileSize{ tileSize }
{
  if (m_textureSize.x == 0 && m_textureSize.y == 0)
  {
    auto maxSize = m_texture.getMaximumSize();
    m_textureSize.x = maxSize;
    m_textureSize.y = maxSize;
  }

  bool success = m_texture.create(m_textureSize.x, m_textureSize.y);

  Assert("TileSheet", success, "could not create TileSheet texture; now we're sad.");

  UintVec2 textureSizeInTiles = { m_textureSize.x / m_tileSize.x, m_textureSize.y / m_tileSize.y };
  uint32_t usedMapSize = textureSizeInTiles.x * textureSizeInTiles.y;
  m_used.resize(usedMapSize);

  CLOG(TRACE, "TileSheet") << "Created " << m_textureSize.x << " x " << m_textureSize.y << " tilesheet texture";
  CLOG(TRACE, "TileSheet") << "Fits "
    << textureSizeInTiles.x << " x " << textureSizeInTiles.y
    << " tiles of size "
    << m_tileSize.x << " x " << m_tileSize.y;

  m_image.create(m_textureSize.x, m_textureSize.y);
}

TileSheet::~TileSheet()
{
  //dtor
}

UintVec2 TileSheet::loadCollection(FileName const& filename)
{
  if (!m_image.loadFromFile(filename))
  {
    throw std::runtime_error("Tile collection file not found: \"" + filename + "\"");
  }

  m_image.createMaskFromColor(sf::Color(255, 0, 255));

  UintVec2 imageSize = m_image.getSize();

  UintVec2 imageSizeInTiles =
    UintVec2(Math::divideAndRoundUp(imageSize.x, m_tileSize.x),
             Math::divideAndRoundUp(imageSize.y, m_tileSize.y));

  UintVec2 freeSpaceCoords = findUnusedArea(imageSizeInTiles);

  m_texture.update(m_image, freeSpaceCoords.x * m_tileSize.x, freeSpaceCoords.y * m_tileSize.y);

  markTilesUsed(freeSpaceCoords, imageSizeInTiles);

  return freeSpaceCoords;
}

sf::IntRect TileSheet::getTile(UintVec2 tile) const
{
  sf::IntRect rect;
  rect.left = tile.x * m_tileSize.x;
  rect.top = tile.y * m_tileSize.y;
  rect.width = m_tileSize.x;
  rect.height = m_tileSize.y;

#ifdef _DEBUG
  if ((rect.left < 0) || (rect.top < 0) ||
    (rect.left + rect.width >= static_cast<int>(m_texture.getSize().x)) ||
      (rect.top + rect.height >= static_cast<int>(m_texture.getSize().y)))
  {
    CLOG(ERROR, "TileSheet") << "Request for tile (" << tile.x << ", " << tile.y << ") is out of bounded on the sprite sheet!";
  }
#endif // DEBUG

  return rect;
}

sf::Texture& TileSheet::getTexture(void)
{
  return m_texture;
}

sf::Image& TileSheet::getImage(void)
{
  return m_image;
}

void TileSheet::addQuad(sf::VertexArray& vertices,
                         UintVec2 tile_coords, Color bg_color,
                         RealVec2 ul_coord, RealVec2 ur_coord,
                         RealVec2 ll_coord, RealVec2 lr_coord)
{
  sf::Vertex new_vertex;
  RealVec2 ts{ static_cast<float>(m_tileSize.x), static_cast<float>(m_tileSize.y) };
  RealVec2 texNW(tile_coords.x * ts.x, tile_coords.y * ts.y);

  new_vertex.color = bg_color;

  new_vertex.position = ul_coord;
  new_vertex.texCoords = texNW;
  vertices.append(new_vertex);

  new_vertex.position = ur_coord;
  new_vertex.texCoords = RealVec2(texNW.x + ts.x, texNW.y);
  vertices.append(new_vertex);

  new_vertex.position = lr_coord;
  new_vertex.texCoords = RealVec2(texNW.x + ts.x, texNW.y + ts.y);
  vertices.append(new_vertex);

  new_vertex.position = ll_coord;
  new_vertex.texCoords = RealVec2(texNW.x, texNW.y + ts.y);
  vertices.append(new_vertex);
}

/// @todo Implement use of colorOverall.
void TileSheet::addGradientQuadTo(sf::VertexArray& vertices,
                                  UintVec2 tile_coords,
                                  RealVec2 coordNW, RealVec2 coordNE,
                                  RealVec2 coordSW, RealVec2 coordSE,
                                  Color colorOverall,
                                  Color colorNW, Color colorN, Color colorNE,
                                  Color colorW, Color colorC, Color colorE,
                                  Color colorSW, Color colorS, Color colorSE)
{
  RealVec2 ts{ static_cast<float>(m_tileSize.x), static_cast<float>(m_tileSize.y) };
  RealVec2 halfTs{ ts.x / 2.0f, ts.y / 2.0f };

  RealVec2 coordC((coordNW.x + coordNE.x + coordSE.x + coordSW.x) / 4, (coordNW.y + coordNE.y + coordSE.y + coordSW.y) / 4);
  RealVec2 coordN((coordNW.x + coordNE.x) / 2, (coordNW.y + coordNE.y) / 2);
  RealVec2 coordE((coordNE.x + coordSE.x) / 2, (coordNE.y + coordSE.y) / 2);
  RealVec2 coordS((coordSW.x + coordSE.x) / 2, (coordSW.y + coordSE.y) / 2);
  RealVec2 coordW((coordNW.x + coordSW.x) / 2, (coordNW.y + coordSW.y) / 2);

  RealVec2 texNW(tile_coords.x * ts.x, tile_coords.y * ts.y);
  RealVec2 texN(texNW.x + halfTs.x, texNW.y);
  RealVec2 texNE(texNW.x + ts.x, texNW.y);
  RealVec2 texE(texNW.x + ts.x, texNW.y + halfTs.y);
  RealVec2 texSE(texNW.x + ts.x, texNW.y + ts.y);
  RealVec2 texS(texNW.x + halfTs.x, texNW.y + ts.y);
  RealVec2 texSW(texNW.x, texNW.y + ts.y);
  RealVec2 texW(texNW.x, texNW.y + halfTs.y);
  RealVec2 texC(texNW.x + halfTs.x, texNW.y + halfTs.y);

  // Upper left
  vertices.append(sf::Vertex(coordNW, colorNW * colorOverall, texNW));
  vertices.append(sf::Vertex(coordN, colorN * colorOverall, texN));
  vertices.append(sf::Vertex(coordC, colorC * colorOverall, texC));
  vertices.append(sf::Vertex(coordW, colorW * colorOverall, texW));

  // Upper right
  vertices.append(sf::Vertex(coordN, colorN * colorOverall, texN));
  vertices.append(sf::Vertex(coordNE, colorNE * colorOverall, texNE));
  vertices.append(sf::Vertex(coordE, colorE * colorOverall, texE));
  vertices.append(sf::Vertex(coordC, colorC * colorOverall, texC));

  // Lower right
  vertices.append(sf::Vertex(coordC, colorC * colorOverall, texC));
  vertices.append(sf::Vertex(coordE, colorE * colorOverall, texE));
  vertices.append(sf::Vertex(coordSE, colorSE * colorOverall, texSE));
  vertices.append(sf::Vertex(coordS, colorS * colorOverall, texS));

  // Lower left
  vertices.append(sf::Vertex(coordW, colorW * colorOverall, texW));
  vertices.append(sf::Vertex(coordC, colorC * colorOverall, texC));
  vertices.append(sf::Vertex(coordS, colorS * colorOverall, texS));
  vertices.append(sf::Vertex(coordSW, colorSW * colorOverall, texSW));
}

void TileSheet::addOutlineVertices(sf::VertexArray& vertices,
                                   Color bg_color,
                                   RealVec2 ul_coord,
                                   RealVec2 ur_coord,
                                   RealVec2 lr_coord,
                                   RealVec2 ll_coord)
{
  sf::Vertex new_vertex;

  new_vertex.color = bg_color;
  new_vertex.position = ul_coord;
  vertices.append(new_vertex);

  new_vertex.position = ur_coord;
  vertices.append(new_vertex);
  vertices.append(new_vertex);

  new_vertex.position = lr_coord;
  vertices.append(new_vertex);
  vertices.append(new_vertex);

  new_vertex.position = ll_coord;
  vertices.append(new_vertex);
  vertices.append(new_vertex);

  new_vertex.color = bg_color;
  new_vertex.position = ul_coord;
  vertices.append(new_vertex);
}

/// === PROTECTED METHODS =====================================================

unsigned int TileSheet::getIndex(UintVec2 coords)
{
  UintVec2 textureSizeInTiles = { m_textureSize.x / m_tileSize.x, m_textureSize.y / m_tileSize.y };
  return (coords.y * textureSizeInTiles.x) + coords.x;
}

bool TileSheet::areaIsUnused(UintVec2 start, UintVec2 size)
{
  UintVec2 textureSizeInTiles = { m_textureSize.x / m_tileSize.x, m_textureSize.y / m_tileSize.y };

  if (((start.x + size.x) > textureSizeInTiles.x) ||
    ((start.y + size.y) > textureSizeInTiles.y))
  {
    return false;
  }

  for (unsigned int y = start.y; y < start.y + size.y; ++y)
  {
    for (unsigned int x = start.x; x < start.x + size.x; ++x)
    {
      if (m_used[getIndex({ x, y })])
      {
        return false;
      }
    }
  }
  return true;
}

UintVec2 TileSheet::findUnusedArea(UintVec2 size)
{
  UintVec2 start(0, 0);

  UintVec2 textureSizeInTiles = { m_textureSize.x / m_tileSize.x, m_textureSize.y / m_tileSize.y };

  while (start.y < textureSizeInTiles.y)
  {
    while (start.x < textureSizeInTiles.x)
    {
      if (areaIsUnused(start, size))
      {
        return start;
      }
      ++start.x;
    }
    start.x = 0;
    ++start.y;
  }

  /// If we got here, there's no free space.
  throw std::out_of_range("Out of space on tile sheet");
}

/// Mark a rectangle of tiles as being used.
/// @param upperLeft Upper-left corner of rectangle.
/// @param size      Size of the rectangle to mark.
/// @todo This is an extremely naive algorithm and can definitely be optimized.
void TileSheet::markTilesUsed(UintVec2 upperLeft, UintVec2 size)
{
  for (uint32_t y = upperLeft.y; y < upperLeft.y + size.y; ++y)
  {
    for (uint32_t x = upperLeft.x; x < upperLeft.x + size.x; ++x)
    {
      m_used[getIndex({ x, y })] = true;
    }
  }
}

bool TileSheet::hasTilesFor(std::string category)
{
  if (needToLoadFilesFor(category))
  {
    loadViewResourcesFor(category);
  }

  return (m_tileCoords.count(category) != 0);
}

bool TileSheet::needToLoadFilesFor(std::string category)
{
  return (m_triedToLoad.count(category) == 0);
}

UintVec2 const& TileSheet::getTileSheetCoords(std::string category)
{
  if (needToLoadFilesFor(category))
  {
    loadViewResourcesFor(category);
  }

  Assert("TileSheet", m_tileCoords.count(category) != 0,
         "getTileSheetCoords(\"" << category << "\") called, but requested category has no tiles");

  return m_tileCoords.at(category);
}

void TileSheet::loadViewResourcesFor(std::string category)
{
  StringPair stringPair = StringTransforms::splitName(category);

  FileName resourceString = Config::paths().resources() + "/entity/" + stringPair.second;
  FileName pngFileString = resourceString + ".png";
  fs::path pngFilePath = fs::path(pngFileString);
  m_triedToLoad.insert(category);

  if (fs::exists(pngFilePath))
  {
    UintVec2 tileLocation;
    CLOG(TRACE, "TileSheet") << "Tiles were found for " << category << " category";

    tileLocation = loadCollection(pngFileString);
    CLOG(TRACE, "TileSheet") << "Tiles for " << category <<
      " were placed on the TileSheet at " << tileLocation;

    m_tileCoords[category] = tileLocation;
  }
  else
  {
    CLOG(TRACE, "TileSheet") << "No tiles found for " << category << ", checking templates";

    // No graphics for this category, so try to fall back upon templates, one at a time.
    auto& categoryData = Config::bible().categoryData(category);

    // First we actually need templates for this to work...
    if (categoryData.count("templates") != 0)
    {
      auto& templatesList = categoryData["templates"];
      // And the templates list must be an array...
      if (templatesList.is_array())
      {
        // For each template in the list, see if there are associated graphics.
        for (auto templateName : templatesList)
        {
          if (templateName.is_string())
          {
            auto templateNameStr = "template." + templateName.get<std::string>();
            if (hasTilesFor(templateNameStr))
            {
              CLOG(TRACE, "TileSheet") << "..." << category << " will use tiles from " << templateNameStr;
              m_tileCoords[category] = m_tileCoords[templateNameStr];
              return;
            }
          }
        } // end for loop
      }
    }

    CLOG(TRACE, "TileSheet") << "... no tiles found after searching entire template tree";

    /// @note No way to actually "erase" a tile from a tilesheet right now, so
    ///       this would end up leaving an unusable "gap" in the sheet. But
    ///       the only way this code should be called is if the PNG file were
    ///       erased *during* program run, and no fallback graphics were found
    ///       when scanning the template tree, *and* the views were reset. This
    ///       is *super* unlikely.
    m_tileCoords.erase(category);
  }
}
