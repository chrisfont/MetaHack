#pragma once

#include "AssertHelper.h"

#include "json.hpp"
using json = ::nlohmann::json;

#include "SFML/Graphics.hpp"

/// Definition of a two-dimensional vector.
template <typename T>
class Vec2
{
public:
  Vec2()
  {}

  Vec2(T x_, T y_)
    :
    x{ x_ }, y{ y_ }
  {}

  template <typename U>
  Vec2(sf::Vector2<U> vec)
    :
    x{ static_cast<T>(vec.x) }, y{ static_cast<T>(vec.y) }
  {}

  virtual ~Vec2() = default;
  Vec2(Vec2 const& other) = default;
  Vec2(Vec2&& other) = default;
  Vec2& operator=(Vec2 const& other) = default;
  Vec2& operator=(Vec2&& other) = default;

  Vec2& operator+=(Vec2 const& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }

  Vec2& operator-=(Vec2 const& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }

  friend Vec2 operator+(Vec2 lhs, Vec2 const& rhs)
  {
    lhs += rhs;
    return lhs;
  }

  friend Vec2 operator-(Vec2 lhs, Vec2 const& rhs)
  {
    lhs -= rhs;
    return lhs;
  }

  friend Vec2 operator*(Vec2 lhs, T rhs)
  {
    lhs.x *= rhs;
    lhs.y *= rhs;
    return *lhs;
  }

  friend Vec2 operator/(Vec2 lhs, T rhs)
  {
    lhs.x /= rhs;
    lhs.y /= rhs;
    return *lhs;
  }

  friend bool operator==(Vec2 const& lhs, Vec2 const& rhs)
  {
    return ((lhs.x == rhs.x) && (lhs.y == rhs.y));
  }

  friend bool operator!=(Vec2 const& lhs, Vec2 const& rhs)
  {
    return !(lhs == rhs);
  }

  friend std::ostream& operator<<(std::ostream& os, Vec2 const& obj)
  {
    os << "(" << obj.x << ", " << obj.y << ")";
    return os;
  }

  template<typename X>
  friend void to_json(json& j, Vec2<X> const& obj);

  friend void from_json(json const& j, Vec2& obj)
  {
    if (j.is_number())
    {
      T number = j;
      LOG(WARNING) << "Converting number " << number <<
        " into a Vec2<>(" << number << ", " << number <<
        "); this will work but may not be what you were looking for!";
      obj.x = number;
      obj.y = number;
    }
    else if (j.is_array() && (j[0] == "realvec2" || j[0] == "uintvec2" || j[0] == "intvec2"))
    {
      obj.x = j[1];
      obj.y = j[2];
    }
    else
    {
      LOG(FATAL) << "Attempted to create a Vec2<> out of an invalid JSON object: " << j.dump();
    }
  }

  T area()
  {
    return x * y;
  }

  T perimeter()
  {
    return (x * 2) + (y * 2);
  }

  T r()
  {
    return static_cast<T>(sqrt((x * x) + (y * y)));
  }

  double theta()
  {
    return atan(static_cast<double>(y) / static_cast<double>(x));
  }

  static T square_distance(Vec2 const& first, Vec2 const& second)
  {
    auto xDiff = (first.x > second.x) ? (first.x - second.x) : (second.x - first.x);
    auto yDiff = (first.y > second.y) ? (first.y - second.y) : (second.y - first.y);

    return (xDiff * xDiff) + (yDiff * yDiff);
  }

  static T distance(Vec2 const& first, Vec2 const& second)
  {
    return static_cast<T>(sqrt(square_distance(first, second)));
  }

  template <typename U>
  operator sf::Vector2<U>() const
  {
    return{ static_cast<U>(x), static_cast<U>(y) };
  }

  T x, y;
};


namespace std
{
  template<typename T> struct hash<Vec2<T>>
  {
    using argument_type = Vec2<T>;
    using result_type = std::size_t;
    result_type operator()(argument_type const& obj) const
    {
      result_type const h1(std::hash<T>{}(obj.x));
      result_type const h2(std::hash<T>{}(obj.y));
      return h1 ^ (h2 << 1);
    }
  };
}


template<typename T>
void to_json(json& j, Vec2<T> const& obj);

void to_json(json& j, Vec2<float> const& obj);
void to_json(json& j, Vec2<unsigned int> const& obj);
void to_json(json& j, Vec2<int> const& obj);

using RealVec2 = Vec2<float>;
using IntVec2 = Vec2<int32_t>;
using UintVec2 = Vec2<uint32_t>;
