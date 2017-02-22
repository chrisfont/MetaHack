#include "stdafx.h"

#include "AppStateGameMode.h"

#include "Action.h"
#include "App.h"
#include "GetLetterKey.h"
#include "IConfigSettings.h"
#include "InventoryArea.h"
#include "InventorySelection.h"
#include "IStringDictionary.h"
#include "KeyBuffer.h"
#include "Map.h"
#include "MapFactory.h"
#include "MapStandard2DView.h"
#include "MapTile.h"
#include "MessageLog.h"
#include "MessageLogView.h"
#include "Service.h"
#include "StatusArea.h"
#include "StringTransforms.h"
#include "Thing.h"
#include "ThingManager.h"

/// Actions that can be performed.
#include "ActionAttack.h"
#include "ActionAttire.h"
#include "ActionClose.h"
#include "ActionDrop.h"
#include "ActionEat.h"
#include "ActionFill.h"
#include "ActionGet.h"
#include "ActionHurl.h"
#include "ActionInscribe.h"
#include "ActionLock.h"
#include "ActionMix.h"
#include "ActionMove.h"
#include "ActionOpen.h"
#include "ActionPutInto.h"
#include "ActionQuaff.h"
#include "ActionRead.h"
#include "ActionShoot.h"
#include "ActionTakeOut.h"
#include "ActionTurn.h"
#include "ActionUnlock.h"
#include "ActionUse.h"
#include "ActionWait.h"
#include "ActionWield.h"

#include "GUIObject.h"
#include "GUIWindow.h"

AppStateGameMode::AppStateGameMode(StateMachine& state_machine, sf::RenderWindow& m_app_window)
  :
  AppState(state_machine,
           std::bind(&AppStateGameMode::render_map, this, std::placeholders::_1, std::placeholders::_2)),
  m_app_window{ m_app_window },
  m_debug_buffer{ NEW KeyBuffer() },
  m_game_state{ NEW GameState() },
  m_map_view{ NEW MapStandard2DView() },
  m_inventory_selection{ NEW InventorySelection() },
  m_window_in_focus{ true },
  m_inventory_area_shows_player{ false },
  m_map_zoom_level{ 1.0f },
  m_current_input_state{ GameInputState::Map },
  m_cursor_coords{ 0, 0 }
{
  the_desktop.add_child(NEW MessageLogView("MessageLogView", Service<IMessageLog>::get(), *(m_debug_buffer.get()), calc_message_log_dims())).set_flag("titlebar", true);
  the_desktop.add_child(NEW InventoryArea("InventoryArea", *(m_inventory_selection.get()), calc_inventory_dims())).set_flag("titlebar", true);
  the_desktop.add_child(NEW StatusArea("StatusArea", calc_status_area_dims())).set_global_focus(true);
}

AppStateGameMode::~AppStateGameMode()
{
  the_desktop.remove_child("StatusArea");
  the_desktop.remove_child("InventoryArea");
  the_desktop.remove_child("MessageLogView");
}

void AppStateGameMode::execute()
{
  // First, check for debug commands ready to be run.
  if (m_debug_buffer->get_enter())
  {
    /// Call the Lua interpreter with the command.
    std::string luaCommand = m_debug_buffer->get_buffer();
    Service<IMessageLog>::get().add("> " + luaCommand);
    if (luaL_dostring(the_lua_state, luaCommand.c_str()))
    {
      std::string result = lua_tostring(the_lua_state, -1);
      Service<IMessageLog>::get().add(result);
    }

    m_debug_buffer->clear_buffer();
  }

  bool ticked = GAME.process_tick();

  // If outstanding player actions have completed...
  auto player = GAME.get_player();
  if (ticked)
  {
    // Update view's cached tile data.
    m_map_view->update_tiles(player);

    GAME.increment_game_clock();
    if (!player->action_is_pending() && !player->action_is_in_progress())
    {
      reset_inventory_selection();
    }
  }
}

