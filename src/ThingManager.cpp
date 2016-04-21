#include "stdafx.h"

#include "ThingManager.h"

#include "App.h"
#include "ErrorHandler.h"
#include "GameState.h"
#include "LuaObject.h"
#include "LuaThingFunctions.h"
#include "Metadata.h"
#include "Thing.h"
#include "ThingId.h"

ThingManager::ThingManager()
{
  // Set up the logger.
  SET_UP_LOGGER("Thing", true);

  // Register the Thing Lua functions.
  LuaThingFunctions::register_functions();

  // Create the "nothingness" object.
  ThingId mu = create("Mu");
  if (mu != 0ULL)
  {
    FATAL_ERROR("Mu's ID is %" PRIu64 " instead of zero!", static_cast<uint64_t>(mu));
  }

  m_initialized = true;
}

ThingManager::~ThingManager()
{
}

bool ThingManager::first_is_subtype_of_second(StringKey first, StringKey second)
{
  StringKey first_parent = MDC::get_collection("thing").get(first).get_intrinsic<StringKey>("parent");

  if (first_parent.empty())
  {
    return false;
  }

  if (first_parent == second)
  {
    return true;
  }

  return first_is_subtype_of_second(first_parent, second);
}

ThingId ThingManager::create(StringKey type)
{
  ThingId new_id = ThingId(m_nextThingId);
  ++m_nextThingId;
  Metadata& metadata = MDC::get_collection("thing").get(type);

  std::unique_ptr<Thing> new_thing{ new Thing{ metadata, new_id } };
  m_thing_map[new_id] = std::move(new_thing);

  if (m_initialized)
  {
    m_thing_map[new_id]->call_lua_function<ActionResult>("on_create", {}, ActionResult::Success);
  }

  return ThingId(new_id);
}

ThingId ThingManager::create_tile_contents(MapTile* map_tile)
{
  ThingId new_id = ThingId(m_nextThingId);
  ++m_nextThingId;
  Metadata& metadata = MDC::get_collection("thing").get("TileContents");

  std::unique_ptr<Thing> new_thing{ new Thing { map_tile, metadata, new_id } };
  m_thing_map[new_id] = std::move(new_thing);

  return ThingId(new_id);
}

ThingId ThingManager::clone(ThingId original)
{
  if (this->exists(original) == false) return get_mu();
  Thing& original_thing = this->get(original);

  ThingId new_id = ThingId(m_nextThingId);
  ++m_nextThingId;

  std::unique_ptr<Thing> new_thing{ new Thing { original_thing, new_id } };
  m_thing_map[new_id] = std::move(new_thing);

  return ThingId(new_id);
}

void ThingManager::destroy(ThingId id)
{
  if (id != get_mu())
  {
    if (m_thing_map.count(id) != 0)
    {
      m_thing_map.erase(id);
    }
  }
  else
  {
    throw std::exception("Attempted to destroy Mu object!");
  }
}

bool ThingManager::exists(ThingId id)
{
  return (m_thing_map.count(id) != 0);
}

Thing& ThingManager::get(ThingId id)
{
  try
  {
    return *(m_thing_map.at(id).get());
  }
  catch (std::out_of_range&)
  {
    CLOG(WARNING, "Thing") << "Tried to get thing " << id << " which does not exist";
    return *(m_thing_map[get_mu()].get());
  }
}

Thing const& ThingManager::get(ThingId id) const
{
  try
  {
    return *(m_thing_map.at(id).get());
  }
  catch (std::out_of_range&)
  {
    CLOG(WARNING, "Thing") << "Tried to get thing " << id << " which does not exist";
    return *(m_thing_map.at(get_mu()).get());
  }
}

ThingId ThingManager::get_mu() const
{
  return ThingId();
}