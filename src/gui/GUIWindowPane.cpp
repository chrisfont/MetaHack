#include "stdafx.h"

#include "gui/GUIWindowPane.h"

#include "gui/GUITitleBar.h"
#include "gui/GUICloseHandle.h"
#include "gui/GUIResizeHandle.h"
#include "gui/GUIShrinkHandle.h"

#include "App.h"
#include "ConfigSettings.h"
#include "New.h"

namespace metagui
{
  WindowPane::WindowPane(std::string name, sf::Vector2i location, sf::Vector2i size)
    :
    Pane(name, location, size)
  {}

  WindowPane::WindowPane(std::string name, sf::IntRect dimensions)
    :
    Pane(name, dimensions)
  {}

  WindowPane::~WindowPane()
  {}

  void WindowPane::set_title_bar(bool enabled)
  {
    m_title_bar = enabled;
    std::string name = get_name() + "_titlebar";

    if (enabled)
    {
      if (!child_exists(name))
      {
        add_child(new TitleBar(name));
      }
    }
    else
    {
      remove_child(name);
    }
  }

  bool WindowPane::get_title_bar()
  {
    return m_title_bar;
  }

  void WindowPane::set_movable(bool enabled)
  {
    m_movable = enabled;
  }

  bool WindowPane::get_movable()
  {
    return m_movable;
  }

  void WindowPane::set_resizable(bool enabled)
  {
    m_resizable = enabled;

    std::string name = get_name() + "_resizehandle";
    if (enabled)
    {
      if (!child_exists(name))
      {
        add_child(new ResizeHandle(name));
      }
    }
    else
    {
      remove_child(name);
    }
  }

  bool WindowPane::get_resizable()
  {
    return m_resizable;
  }

  void WindowPane::set_closable(bool enabled)
  {
    m_closable = enabled;

    std::string name = get_name() + "_closehandle";
    if (enabled)
    {
      if (!child_exists(name))
      {
        add_child(new CloseHandle(name));
      }
    }
    else
    {
      remove_child(name);
    }
  }

  bool WindowPane::get_shrinkable()
  {
    return m_shrinkable;
  }

  void WindowPane::set_shrinkable(bool enabled)
  {
    m_shrinkable = enabled;

    std::string name = get_name() + "_shrinkhandle";
    if (enabled)
    {
      if (!child_exists(name))
      {
        add_child(new ShrinkHandle(name));
      }
    }
    else
    {
      remove_child(name);
    }
  }

  bool WindowPane::get_closable()
  {
    return m_closable;
  }

  // === PROTECTED METHODS ======================================================

  void WindowPane::render_self_before_children_(sf::RenderTexture& texture, int frame)
  {
    sf::Vector2i size = get_size();

    float line_spacing_y = the_default_font.getLineSpacing(Settings.get<unsigned int>("text_default_size"));

    // Text offsets relative to the background rectangle.
    float text_offset_x = 3;
    float text_offset_y = 3;

    // Clear the target.
    texture.clear(Settings.get<sf::Color>("window_bg_color"));

    // Render the contents, if any.
    render_contents_(texture, frame);

    // IF the pane has a title...
    if (!get_text().empty())
    {
      // Draw the title in the upper-left corner.
      sf::RectangleShape title_rect;
      sf::Text title_text;

      title_text.setString(get_text());
      title_text.setFont(the_default_bold_font);
      title_text.setCharacterSize(Settings.get<unsigned int>("text_default_size"));

      title_rect.setFillColor(Settings.get<sf::Color>("window_bg_color"));
      title_rect.setOutlineColor(get_focus() ?
                                 Settings.get<sf::Color>("window_focused_border_color") :
                                 Settings.get<sf::Color>("window_border_color"));
      title_rect.setOutlineThickness(Settings.get<float>("window_border_width"));
      title_rect.setPosition({ 0, 0 });
      title_rect.setSize(sf::Vector2f(static_cast<float>(size.x),
                                      static_cast<float>(line_spacing_y + (text_offset_y * 2))));

      texture.draw(title_rect);

      title_text.setColor(Settings.get<sf::Color>("text_color"));
      title_text.setPosition(sf::Vector2f(text_offset_x + line_spacing_y,
                                          text_offset_y));
      texture.draw(title_text);
    }

    // Draw the border.
    float border_width = Settings.get<float>("window_border_width");
    m_border_shape.setPosition(sf::Vector2f(border_width, border_width));
    m_border_shape.setSize(sf::Vector2f(static_cast<float>(size.x - (2 * border_width)), static_cast<float>(size.y - (2 * border_width))));
    m_border_shape.setFillColor(sf::Color::Transparent);
    m_border_shape.setOutlineColor(
      get_focus() ?
      Settings.get<sf::Color>("window_focused_border_color") :
      Settings.get<sf::Color>("window_border_color"));
    m_border_shape.setOutlineThickness(border_width);

    //texture.setView(sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(target.getSize().x), static_cast<float>(target.getSize().y))));

    texture.draw(m_border_shape);
  }

  void WindowPane::render_contents_(sf::RenderTexture& texture, int frame)
  {}
}; // end namespace metagui