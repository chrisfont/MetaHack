#include "stdafx.h"

#include "GUIObject.h"

#include <exception>

#include "App.h"
#include "IConfigSettings.h"
#include "Service.h"

namespace metagui
{
  Object::Object(std::string name, Vec2i location, Vec2u size)
  {
    SET_UP_LOGGER("GUI", true);

    m_name = name;
    m_parent = nullptr;
    setRelativeLocation(location);
    setSize(size);
  }

  Object::Object(std::string name, sf::IntRect dimensions)
  {
    SET_UP_LOGGER("GUI", true);

    m_name = name;
    m_parent = nullptr;
    setRelativeDimensions(dimensions);
  }

  Object::~Object()
  {
    //dtor
  }

  std::string Object::getName()
  {
    return m_name;
  }

  void Object::setFocus(bool focus)
  {
    if ((m_cached_flags.disabled == false) || (m_cached_flags.hidden == false))
    {
      if (m_parent != nullptr)
      {
        m_parent->clearChildFocuses();
      }

      setFocusOnly(focus);
    }
  }

  bool Object::getFocus()
  {
    return m_focus;
  }

  bool Object::getGlobalFocus()
  {
    bool global_focus = m_focus;

    if ((global_focus == true) && (m_parent != nullptr))
    {
      global_focus &= m_parent->getGlobalFocus();
    }

    return global_focus;
  }

  void Object::setGlobalFocus(bool focus)
  {
    if ((m_cached_flags.disabled == false) || (m_cached_flags.hidden == false))
    {
      if (m_parent != nullptr)
      {
        m_parent->setGlobalFocus(focus);
      }

      setFocus(focus);
    }
  }

  void Object::setText(std::string text)
  {
    if (m_text != text) flagForRedraw();
    m_text = text;
  }

  std::string Object::getText()
  {
    return m_text;
  }

  Vec2i Object::getRelativeLocation()
  {
    return m_location;
  }

  void Object::setRelativeLocation(Vec2i location)
  {
    m_location = location;
    if (m_parent != nullptr)
    {
      m_parent->flagForRedraw();
    }
  }

  Vec2u Object::getSize()
  {
    return m_size;
  }

  /// @todo Minimum texture size, configurable by subclass.
  void Object::setSize(Vec2u size)
  {
    // Do nothing if requested size is same as current size.
    if (m_size == size) return;

    m_size = size;

    // Would be better not to create a texture if size is 0 in either dimension
    // but for the moment this is faster.
    /// @todo See if we can make this more elegant.
    if (size.x < 1) size.x = 1;
    if (size.y < 1) size.y = 1;

    if (m_parent != nullptr)
    {
      auto max_size = ((m_cached_flags.decor == true) ?
                       m_parent->getSize() :
                       m_parent->getChildAreaSize());

      if (size.x > max_size.x) size.x = max_size.x;
      if (size.y > max_size.y) size.y = max_size.y;
    }

    // Texture size should be the nearest power-of-2, for speed.
    Vec2u new_texture_size{ next_power_of_two(size.x), next_power_of_two(size.y) };
    if (new_texture_size != m_bg_texture_size)
    {
      m_bg_texture_size = new_texture_size;
      m_bg_texture.reset(NEW sf::RenderTexture());
      m_bg_texture->create(m_bg_texture_size.x, m_bg_texture_size.y);
    }
    
    // Set "dirty" for both this object and its parent.
    // (Right now flagForRedraw() will automatically set the flag on the 
    //  parent chain, but I'll leave the other line commented out just in
    //  case I change the behavior later.)
    flagForRedraw();
    //if (m_parent != nullptr) m_parent->flagForRedraw();

    // Inform children of the parent size change.
    for (auto& child : m_children)
    {
      child.second->handleParentSizeChanged_(size);
    }
  }

  sf::IntRect Object::getRelativeDimensions()
  {
    sf::IntRect dimensions;
    Vec2i location = getRelativeLocation();
    Vec2u size = getSize();
    dimensions.left = location.x;
    dimensions.top = location.y;
    dimensions.width = size.x;
    dimensions.height = size.y;
    return dimensions;
  }

