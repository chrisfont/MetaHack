#ifndef DIRECTION_H
#define DIRECTION_H

#include <iostream>

enum class Direction
{
  None,
  North,
  Northeast,
  East,
  Southeast,
  South,
  Southwest,
  West,
  Northwest,
  Up,
  Down,
  Self,
  Count
};

inline std::ostream& operator<<(std::ostream& os, const Direction& d)
{
  switch (d)
  {
  case Direction::None:       os << "None"; break;
  case Direction::North:      os << "North"; break;
  case Direction::Northeast:  os << "Northeast"; break;
  case Direction::East:       os << "East"; break;
  case Direction::Southeast:  os << "Southeast"; break;
  case Direction::South:      os << "South"; break;
  case Direction::Southwest:  os << "Southwest"; break;
  case Direction::West:       os << "West"; break;
  case Direction::Northwest:  os << "Northwest"; break;
  case Direction::Up:         os << "Up"; break;
  case Direction::Down:       os << "Down"; break;
  case Direction::Self:       os << "Self"; break;
  case Direction::Count:      os << "Count"; break;
  default:                    os << "???"; break;
  }

  return os;
}

inline Direction update_direction(Direction current_direction,
                                  Direction new_direction)
{
  switch (new_direction)
  {
  case Direction::North:
  case Direction::East:
  case Direction::South:
  case Direction::West:
    return new_direction;
  case Direction::Northeast:
    if (current_direction == Direction::North) return Direction::North;
    if (current_direction == Direction::East) return Direction::East;
    if (current_direction == Direction::South) return Direction::East;
    if (current_direction == Direction::West) return Direction::North;
    return current_direction;
  case Direction::Southeast:
    if (current_direction == Direction::North) return Direction::East;
    if (current_direction == Direction::East) return Direction::East;
    if (current_direction == Direction::South) return Direction::South;
    if (current_direction == Direction::West) return Direction::South;
    return current_direction;
  case Direction::Southwest:
    if (current_direction == Direction::North) return Direction::West;
    if (current_direction == Direction::East) return Direction::South;
    if (current_direction == Direction::South) return Direction::South;
    if (current_direction == Direction::West) return Direction::West;
    return current_direction;
  case Direction::Northwest:
    if (current_direction == Direction::North) return Direction::North;
    if (current_direction == Direction::East) return Direction::North;
    if (current_direction == Direction::South) return Direction::West;
    if (current_direction == Direction::West) return Direction::West;
    return current_direction;
  default:
    return current_direction;
  }
}

inline int get_appropriate_4way_tile(Direction direction)
{
  switch (direction)
  {
    case Direction::North: return 0;
    case Direction::East: return 1;
    case Direction::South: return 2;
    case Direction::West: return 3;
    default: return 2; // Default to facing player if direction isn't supported
  }
}

inline int get_x_offset(Direction direction)
{
  if ((direction == Direction::Northeast) ||
      (direction == Direction::East) ||
      (direction == Direction::Southeast))
  {
    return 1;
  }
  if ((direction == Direction::Northwest) ||
      (direction == Direction::West) ||
      (direction == Direction::Southwest))
  {
    return -1;
  }
  return 0;
}

inline int get_y_offset(Direction direction)
{
  if ((direction == Direction::Northwest) ||
      (direction == Direction::North) ||
      (direction == Direction::Northeast))
  {
    return -1;
  }
  if ((direction == Direction::Southwest) ||
      (direction == Direction::South) ||
      (direction == Direction::Southeast))
  {
    return 1;
  }
  return 0;
}

#endif // DIRECTION_H
