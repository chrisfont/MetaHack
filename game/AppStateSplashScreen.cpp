#include "stdafx.h"

#include "game/AppStateSplashScreen.h"

#include "game/App.h"

AppStateSplashScreen::AppStateSplashScreen(StateMachine& stateMachine,
                                           sf::RenderWindow& appWindow,
                                           sfg::SFGUI& sfgui,
                                           sfg::Desktop& desktop)
  :
  AppState(stateMachine, {}, "AppStateSplashScreen"),
  m_appWindow{ appWindow },
  m_sfgui{ sfgui },
  m_desktop{ desktop }
{
}

AppStateSplashScreen::~AppStateSplashScreen()
{
}

void AppStateSplashScreen::execute()
{
  if (m_clock.getElapsedTime().asSeconds() >= 1 /*3*/)
  {
    this->change_to("AppStateMainMenu");
  }
}

bool AppStateSplashScreen::initialize()
{
  if (m_splash_graphic.loadFromFile("splash.bmp"))
  {
    m_splash_sprite.setTexture(m_splash_graphic);
  }

  m_clock.restart();
  return true;
}

bool AppStateSplashScreen::terminate()
{
  return true;
}

bool AppStateSplashScreen::render(sf::RenderTexture& texture, int frame)
{
  texture.draw(m_splash_sprite);
  texture.display();
  return true;
}

bool AppStateSplashScreen::onEvent(Event const& event)
{
  /// @todo WRITE ME
  return false;
}
