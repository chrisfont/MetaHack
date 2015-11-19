#ifndef APPSTATEGAMEMODE_H
#define APPSTATEGAMEMODE_H

#include <memory>

#include "GameState.h"
#include "InventoryArea.h"
#include "MapId.h"
#include "MessageLog.h"
#include "State.h"
#include "StatusArea.h"
#include "Thing.h"
#include "UsesPimpl.h"

#include "AppStateGameModeImpl.h"

class AppStateGameMode : public State
{
  public:
    AppStateGameMode(StateMachine& state_machine, sf::RenderWindow& app_window);
    virtual ~AppStateGameMode();

    void execute() override;
    bool render(sf::RenderTarget& target, int frame) override;
    EventResult handle_event(sf::Event& event) override;
    std::string const& get_name() override;
    bool initialize() override;
    bool terminate() override;

    GameState& get_game_state();

  protected:
    EventResult handle_key_press(sf::Event::KeyEvent& key);
    EventResult handle_mouse_wheel(sf::Event::MouseWheelEvent& wheel);

    void add_zoom(float zoom_amount);

  private:
    Pimpl<AppStateGameModeImpl> pImpl;
};

#endif // APPSTATEGAMEMODE_H