  void Object::setRelativeDimensions(sf::IntRect dimensions)
  {
    setRelativeLocation({ dimensions.left, dimensions.top });
    setSize({ static_cast<unsigned int>(dimensions.width),
               static_cast<unsigned int>(dimensions.height) });
  }

  Vec2i Object::getAbsoluteLocation()
  {
    Vec2i absolute_location = getRelativeLocation();

    if (m_parent != nullptr)
    {
      Vec2i child_area_absolute_location =
        m_parent->getAbsoluteLocation();

      if (m_cached_flags.decor != true)
      {
        child_area_absolute_location += m_parent->getChildAreaLocation();
      }

      absolute_location += child_area_absolute_location;
    }

    return absolute_location;
  }

  void Object::setAbsoluteLocation(Vec2i location)
  {
    Vec2i relative_location = location;

    if (m_parent != nullptr)
    {
      Vec2i child_area_absolute_location =
        m_parent->getAbsoluteLocation();

      if (m_cached_flags.decor != true)
      {
        child_area_absolute_location += m_parent->getChildAreaLocation();
      }

      relative_location -= child_area_absolute_location;
    }

    setRelativeLocation(relative_location);
  }

  sf::IntRect Object::getAbsoluteDimensions()
  {
    sf::IntRect dimensions;
    Vec2i location = getAbsoluteLocation();
    Vec2u size = getSize();
    dimensions.left = location.x;
    dimensions.top = location.y;
    dimensions.width = size.x;
    dimensions.height = size.y;
    return dimensions;
  }

  Object& Object::addChild(std::unique_ptr<Object> child, uint32_t z_order)
  {
    ASSERT_CONDITION(child);

    std::string name = child->getName();

    if (childExists(name))
    {
      throw std::runtime_error("Tried to add already-present child \"" + name + "\" of GUI object \"" + getName() + "\"");
    }

    Object& child_ref = *child;
    child->setParent(this);

    // This odd syntax is in order to work around VS compiler bug when having
    // a unique_ptr as a map value. See:
    // https://stackoverflow.com/questions/21056872/c-stdunique-ptr-wont-compile-in-map
    m_children.insert<ChildMap::value_type>(ChildMap::value_type(name, std::move(child)));
    m_zorder_map.insert({ z_order, name });

    child_ref.handleParentSizeChanged_(getSize());

    CLOG(TRACE, "GUI") << "Added child \"" << name <<
      "\" (with Z-order " << z_order <<
      ") to parent \"" << getName() << "\"";

    return child_ref;
  }

  Object& Object::addChild(std::unique_ptr<Object> child)
  {
    uint32_t z_order = getHighestChildZOrder() + 1;
    return addChild(std::move(child), z_order);
  }

  Object & Object::addChildTop(std::unique_ptr<Object> child)
  {
    uint32_t z_order = getLowestChildZOrder() - 1;
    return addChild(std::move(child), z_order);
  }

  bool Object::childExists(std::string name)
  {
    return (m_children.count(name) > 0);
  }

  Object& Object::getChild(std::string name)
  {
    if (childExists(name))
    {
      return *m_children.at(name);
    }

    throw std::runtime_error("Tried to get non-existent child \"" + name + "\" of GUI object \"" + getName() + "\"");
  }

  std::unique_ptr<Object> Object::removeChild(std::string name)
  {
    if (childExists(name))
    {
      // This moves the object out of the stored unique_ptr.
      std::unique_ptr<Object> moved_object = std::move(m_children.at(name));
      // This erases the (now empty) unique_ptr from the map.
      m_children.erase(name);

      // This finds the child name in the Z-order map and gets rid of it.
      for (auto& iter = m_zorder_map.begin(); iter != m_zorder_map.end(); ++iter)
      {
        if (iter->second == name)
        {
          m_zorder_map.erase(iter);
          break;
        }
      }

      // This returns the object we removed.
      return std::move(moved_object);
    }

    // Didn't find the object, so return an empty unique_ptr.
    return std::unique_ptr<Object>();
  }

  uint32_t Object::getLowestChildZOrder()
  {
    if (m_zorder_map.size() > 0)
    {
      auto iter = m_zorder_map.cbegin();
      return iter->first;
    }
    return 0;
  }

