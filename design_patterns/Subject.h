#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

#include "Event.h"
#include "Serializable.h"

// Forward declarations
class Observer;
using ObserverPriority = int32_t;
using ObserversSet = std::unordered_set<Observer*>;
using PrioritizedObservers = std::map<ObserverPriority, ObserversSet>;
using PrioritizedObserversPair = std::pair<ObserverPriority, ObserversSet>;
using EventObservers = std::unordered_map<EventID, PrioritizedObservers>;
using EventObserversPair = std::pair<EventID, PrioritizedObservers>;
using EventQueue = std::queue<std::function<void()>>;

/// Subject declaration for observer pattern.
/// Adapted from http://0xfede.io/2015/12/13/T-C++-ObserverPattern.html
class Subject : public virtual Serializable
{

public:
  struct Registration : public ConcreteEvent<Registration>
  {
    enum class State { Registered, Unregistered } state;
    void serialize(std::ostream& o) const;
  };

  Subject();
  virtual ~Subject();

  void addObserver(Observer& observer, EventID eventID, ObserverPriority priority = 0);
  void removeObserver(Observer& observer, EventID eventID = EventID::All);

  /// Return the set of events this Subject provides.
  /// By default, returns nothing. Must be overridden with the events
  /// declared in the Subject subclass.
  using RegisteredEventsDelegate = std::function<std::unordered_set<EventID>()>;
  virtual std::unordered_set<EventID> registeredEvents() const;

protected:
  using BroadcastDelegate = std::function<void(Event& event, bool shouldSend)>;
  using UnicastDelegate = std::function<void(Event& event, Observer& observer, bool shouldSend)>;

  void broadcast(Event& event);
  void unicast(Event & event, Observer & observer);

  size_t getObserverCount(EventID eventID) const;
  PrioritizedObservers& getObservers(EventID eventID);
  PrioritizedObservers const& getObservers(EventID eventID) const;
  bool Subject::observerIsObservingEvent(Observer& observer, EventID eventID) const;

  virtual void broadcast_(Event& event, BroadcastDelegate do_broadcast);
  virtual void unicast_(Event& event, Observer& observer, UnicastDelegate do_unicast);

private:
  class Impl;
  const std::unique_ptr<Impl> pImpl;
};