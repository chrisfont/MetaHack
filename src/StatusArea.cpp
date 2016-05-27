#include "stdafx.h"

#include "StatusArea.h"

#include "GUILabel.h"
#include "App.h"
#include "ConfigSettings.h"
#include "GameState.h"
#include "Thing.h"
#include "ThingManager.h"

struct StatusArea::Impl
{
  bool dummy;
};

StatusArea::StatusArea(sf::IntRect dimensions)
  : metagui::Window("StatusArea", dimensions),
  pImpl(NEW Impl())
{
  //ctor
  //std::function<std::string()> label_function = std::bind(&StatusArea::get_test_label, this);
  //GUILabel* label = NEW GUILabel( {0, 0, dimensions.width / 2, dimensions.height / 2}, label_function );
  //add_child(label);
}

StatusArea::~StatusArea()
{
  //dtor
}

void StatusArea::render_contents_(sf::RenderTexture& texture, int frame)
{
  sf::IntRect pane_dims = get_relative_dimensions();
  ThingId player = GAME.get_player();
  sf::Vector2f origin(the_config.get<float>("window_text_offset_x"),
                      the_config.get<float>("window_text_offset_y"));
  sf::Color text_color = the_config.get<sf::Color>("text_color");
  sf::Color text_dim_color = the_config.get<sf::Color>("text_dim_color");
  sf::Color text_warning_color = the_config.get<sf::Color>("text_warning_color");
  sf::Color text_danger_color = the_config.get<sf::Color>("text_danger_color");
  float line_spacing = the_default_font.getLineSpacing(the_config.get<unsigned int>("text_default_size")) + 3.0f;
  float attrib_spacing = 75.0f;

  // Text offsets relative to the background rectangle.
  sf::Text render_text;
  render_text.setFont(the_default_font);
  render_text.setColor(text_color);
  render_text.setCharacterSize(the_config.get<unsigned int>("text_default_size"));
  render_text.setPosition(origin.x, origin.y);

  if (player != ThingId::Mu())
  {
    // Render player name
    StringDisplay name = player->get_proper_name();
    name[0] = std::toupper(name[0], std::locale());

    StringDisplay type = player->get_display_name();
    type[0] = std::toupper(type[0], std::locale());

    render_text.setString(name + L" the " + type);
    texture.draw(render_text);

    // Render HP
    render_text.setFont(the_default_mono_font);
    render_text.setColor(text_dim_color);
    render_text.setPosition(origin.x, origin.y + line_spacing);
    render_text.setString(L"HP");
    texture.draw(render_text);

    int hp = player->get_modified_property<int>("hp");
    int max_hp = player->get_modified_property<int>("maxhp");

    float hp_percentage = static_cast<float>(hp) / static_cast<float>(max_hp);

    if (hp_percentage > 0.6)
    {
      render_text.setColor(text_color);
    }
    else if (hp_percentage > 0.3)
    {
      render_text.setColor(text_warning_color);
    }
    else
    {
      render_text.setColor(text_danger_color);
    }

    StringDisplay hp_string = boost::lexical_cast<StringDisplay>(hp) + L"/" + boost::lexical_cast<StringDisplay>(max_hp);

    render_text.setPosition(origin.x + 30, origin.y + line_spacing);
    render_text.setString(hp_string);
    texture.draw(render_text);

    // Render attributes
    render_attribute(texture, L"XP", "xp", { origin.x, origin.y + (2 * line_spacing) });
    render_attribute(texture, L"STR", "attribute_strength", { origin.x + (0 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"VIG", "attribute_vigilance", { origin.x + (1 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"END", "attribute_endurance", { origin.x + (2 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"CHA", "attribute_charisma", { origin.x + (3 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"INT", "attribute_intelligence", { origin.x + (4 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"AGI", "attribute_agility", { origin.x + (5 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"LUC", "attribute_luck", { origin.x + (6 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"VIT", "attribute_vitality", { origin.x + (7 * attrib_spacing), origin.y + (3 * line_spacing) });
    render_attribute(texture, L"AUR", "attribute_aura", { origin.x + (8 * attrib_spacing), origin.y + (3 * line_spacing) });
  }

  return;
}

void StatusArea::render_attribute(sf::RenderTarget& target, StringDisplay abbrev, StringKey name, sf::Vector2f location)
{
  sf::Text render_text;
  sf::Color text_color = the_config.get<sf::Color>("text_color");
  sf::Color text_dim_color = the_config.get<sf::Color>("text_dim_color");
  ThingId player = GAME.get_player();

  // Render STR
  render_text.setFont(the_default_mono_font);
  render_text.setColor(text_dim_color);
  render_text.setCharacterSize(the_config.get<unsigned int>("text_default_size"));
  render_text.setPosition(location.x, location.y);
  render_text.setString(abbrev + L":");
  target.draw(render_text);

  StringDisplay attr_string = std::to_wstring(player->get_modified_property<int>(name));

  render_text.setColor(text_color);
  render_text.setPosition(location.x + 40, location.y);
  render_text.setString(attr_string);
  target.draw(render_text);
}

StringDisplay StatusArea::get_test_label()
{
  return L"Testing GUILabel";
}