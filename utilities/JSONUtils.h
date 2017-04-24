#pragma once

#include "AssertHelper.h"
#include "utilities/RNGUtils.h"

#include "json.hpp"
using json = ::nlohmann::json;

namespace JSONUtils
{

  /// Add data in second JSON object to first JSON object.
  inline void addTo(json& first, json& second)
  {
    json flat_first = first.flatten();

    json flat_second = second.flatten();

    for (auto iter = flat_second.cbegin(); iter != flat_second.cend(); ++iter)
    {
      if (flat_first.count(iter.key()) == 0)
      {
        flat_first[iter.key()] = iter.value();
      }
    }

    first = flat_first.unflatten();
  }

  /// Merge second JSON array into first JSON array.
  /// Duplicate values are discarded.
  /// This algorithm has O(n^2) complexity, so it shouldn't be used on
  /// really big arrays.
  inline void mergeArrays(json& first, json& second)
  {
    if (!first.is_array() || !second.is_array()) return;

    for (auto citer2 = second.cbegin(); citer2 != second.cend(); ++citer2)
    {
      bool alreadyExists = false;
      for (auto citer1 = first.cbegin(); citer1 != first.cend(); ++citer1)
      {
        if (citer1.value() == citer2.value())
        {
          alreadyExists = true;
          break;
        }
      }
      if (!alreadyExists)
      {
        first.push_back(citer2.value());
      }
    }
  }

  /// Populate a JSON key if it doesn't already exist.
  /// Returns the contents of the key.
  /// Similar to json::value() except it actually adds the key if it was not
  /// found; value() just returns a default value if it wasn't found, and
  /// does not touch the JSON.
  inline json& addIfMissing(json& j, std::string const& key, json& value)
  {
    if (j.count(key) == 0)
    {
      j[key] = value;
    }

    return j[key];
  }

  /// Perform an action with a key/value pair if the key is present in the JSON.
  inline void doIfPresent(json const& j,
                          std::string key,
                          std::function<void(json const& value)> function)
  {
    if (j.is_object() && j.count(key) != 0)
    {
      json const& value = j[key];
      function(value);
    }
  }

  /// Do a fancypants read of a value to get an integer.
  /// If the value is a number, just set it directly.
  /// If the value is an array, set the integer to a random number between the two.
  /// This function can be expanded to do other stuff later as well.
  inline int getFancyInteger(json const& j)
  {
    int result = 0;

    if (j.is_array())
    {
      if (j.size() >= 2)
      {
        result = pick_uniform(j[0].get<int>(), j[1].get<int>());
      }
      else
      {
        result = j[0].get<int>();
      }
    }
    else if (j.is_object())
    {
      CLOG(WARNING, "Utilities") << "JSON object was passed in, which is unsupported -- returning 0";      
    }
    else
    {
      result = j.get<int>();
    }

    return result;
  }

} // end namespace JSONUtils
