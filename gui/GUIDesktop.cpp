#include "stdafx.h"

#include "GUIDesktop.h"

#include "design_patterns/Subject.h"
#include "game/App.h"
#include "services/IConfigSettings.h"
#include "Service.h"

namespace metagui
{
  Desktop::Desktop(Subject& event_parent,
                   std::string name, 
                   UintVec2 size)
    :
    GUIObject{ name, IntVec2(0, 0), size },
    m_event_parent{ event_parent }
  {
    m_event_parent.addObserver(*this, EventID::All);
  }

  Desktop::~Desktop()
  {
    m_event_parent.removeObserver(*this, EventID::All);
  }

  SFMLEventResult Desktop::handle_sfml_event(sf::Event & sfml_event)
  {
    SFMLEventResult sfml_result = SFMLEventResult::Ignored;

    switch (sfml_event.type)
    {
      case sf::Event::EventType::KeyPressed:
      {
        EventKeyPressed event{ sfml_event.key };
        broadcast(event);
        sfml_result = SFMLEventResult::Handled;
      }
      break;

      case sf::Event::EventType::Resized:
      {
        setSize({ sfml_event.size.width, sfml_event.size.height });
        sfml_result = SFMLEventResult::Acknowledged;
      }
      break;

      case sf::Event::EventType::MouseButtonPressed:
      {
        IntVec2 point{ sfml_event.mouseButton.x, sfml_event.mouseButton.y };
        auto& button{ m_button_info[static_cast<unsigned int>(sfml_event.mouseButton.button)] };

        button.pressed = true;
        button.location = point;
        button.elapsed.restart();

        EventMouseDown event{ sfml_event.mouseButton.button, point };
        broadcast(event);

        /// @todo Handle click, double-click, etc.
        sfml_result = SFMLEventResult::Acknowledged;
      }
      break;

      case sf::Event::EventType::MouseButtonReleased:
      {
        IntVec2 point{ sfml_event.mouseButton.x, sfml_event.mouseButton.y };
        auto& button{ m_button_info[static_cast<unsigned int>(sfml_event.mouseButton.button)] };

        button.pressed = false;
        button.location = point;
        button.elapsed.restart();

        if (button.dragging)
        {
          button.dragging = false;
          EventDragFinished event{ sfml_event.mouseButton.button, point };
          broadcast(event);
        }

        /// @todo Handle click, double-click, etc.
        sfml_result = SFMLEventResult::Acknowledged;
      }
      break;

      case sf::Event::EventType::MouseMoved:
      {
        IntVec2 point{ sfml_event.mouseMove.x, sfml_event.mouseMove.y };
        m_mouse_location = point;

        /// @todo Handle entities like dragging, resizing
        for (unsigned int index = 0; index < sf::Mouse::ButtonCount; ++index)
        {
          auto& button_info = m_button_info[index];

          if ((button_info.dragging == true) ||
            (button_info.pressed &&
             distance(point, button_info.location) > EventDragging::drag_threshold))
          {
            if (button_info.dragging != true)
            {
              button_info.dragging = true;
              EventDragStarted event{ static_cast<sf::Mouse::Button>(index), button_info.location };
              broadcast(event);
            }

            EventDragging event{ static_cast<sf::Mouse::Button>(index), point };
            broadcast(event);

            sfml_result = SFMLEventResult::Handled;
          }
        }

        sfml_result = SFMLEventResult::Handled;
      }
      break;

      case sf::Event::EventType::MouseLeft:
      {
        for (auto& button : m_button_info)
        {
          if (button.pressed)
          {
            button.pressed = false;
            // Don't update location, just keep it as last seen
            button.elapsed.restart();

            if (button.dragging)
            {
              button.dragging = false;
              EventDragFinished event{ sfml_event.mouseButton.button, button.location };
              broadcast(event);
            }
          }
        }
        sfml_result = SFMLEventResult::Handled;
      }
      break;

      default:
        break;
    }

    return sfml_result;
  }

  std::unordered_set<EventID> Desktop::registeredEvents() const
  {
    auto events = Object::registeredEvents();
    /// @todo Add generated and propogated events here.
    return events;
  }

  // === PROTECTED METHODS ======================================================

  bool Desktop::onEvent_PreChildren_NVI(Event const& event)
  {
    auto id = event.getId();
    if (id == EventResized::id())
    {
      if (event.subject == getParent())
      {
        auto& castEvent = static_cast<EventResized const&>(event);
        setSize(castEvent.new_size);
      }
      // Don't forward one of these to children, as we'll make our own to pass
      // along if necessary.
      return false;
    }

    return true;
  }

  void Desktop::drawPreChildren_(sf::RenderTexture& texture, int frame)
  {
  }

}; // end namespace metagui