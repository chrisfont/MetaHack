#include "stdafx.h"

#include "AppStateMainMenu.h"

#include "App.h"

AppStateMainMenu::AppStateMainMenu(StateMachine& state_machine, sf::RenderWindow& app_window)
  :
  State{ state_machine }
{
  auto render_title_functor = std::bind(&AppStateMainMenu::render_title, this, std::placeholders::_1, std::placeholders::_2);
  the_desktop.set_pre_child_render_functor(render_title_functor);
}

AppStateMainMenu::~AppStateMainMenu()
{
  the_desktop.clear_children();
  the_desktop.clear_pre_child_render_functor();
}

void AppStateMainMenu::execute()
{}

bool AppStateMainMenu::render(sf::RenderTexture& texture, int frame)
{
  the_desktop.render(texture, frame);

  texture.display();
  return true;
}

SFMLEventResult AppStateMainMenu::handle_sfml_event(sf::Event& event)
{
  SFMLEventResult result = the_desktop.handle_sfml_event(event);

  if (result != SFMLEventResult::Handled)
  {
    switch (event.type)
    {
      case sf::Event::EventType::KeyPressed:
        switch (event.key.code)
        {
          case sf::Keyboard::Key::Space:
            // Switch to game state.
            this->change_to("AppStateGameMode");

            result = SFMLEventResult::Handled;
            break;

          default:
            break;
        }
        break;

      default:
        break;
    }
  }

  return result;
}

StringKey const& AppStateMainMenu::get_name()
{
  static StringKey name = "AppStateMainMenu";
  return name;
}

bool AppStateMainMenu::initialize()
{
  m_title.setString("Hey y'all main menu in the hizzouse");
  m_title.setFont(the_default_font);
  m_subtitle.setString("But actually there's no main menu yet so just press Spacebar.");
  m_subtitle.setFont(the_default_font);
  m_subtitle.setCharacterSize(16);
  m_subtitle.setPosition(0, 100);

  return true;
}

bool AppStateMainMenu::terminate()
{
  return true;
}

// === PROTECTED METHODS ======================================================

void AppStateMainMenu::render_title(sf::RenderTexture & texture, int frame)
{
  texture.draw(m_title);
  texture.draw(m_subtitle);
  texture.display();
}