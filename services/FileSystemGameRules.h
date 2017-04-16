#pragma once

#include "stdafx.h"

#include "services/IGameRules.h"

/// Class that encapsulates all static data in the game.
///
class FileSystemGameRules : public IGameRules
{
public:
  FileSystemGameRules();
  virtual ~FileSystemGameRules();

  /// Get data for a specific Entity category.
  /// If it doesn't exist, attempt to load it.
  json& category(std::string name);

  /// Get reference to game rules data.
  inline json& data()
  {
    return m_data;
  }

  /// Get const reference to game rules data.
  inline json const& data() const
  {
    return m_data;
  }

protected:
  /// Attempt to load JSON data for an entity category.
  /// Also runs any associated Lua script.
  void loadCategory(std::string name);

private:
  /// Game rules data, as stored in a JSON object.
  json m_data;

};