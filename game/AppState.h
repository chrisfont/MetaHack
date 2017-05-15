#ifndef APPSTATE_H
#define APPSTATE_H

#include "stdafx.h"

#include "types/IRenderable.h"
#include "state_machine/State.h"

#include "GUIDesktop.h"

// Forward declarations
class StateMachine;

class AppState :
  public State
{
public:
  AppState(StateMachine& state_machine,
           metagui::RenderFunctor preDesktopRenderFunctor = metagui::RenderFunctor(),
           metagui::RenderFunctor postDesktopRenderFunctor = metagui::RenderFunctor());

  virtual ~AppState();

  virtual bool render(sf::RenderTexture& texture, int frame) override final;

  virtual std::unordered_set<EventID> registeredEvents() const override;

protected:
private:
  metagui::RenderFunctor m_preDesktopRenderFunctor;
  metagui::RenderFunctor m_postDesktopRenderFunctor;
};

#endif // APPSTATE_H