  uint32_t Object::getHighestChildZOrder()
  {
    if (m_zorder_map.size() > 0)
    {
      auto iter = m_zorder_map.cend();
      --iter;
      return iter->first;
    }
    return 0;
  }

  void Object::clearChildren()
  {
    m_children.clear();
    m_zorder_map.clear();
  }

  Vec2i Object::getChildAreaLocation()
  {
    return{ 0, 0 };
  }

  Vec2u Object::getChildAreaSize()
  {
    return getSize();
  }

  bool Object::render(sf::RenderTexture& parent_texture, int frame)
  {
    sf::RenderTexture& our_texture = *m_bg_texture;

    if (m_cached_flags.hidden == false)
    {
      if (m_flag_for_redraw == true)
      {
        our_texture.clear(sf::Color::Transparent);
        draw(frame);
        m_flag_for_redraw = false;
      }

      // Create the RectangleShape that will be drawn onto the target.
      m_bg_shape.setPosition(Vec2f(static_cast<float>(m_location.x), static_cast<float>(m_location.y)));
      m_bg_shape.setSize(Vec2f(static_cast<float>(m_size.x), static_cast<float>(m_size.y)));
      m_bg_shape.setTexture(&(m_bg_texture->getTexture()));
      m_bg_shape.setTextureRect(sf::IntRect(0, 0, m_size.x, m_size.y));

      // Draw onto the parent.
      parent_texture.setView(sf::View(sf::FloatRect(0.0f, 0.0f, static_cast<float>(parent_texture.getSize().x), static_cast<float>(parent_texture.getSize().y))));
      parent_texture.draw(m_bg_shape);

      parent_texture.display();
      return true;
    }
    else
    {
      return false;
    }
  }

  void Object::draw(int frame)
  {
    sf::RenderTexture& our_texture = *m_bg_texture;

    /// Render self to our bg texture.
    drawPreChildren_(our_texture, frame);

    /// Render all child objects to our bg texture.
    for (auto& z_pair : m_zorder_map)
    {
      m_children.at(z_pair.second)->render(our_texture, frame);
    }

    /// Render self after children are done.
    drawPostChildren_(our_texture, frame);

    our_texture.display();
  }

  void Object::flagForRedraw()
  {
    m_flag_for_redraw = true;
    if (m_parent)
    {
      m_parent->flagForRedraw();
    }
  }

  void Object::setFlag(std::string name, bool value)
  {
    if ((m_flags.count(name) == 0) || (m_flags[name] != value))
    {
      m_flags[name] = value;
      handleSetFlag(name, value);
    }
  }

  bool Object::getFlag(std::string name, bool default_value)
  {
    if (m_flags.count(name) == 0)
    {
      setFlag(name, default_value);
    }
    return m_flags[name];
  }

  void Object::handleSetFlag(std::string name, bool value)
  {
    if (name == "hidden")
    {
      if (value == true)
      {
        setFocus(false);
      }
      m_cached_flags.hidden = value;
    }
    else if (name == "disabled")
    {
      if (value == true)
      {
        setFocus(false);
      }
      m_cached_flags.disabled = value;
    }
    else if (name == "movable")
    {
      m_cached_flags.movable = value;
    }
    else if (name == "decor")
    {
      m_cached_flags.decor = value;
    }

    handleSetFlag_(name, value);
  }

  bool Object::containsPoint(Vec2i point)
  {
    auto left = getAbsoluteLocation().x;
    auto top = getAbsoluteLocation().y;
    auto right = left + (static_cast<int>(getSize().x) - 1);
    auto bottom = top + (static_cast<int>(getSize().y) - 1);

    return ((point.x >= left) && (point.x <= right) &&
      (point.y >= top) && (point.y <= bottom));
  }

  Event::Result Object::handleGUIEventPreChildren(EventDragFinished& event)
  {
    Event::Result result = handleGUIEventPreChildren_(event);

    return result;
  }

  Event::Result Object::handleGUIEventPostChildren(EventDragFinished& event)
  {
    m_being_dragged = false;

    Event::Result result = handleGUIEventPostChildren_(event);

    return result;
  }

