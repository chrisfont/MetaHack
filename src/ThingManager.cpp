#include "ThingManager.h"

#include "App.h"
#include "ErrorHandler.h"
#include "GameState.h"
#include "Lua.h"
#include "Metadata.h"
#include "Thing.h"
#include "ThingRef.h"

#include <boost/algorithm/string.hpp>
#include <boost/bimap.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cinttypes>
#include <queue>
#include <string>

ThingManager::ThingManager(GameState& game_state)
  :
  m_game_state{ game_state }
{
  // Register the Thing Lua functions.
  the_lua_instance.register_function("thing_create", Thing::LUA_thing_create);
  the_lua_instance.register_function("thing_destroy", Thing::LUA_thing_create);
  the_lua_instance.register_function("thing_get_player", Thing::LUA_thing_get_player);
  the_lua_instance.register_function("thing_get_coords", Thing::LUA_thing_get_coords);
  the_lua_instance.register_function("thing_get_type", Thing::LUA_thing_get_type);
  the_lua_instance.register_function("thing_get_intrinsic_flag", Thing::LUA_thing_get_intrinsic_flag);
  the_lua_instance.register_function("thing_get_intrinsic_value", Thing::LUA_thing_get_intrinsic_value);
  the_lua_instance.register_function("thing_get_intrinsic_string", Thing::LUA_thing_get_intrinsic_string);
  the_lua_instance.register_function("thing_get_property_flag", Thing::LUA_thing_get_property_flag);
  the_lua_instance.register_function("thing_get_property_value", Thing::LUA_thing_get_property_value);
  the_lua_instance.register_function("thing_get_property_string", Thing::LUA_thing_get_property_string);
  the_lua_instance.register_function("thing_set_property_flag", Thing::LUA_thing_set_property_flag);
  the_lua_instance.register_function("thing_set_property_value", Thing::LUA_thing_set_property_value);
  the_lua_instance.register_function("thing_set_property_string", Thing::LUA_thing_set_property_string);
  the_lua_instance.register_function("thing_move_into", Thing::LUA_thing_move_into);

  // Create the "nothingness" object.
  ThingRef mu = create("Mu");
  if (mu.get_id().to_uint64() != 0)
  {
    FATAL_ERROR("Mu's ID is %" PRIu64 " instead of zero!", mu.get_id().to_uint64());
  }
}

ThingManager::~ThingManager()
{
}

ThingRef ThingManager::create(std::string type)
{
  ThingId new_id = ThingRef::create();
  ThingRef new_ref = ThingRef(new_id);
  Metadata& metadata = MDC::get_collection("thing").get(type);

  Thing* new_thing = m_thing_pool.construct(boost::ref(metadata), new_ref);
  m_thing_map[new_id] = new_thing;

  // Temporary test of Lua call
  new_thing->call_lua_function("on_create", {});

  return ThingRef(new_id);
}

ThingRef ThingManager::create_tile_contents(MapTile* map_tile)
{
  ThingId new_id = ThingRef::create();
  ThingRef new_ref = ThingRef(new_id);
  Metadata& metadata = MDC::get_collection("thing").get("TileContents");

  Thing* new_thing = m_thing_pool.construct(map_tile, boost::ref(metadata), new_ref);
  m_thing_map[new_id] = new_thing;

  return ThingRef(new_id);
}

ThingRef ThingManager::clone(ThingRef original_ref)
{
  if (this->exists(original_ref) == false) return get_mu();
  Thing* original_thing = this->get_ptr(original_ref.m_id);

  ThingId new_id = ThingRef::create();
  ThingRef new_ref = ThingRef(new_id);

  Thing* new_thing = m_thing_pool.construct(*original_thing, new_ref);
  m_thing_map[new_id] = new_thing;

  return ThingRef(new_id);
}

void ThingManager::destroy(ThingRef ref)
{
  if (ref != get_mu())
  {
    if (m_thing_map.count(ref.m_id) != 0)
    {
      Thing* old_thing = m_thing_map[ref.m_id];
      m_thing_pool.destroy(old_thing);
      m_thing_map.erase(ref.m_id);
    }
  }
  else
  {
    throw std::exception("Attempted to destroy Mu object!");
  }
}

bool ThingManager::exists(ThingRef ref)
{
  return (m_thing_map.count(ref.m_id) != 0);
}

Thing* ThingManager::get_ptr(ThingId id)
{
  try
  {
    return m_thing_map.at(id);
  }
  catch (std::out_of_range&)
  {
    MAJOR_ERROR("Tried to get thing %s which does not exist", boost::lexical_cast<std::string>(id).c_str());
    return m_thing_map[get_mu().m_id];
  }
}

Thing const* ThingManager::get_ptr(ThingId id) const
{
  try
  {
    return m_thing_map.at(id);
  }
  catch (std::out_of_range&)
  {
    MAJOR_ERROR("Tried to get thing %s which does not exist", boost::lexical_cast<std::string>(id).c_str());
    return m_thing_map.at(get_mu().m_id);
  }
}

ThingRef ThingManager::get_mu()
{
  return ThingRef();
}