SFMLEventResult AppStateGameMode::handle_sfml_event(sf::Event& event)
{
  SFMLEventResult result = SFMLEventResult::Ignored;

  if (result != SFMLEventResult::Handled)
  {
    switch (event.type)
    {
      case sf::Event::EventType::Resized:
      {
        the_desktop.set_size({ event.size.width, event.size.height });
        the_desktop.get_child("MessageLogView").set_relative_dimensions(calc_message_log_dims());
        the_desktop.get_child("InventoryArea").set_relative_dimensions(calc_inventory_dims());
        the_desktop.get_child("StatusArea").set_relative_dimensions(calc_status_area_dims());
        result = SFMLEventResult::Acknowledged;
        break;
      }

      case sf::Event::EventType::KeyPressed:
        result = this->handle_key_press(event.key);
        break;

      case sf::Event::EventType::KeyReleased:
        break;

      case sf::Event::EventType::MouseWheelMoved:
        result = this->handle_mouse_wheel(event.mouseWheel);
        break;

      default:
        break;
    }
  }

  // Finally let the GUI handle events.
  if (result != SFMLEventResult::Handled)
  {
    result = the_desktop.handle_sfml_event(event);
  }

  return result;
}

std::string const& AppStateGameMode::get_name()
{
  static std::string name = "AppStateGameMode";
  return name;
}

bool AppStateGameMode::initialize()
{
  auto& config = Service<IConfigSettings>::get();

  // Create the player.
  ThingId player = get_game_state().get_things().create("Human");
  player->set_proper_name(config.get<std::string>("player_name"));
  get_game_state().set_player(player);

  // Create the game map.
  /// @todo This shouldn't be hardcoded here
#ifdef NDEBUG
  MapId current_map_id = GAME.get_maps().create(64, 64);
#else
  MapId current_map_id = GAME.get_maps().create(16, 16);
#endif

  Map& game_map = GAME.get_maps().get(current_map_id);

  // Move player to start position on the map.
  auto& start_coords = game_map.get_start_coords();
  auto start_floor = game_map.get_tile(start_coords).get_tile_contents();
  /* bool player_moved = */ player->move_into(start_floor);

  // Set cursor to starting location.
  m_cursor_coords = start_coords;

  // Set the viewed inventory location to the player's location.
  m_inventory_area_shows_player = false;
  reset_inventory_selection();

  // Set the map view.
  m_map_view->set_map_id(current_map_id);

  // Get the map ready.
  game_map.update_lighting();

  // Get the map view ready.
  m_map_view->update_tiles(player);
  m_map_view->update_things(player, 0);

  put_tmsg("WELCOME_MSG");

  return true;
}

bool AppStateGameMode::terminate()
{
  return true;
}

GameState& AppStateGameMode::get_game_state()
{
  return *m_game_state;
}

// === PROTECTED METHODS ======================================================
void AppStateGameMode::render_map(sf::RenderTexture& texture, int frame)
{
  auto& config = Service<IConfigSettings>::get();

  texture.clear();

  ThingId player = get_game_state().get_player();
  ThingId location = player->get_location();

  if (location == get_game_state().get_things().get_mu())
  {
    throw std::runtime_error("Uh oh, the player's location appears to have been deleted!");
  }

  /// @todo We need a way to determine if the player is directly on a map,
  ///       and render either the map, or a container interior.
  ///       Should probably use an overridden "render_surroundings" method
  ///       for Things.

  if (!player->is_inside_another_thing())
  {
    MapTile* tile = player->get_maptile();
    if (tile != nullptr)
    {
      Map& game_map = GAME.get_maps().get(tile->get_map_id());
      Vec2i tile_coords = tile->get_coords();
      Vec2f player_pixel_coords = MapTile::get_pixel_coords(tile_coords);
      Vec2f cursor_pixel_coords = MapTile::get_pixel_coords(m_cursor_coords);

      // Update thing vertex array.
      m_map_view->update_things(player, frame);

      if (m_current_input_state == GameInputState::CursorLook)
      {
        m_map_view->set_view(texture, cursor_pixel_coords, m_map_zoom_level);
        m_map_view->render(texture, frame);

        auto& cursor_tile = game_map.get_tile(m_cursor_coords);
        cursor_tile.draw_highlight(texture,
                                   cursor_pixel_coords,
                                   config.get<sf::Color>("cursor_border_color"),
                                   config.get<sf::Color>("cursor_bg_color"),
                                   frame);
      }
      else
      {
        m_map_view->set_view(texture, player_pixel_coords, m_map_zoom_level);
        m_map_view->render(texture, frame);
      }
    }
  }
  texture.display();
}