  Event::Result Object::handleGUIEventPreChildren(EventDragStarted& event)
  {
    Event::Result result;

    if (containsPoint(event.start_location))
    {
      result = handleGUIEventPreChildren_(event);
    }
    else
    {
      // We "Ignore" the event so it is not passed to children.
      result = Event::Result::Ignored;
    }

    return result;
  }

  Event::Result Object::handleGUIEventPostChildren(EventDragStarted& event)
  {
    Event::Result result;

    if (containsPoint(event.start_location))
    {
      m_being_dragged = true;
      m_drag_start_location = event.start_location;
      m_absolute_location_drag_start = getAbsoluteLocation();

      result = handleGUIEventPostChildren_(event);
    }
    else
    {
      // We "Ignore" the event so it is not passed to children.
      result = Event::Result::Ignored;
    }

    return result;
  }

  Event::Result Object::handleGUIEventPreChildren(EventDragging& event)
  {
    // We "Acknowledge" the event so it is passed to children.
    return handleGUIEventPreChildren_(event);
  }

  Event::Result Object::handleGUIEventPostChildren(EventDragging& event)
  {
    Event::Result result = handleGUIEventPostChildren_(event);

    if (result != Event::Result::Handled)
    {
      // If we got here, all children ignored the event (or there are no
      // children), and there's no subclass override -- so we want to 
      // process it if we are movable.
      if ((m_being_dragged == true) && (m_cached_flags.movable == true))
      {
        auto move_amount = event.current_location - m_drag_start_location;
        auto new_coords = m_absolute_location_drag_start + move_amount;

        setAbsoluteLocation(new_coords);
        result = Event::Result::Handled;
      }
      else
      {
        result = Event::Result::Ignored;
      }
    }

    return result;
  }

  Event::Result Object::handleGUIEventPreChildren(EventKeyPressed& event)
  {
    return handleGUIEventPreChildren_(event);
  }

  Event::Result Object::handleGUIEventPostChildren(EventKeyPressed& event)
  {
    return handleGUIEventPostChildren_(event);
  }

  Event::Result Object::handleGUIEventPreChildren(EventMouseDown& event)
  {
    return handleGUIEventPreChildren_(event);
  }

  Event::Result Object::handleGUIEventPostChildren(EventMouseDown& event)
  {
    return handleGUIEventPostChildren_(event);
  }

  Event::Result Object::handleGUIEventPreChildren(EventResized& event)
  {
    return handleGUIEventPreChildren_(event);
  }

  Event::Result Object::handleGUIEventPostChildren(EventResized& event)
  {
    return handleGUIEventPostChildren_(event);
  }

  Object * Object::getParent()
  {
    return m_parent;
  }

  void Object::setParent(Object* parent)
  {
    m_parent = parent;
    m_parent->flagForRedraw();
  }

  void Object::clearChildFocuses()
  {
    for (auto& child_pair : m_children)
    {
      child_pair.second->setFocusOnly(false);
    }
  }

  void Object::setFocusOnly(bool focus)
  {
    if (m_focus != focus) flagForRedraw();
    m_focus = focus;
  }

  bool Object::isBeingDragged()
  {
    return m_being_dragged;
  }

  Vec2i Object::getDragStartLocation()
  {
    return m_drag_start_location;
  }

  void Object::drawPreChildren_(sf::RenderTexture& texture, int frame)
  {
    // Default behavior is to do nothing.
  }

  void Object::drawPostChildren_(sf::RenderTexture& texture, int frame)
  {
    // Default behavior is to do nothing.
  }

  Event::Result Object::handleGUIEventPreChildren_(EventDragFinished& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventDragFinished& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPreChildren_(EventDragStarted& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventDragStarted& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPreChildren_(EventDragging& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventDragging& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPreChildren_(EventKeyPressed& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventKeyPressed& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPreChildren_(EventMouseDown& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventMouseDown& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPreChildren_(EventResized& event)
  {
    return Event::Result::Acknowledged;
  }

  Event::Result Object::handleGUIEventPostChildren_(EventResized& event)
  {
    return Event::Result::Acknowledged;
  }

  void Object::handleSetFlag_(std::string name, bool enabled)
  {}

  void Object::handleParentSizeChanged_(Vec2u parent_size)
  {
  }
}; // end namespace metagui