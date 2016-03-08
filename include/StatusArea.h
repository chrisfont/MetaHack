#ifndef STATUSAREA_H
#define STATUSAREA_H

#include "stdafx.h"

#include "gui/GUIWindowPane.h"

class StatusArea : public metagui::WindowPane
{
public:
  explicit StatusArea(sf::IntRect dimensions);
  virtual ~StatusArea();

  virtual EventResult handle_event_before_children_(sf::Event& event) override;

protected:
  virtual void render_contents_(sf::RenderTexture& texture, int frame) override;

private:
  void render_attribute(sf::RenderTarget& target, std::string abbrev, std::string name, sf::Vector2f location);

  std::string get_test_label();

  struct Impl;
  std::unique_ptr<Impl> pImpl;
};

#endif // STATUSAREA_H