SFMLEventResult AppStateGameMode::handle_key_press(sf::Event::KeyEvent& key)
{
  SFMLEventResult result = SFMLEventResult::Ignored;

  ThingId player = get_game_state().get_player();

  // *** Handle keys processed in any mode.
  if (!key.alt && !key.control)
  {
    if (key.code == sf::Keyboard::Key::Tilde)
    {
      switch (m_current_input_state)
      {
        case GameInputState::Map:
          m_current_input_state = GameInputState::MessageLog;
          the_desktop.get_child("MessageLogView").set_global_focus(true);
          return SFMLEventResult::Handled;

        case GameInputState::MessageLog:
          m_current_input_state = GameInputState::Map;
          the_desktop.get_child("StatusArea").set_global_focus(true);
          return SFMLEventResult::Handled;

        default:
          break;
      }
    }
  }

  // *** Handle keys unique to a particular focus.
  switch (m_current_input_state)
  {
    case GameInputState::TargetSelection:
      return handle_key_press_target_selection(player, key);

    case GameInputState::CursorLook:
      return handle_key_press_cursor_look(player, key);

    case GameInputState::Map:
    {
      std::unique_ptr<Action> p_action;

      std::vector<ThingId>& things = m_inventory_selection->get_selected_things();
      int key_number = get_letter_key(key);
      Direction key_direction = get_direction_key(key);

      // *** No ALT, no CTRL, shift is irrelevant ****************************
      if (!key.alt && !key.control)
      {
        if (key_number != -1)
        {
          m_inventory_selection->toggle_selection(static_cast<InventorySlot>(key_number));
          result = SFMLEventResult::Handled;
        }
        else if (key.code == sf::Keyboard::Key::Tab)
        {
          m_inventory_area_shows_player = !m_inventory_area_shows_player;
          reset_inventory_selection();
          result = SFMLEventResult::Handled;
        }
        else if (key.code == sf::Keyboard::Key::Escape)
        {
          put_tmsg("QUIT_MSG");
          result = SFMLEventResult::Handled;
        }
      }

      // *** No ALT, no CTRL, no SHIFT ***************************************
      if (!key.alt && !key.control && !key.shift)
      {
        if (key_direction != Direction::None)
        {
          if (key_direction == Direction::Self)
          {
            p_action.reset(new ActionWait(player));
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
          else
          {
            p_action.reset(new ActionTurn(player));
            p_action->set_target(key_direction);
            player->queue_action(std::move(p_action));

            p_action.reset(new ActionMove(player));
            p_action->set_target(key_direction);
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
        }
        else switch (key.code)
        {
          case sf::Keyboard::Key::BackSpace:
            reset_inventory_selection();
            break;

            // "/" - go to cursor look mode.
          case sf::Keyboard::Key::Slash:
            m_current_input_state = GameInputState::CursorLook;
            m_inventory_area_shows_player = false;
            reset_inventory_selection();
            result = SFMLEventResult::Handled;
            break;

            // "-" - subtract quantity
          case sf::Keyboard::Key::Dash:
          case sf::Keyboard::Key::Subtract:
          {
            /// @todo Need a way to choose which inventory we're affecting.
            auto slot_count = m_inventory_selection->get_selected_slot_count();
            if (slot_count < 1)
            {
              put_tmsg("QUANTITY_SUBTRACT_NEED_SOMETHING_SELECTED");
            }
            else if (slot_count > 1)
            {
              put_tmsg("QUANTITY_SUBTRACT_NEED_ONE_THING_SELECTED");
            }
            else
            {
              m_inventory_selection->dec_selected_quantity();
            }
          }
          result = SFMLEventResult::Handled;
          break;

          // "+"/"=" - add quantity
          case sf::Keyboard::Key::Equal:
          case sf::Keyboard::Key::Add:
          {
            auto slot_count = m_inventory_selection->get_selected_slot_count();
            if (slot_count < 1)
            {
              put_tmsg("QUANTITY_SUBTRACT_NEED_SOMETHING_SELECTED");
            }
            else if (slot_count > 1)
            {
              put_tmsg("QUANTITY_SUBTRACT_NEED_ONE_THING_SELECTED");
            }
            else
            {
              m_inventory_selection->inc_selected_quantity();
            }
          }
          result = SFMLEventResult::Handled;
          break;

          case sf::Keyboard::Key::LBracket:
          {
            ThingId thing = m_inventory_selection->get_viewed();
            ThingId location = thing->get_location();
            if (location != get_game_state().get_things().get_mu())
            {
              m_inventory_selection->set_viewed(location);
            }
            else
            {
              put_tmsg("AT_TOP_OF_INVENTORY_TREE");
            }
          }
          break;

          case sf::Keyboard::Key::RBracket:
          {
            auto slot_count = m_inventory_selection->get_selected_slot_count();

            if (slot_count > 0)
            {
              ThingId thing = m_inventory_selection->get_selected_things().at(0);
              if (thing->get_intrinsic<int>("inventory_size") != 0)
              {
                if (!thing->can_have_action_done_by(ThingId::Mu(), ActionOpen::prototype) ||
                    thing->get_modified_property<bool>("open"))
                {
                  if (!thing->can_have_action_done_by(ThingId::Mu(), ActionLock::prototype) ||
                      !thing->get_modified_property<bool>("locked"))
                  {
                    m_inventory_selection->set_viewed(thing);
                  }
                  else // if (container.is_locked())
                  {
                    std::string message = Action::make_string(player, thing, "THE_FOO_IS_LOCKED");
                    put_msg(message);
                  }
                }
                else // if (!container.is_open())
                {
                  /// @todo Need a way to make this cleaner.
                  std::string message = Action::make_string(player, thing, "THE_FOO_IS_CLOSED");
                  put_msg(message);
                }
              }
              else // if (!thing.is_container())
              {
                /// @todo This probably doesn't belong here.
                std::string message = Action::make_string(player, thing, "THE_FOO_IS_NOT_A_CONTAINER");
                put_msg(message);
              }
            }
            else
            {
              put_tmsg("NOTHING_IS_SELECTED");
            }
            break;
          }

          case sf::Keyboard::Key::Comma:
            /// @todo Find a better way to do this. This is hacky.
            key.alt = false;
            key.control = true;
            key.shift = false;
            key.code = sf::Keyboard::Key::G;
            break;

          default:
            break;
        } // end switch (key.code)
      } // end if (!key.alt && !key.control && !key.shift)

        // *** No ALT, YES CTRL, SHIFT is irrelevant ******************************
      if (!key.alt && key.control)
      {
        if (key_direction != Direction::None)
        {
          if (key_direction == Direction::Self)
          {
            p_action.reset(new ActionWait(player));
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
          else
          {
            // CTRL-arrow -- Turn without moving
            p_action.reset(new ActionTurn(player));
            p_action->set_target(key_direction);
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
        }
        else switch (key.code)
        {
          // CTRL-MINUS -- Zoom out
          case sf::Keyboard::Key::Dash:
          case sf::Keyboard::Key::Subtract:
            add_zoom(-0.05f);
            break;

            // CTRL-PLUS -- Zoom in
          case sf::Keyboard::Key::Equal:
          case sf::Keyboard::Key::Add:
            add_zoom(0.05f);
            break;

          case sf::Keyboard::Key::Num0:
            m_map_zoom_level = 1.0f;
            break;

            // CTRL-A -- attire/adorn
          case sf::Keyboard::Key::A:    // Attire
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_WEAR_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionAttire(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-C -- close items
          case sf::Keyboard::Key::C:    // Close
            if (things.size() == 0)
            {
              // No item specified, so ask for a direction.
              m_action_in_progress.reset(new ActionClose(player));
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_DIRECTION"), { tr("VERB_CLOSE_2") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            else
            {
              // Item(s) specified, so proceed with items.
              for (auto thing : things)
              {
                p_action.reset(new ActionClose(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-D -- drop items
          case sf::Keyboard::Key::D:    // Drop
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_DROP_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionDrop(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-E -- eat items
          case sf::Keyboard::Key::E:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_EAT_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionEat(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-F -- fill item(s)
          case sf::Keyboard::Key::F:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_FIRST"), { tr("VERB_FILL_2") }));
            }
            else if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_FILL_2") }));
            }
            else
            {
              m_action_in_progress.reset(new ActionFill(player));
              m_action_in_progress->set_object(things.front());
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_OR_DIRECTION"), { tr("VERB_FILL_GER") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-G -- get (pick up) items
          case sf::Keyboard::Key::G:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_PICKUP_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionGet(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-H -- hurl items
          case sf::Keyboard::Key::H:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_FIRST"), { tr("VERB_THROW_2") }));
            }
            else if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_THROW_2") }));
            }
            else
            {
              m_action_in_progress.reset(new ActionHurl(player));
              m_action_in_progress->set_object(things.front());
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_DIRECTION"), { tr("VERB_THROW_2") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-I -- inscribe with an item
          case sf::Keyboard::Key::I:
            if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_WRITE_2") }));
            }
            else
            {
              m_action_in_progress.reset(new ActionInscribe(player));
              if (things.size() != 0)
              {
                m_action_in_progress->set_object(things.front());
              }

              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_OR_DIRECTION"), { tr("VERB_WRITE_GER") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-M -- mix items
          case sf::Keyboard::Key::M:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_TWO_ITEMS_FIRST"), { tr("VERB_MIX_2") }));
            }
            else if (things.size() != 2)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_EXACTLY_TWO_AT_A_TIME"), { tr("VERB_MIX_2") }));
            }
            else
            {
              p_action.reset(new ActionMix(player));
              p_action->set_objects(things);
              player->queue_action(std::move(p_action));
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

          case sf::Keyboard::Key::O:    // Open
            if (things.size() == 0)
            {
              // No item specified, so ask for a direction.
              m_action_in_progress.reset(new ActionOpen(player));
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_DIRECTION"), { tr("VERB_OPEN_2") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            else
            {
              // Item(s) specified, so proceed with items.
              for (auto thing : things)
              {
                p_action.reset(new ActionOpen(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-P -- put item in another item
          case sf::Keyboard::Key::P:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_STORE_2") }));
            }
            else
            {
              m_action_in_progress.reset(new ActionPutInto(player));
              m_action_in_progress->set_objects(things);
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_CONTAINER"), { tr("VERB_STORE_DESC") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-Q -- quaff (drink) from items
          case sf::Keyboard::Key::Q:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_DRINK_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionQuaff(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-R -- read an item
            /// @todo Maybe allow reading from direction?
          case sf::Keyboard::Key::R:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_READ_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionRead(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-S -- shoot items
          case sf::Keyboard::Key::S:
            /// @todo Skip the item check here, as we want to shoot our wielded weapon
            /// if no item is selected.
            if (things.size() < 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_FIRST"), { tr("VERB_SHOOT_2") }));
            }
            if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_SHOOT_2") }));
            }
            else
            {
              /// @todo If no items are selected, fire your wielded item.
              ///       Otherwise, wield the selected item and fire it.
              m_action_in_progress.reset(new ActionShoot(player));
              m_action_in_progress->set_object(things.front());
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_DIRECTION"), { tr("VERB_SHOOT_2") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-T -- take item out of container
          case sf::Keyboard::Key::T:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_REMOVE_2") }));
            }
            else
            {
              p_action.reset(new ActionTakeOut(player));
              p_action->set_objects(things);
              player->queue_action(std::move(p_action));
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-U -- use an item
          case sf::Keyboard::Key::U:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEMS_FIRST"), { tr("VERB_USE_2") }));
            }
            else
            {
              for (auto thing : things)
              {
                p_action.reset(new ActionUse(player));
                p_action->set_object(thing);
                player->queue_action(std::move(p_action));
              }
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-W -- wield item
          case sf::Keyboard::Key::W:
            if (things.size() == 0)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ITEM_FIRST"), { tr("VERB_WIELD_2") }));
            }
            else if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_WIELD_2") }));
            }
            else
            {
              p_action.reset(new ActionWield(player));
              p_action->set_object(things.front());
              player->queue_action(std::move(p_action));
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }
            result = SFMLEventResult::Handled;
            break;

            // CTRL-X -- Xplicit attack
          case sf::Keyboard::Key::X:
            if (things.size() == 0)
            {
              // No item specified, so ask for a direction.
              m_action_in_progress.reset(new ActionAttack(player));
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_DIRECTION"), { tr("VERB_ATTACK_2") }));
              m_current_input_state = GameInputState::TargetSelection;
              m_inventory_selection->clear_selected_slots();
            }
            else if (things.size() > 1)
            {
              put_msg(Action::make_string(player, ThingId::Mu(), tr("CHOOSE_ONLY_ONE_AT_A_TIME"), { tr("VERB_ATTACK_2") }));
            }
            else
            {
              // Item(s) specified, so proceed with items.
              p_action.reset(new ActionAttack(player));
              p_action->set_object(things.front());
              player->queue_action(std::move(p_action));
              m_inventory_area_shows_player = false;
              reset_inventory_selection();
            }

          default:
            break;
        } // end switch
      } // end if (!key.alt && key.control)

        // *** YES ALT, no CTRL, SHIFT is irrelevant ******************************
        // Right now we don't have any ALT-* combinations.
#if 0
      if (key.alt && !key.control)
      {
        switch (key.code)
        {
          default:
            break;
        }
      }
#endif

      // *** YES ALT, YES CTRL, SHIFT is irrelevant *****************************
      if (key.alt && key.control)
      {
        if (key_direction != Direction::None)
        {
          if (key_direction == Direction::Self)
          {
            p_action.reset(new ActionWait(player));
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
          else
          {
            // CTRL-ALT-arrow -- Move without turning
            p_action.reset(new ActionMove(player));
            p_action->set_target(key_direction);
            player->queue_action(std::move(p_action));
            result = SFMLEventResult::Handled;
          }
        }
        else switch (key.code)
        {
          // There are no other CTRL-ALT key combinations right now.
          default:
            break;
        }
      }

      break;
    } // end case GameInputState::Map

    default:
      break;
  } // end switch (m_current_input_state)

  return result;
}

SFMLEventResult AppStateGameMode::handle_mouse_wheel(sf::Event::MouseWheelEvent& wheel)
{
  add_zoom(wheel.delta * 0.05f);
  return SFMLEventResult::Handled;
}

void AppStateGameMode::add_zoom(float zoom_amount)
{
  float current_zoom_level = m_map_zoom_level;

  current_zoom_level += zoom_amount;

  if (current_zoom_level < 0.1f)
  {
    current_zoom_level = 0.1f;
  }

  if (current_zoom_level > 3.0f)
  {
    current_zoom_level = 3.0f;
  }

  m_map_zoom_level = current_zoom_level;
}

sf::IntRect AppStateGameMode::calc_message_log_dims()
{
  sf::IntRect messageLogDims;
  auto& config = Service<IConfigSettings>::get();

  unsigned int inventory_area_width = config.get<unsigned int>("inventory_area_width");
  unsigned int messagelog_area_height = config.get<unsigned int>("messagelog_area_height");
  messageLogDims.width = m_app_window.getSize().x - (inventory_area_width + 24);
  messageLogDims.height = messagelog_area_height - 10;
  //messageLogDims.height = static_cast<int>(m_app_window.getSize().y * 0.25f) - 10;
  messageLogDims.left = 12;
  messageLogDims.top = 5;
  return messageLogDims;
}

void AppStateGameMode::reset_inventory_selection()
{
  ThingId player = m_game_state->get_player();
  Map& game_map = GAME.get_maps().get(player->get_map_id());

  if (m_inventory_area_shows_player == true)
  {
    m_inventory_selection->set_viewed(player);
  }
  else
  {
    if (m_current_input_state == GameInputState::CursorLook)
    {
      ThingId floor_id = game_map.get_tile(m_cursor_coords).get_tile_contents();
      m_inventory_selection->set_viewed(floor_id);
    }
    else
    {
      m_inventory_selection->set_viewed(player->get_location());
    }
  }
}

sf::IntRect AppStateGameMode::calc_status_area_dims()
{
  sf::IntRect statusAreaDims;
  sf::IntRect invAreaDims = the_desktop.get_child("InventoryArea").get_relative_dimensions();
  auto& config = Service<IConfigSettings>::get();

  statusAreaDims.width = m_app_window.getSize().x -
    (invAreaDims.width + 24);
  statusAreaDims.height = config.get<int>("status_area_height");
  statusAreaDims.top = m_app_window.getSize().y - (config.get<int>("status_area_height") + 5);
  statusAreaDims.left = 12;
  return statusAreaDims;
}

sf::IntRect AppStateGameMode::calc_inventory_dims()
{
  sf::IntRect messageLogDims = the_desktop.get_child("MessageLogView").get_relative_dimensions();
  sf::IntRect inventoryAreaDims;
  auto& config = Service<IConfigSettings>::get();

  inventoryAreaDims.width = config.get<int>("inventory_area_width");
  inventoryAreaDims.height = m_app_window.getSize().y - 10;
  inventoryAreaDims.left = m_app_window.getSize().x - (inventoryAreaDims.width + 3);
  inventoryAreaDims.top = 5;

  return inventoryAreaDims;
}

bool AppStateGameMode::move_cursor(Direction direction)
{
  ThingId player = m_game_state->get_player();
  Map& game_map = GAME.get_maps().get(player->get_map_id());
  bool result;

  result = game_map.calc_coords(m_cursor_coords, direction, m_cursor_coords);

  return result;
}

SFMLEventResult AppStateGameMode::handle_key_press_target_selection(ThingId player, sf::Event::KeyEvent& key)
{
  SFMLEventResult result = SFMLEventResult::Ignored;
  int key_number = get_letter_key(key);
  Direction key_direction = get_direction_key(key);

  if (!key.alt && !key.control && key.code == sf::Keyboard::Key::Tab)
  {
    m_inventory_area_shows_player = !m_inventory_area_shows_player;
    reset_inventory_selection();
  }

  if (!key.alt && !key.control && key.code == sf::Keyboard::Key::Escape)
  {
    put_tmsg("ABORTED");
    m_inventory_area_shows_player = false;
    reset_inventory_selection();
    m_current_input_state = GameInputState::Map;
    result = SFMLEventResult::Handled;
  }

  if (m_action_in_progress && m_action_in_progress->can_be_subject_verb_object_preposition_target())
  {
    if (!key.alt && !key.control && key_number != -1)
    {
      m_action_in_progress->set_target(m_inventory_selection->get_thing(static_cast<InventorySlot>(key_number)));
      player->queue_action(std::move(m_action_in_progress));
      m_inventory_area_shows_player = false;
      reset_inventory_selection();
      m_current_input_state = GameInputState::Map;
      result = SFMLEventResult::Handled;
    }
  } // end if (action_in_progress.target_can_be_thing)

  if (m_action_in_progress && (m_action_in_progress->can_be_subject_verb_direction() ||
                               m_action_in_progress->can_be_subject_verb_object_preposition_direction()))
  {
    if (!key.alt && !key.control && key_direction != Direction::None)
    {
      m_action_in_progress->set_target(key_direction);
      player->queue_action(std::move(m_action_in_progress));
      m_inventory_area_shows_player = false;
      reset_inventory_selection();
      m_current_input_state = GameInputState::Map;
      result = SFMLEventResult::Handled;
    }
  }

  return result;
}

SFMLEventResult AppStateGameMode::handle_key_press_cursor_look(ThingId player, sf::Event::KeyEvent& key)
{
  SFMLEventResult result = SFMLEventResult::Ignored;

  // *** NON-MODIFIED KEYS ***********************************************
  if (!key.alt && !key.control && !key.shift)
  {
    switch (key.code)
    {
      case sf::Keyboard::Key::Up:
        move_cursor(Direction::North);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::PageUp:
        move_cursor(Direction::Northeast);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::Right:
        move_cursor(Direction::East);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::PageDown:
        move_cursor(Direction::Southeast);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::Down:
        move_cursor(Direction::South);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::End:
        move_cursor(Direction::Southwest);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::Left:
        move_cursor(Direction::West);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      case sf::Keyboard::Key::Home:
        move_cursor(Direction::Northwest);
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

        // "/" - go back to Map focus.
      case sf::Keyboard::Key::Slash:
        m_current_input_state = GameInputState::Map;
        m_inventory_area_shows_player = false;
        reset_inventory_selection();
        result = SFMLEventResult::Handled;
        break;

      default:
        break;
    } // end switch (key.code)
  } // end if (no modifier keys)

  return result;
}