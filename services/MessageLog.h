#ifndef MESSAGELOG_H
#define MESSAGELOG_H

#include "stdafx.h"

#include "services/IMessageLog.h"

/// A class that keeps track of game messages, and is renderable on-screen.
class MessageLog : public IMessageLog
{
public:
  MessageLog();

  virtual ~MessageLog();

  /// Add a message to the message log.
  /// The message added is automatically capitalized if it isn't already.
  virtual void add(std::string message) override;

  /// Get the maximum size of the message queue.
  virtual unsigned int getMessageQueueSize() override;

  /// Get a reference to the message queue.
  virtual std::deque<std::string>& getMessageQueue() override;

protected:

private:
  struct Impl;
  std::unique_ptr<Impl> pImpl;
};

#endif // MESSAGELOG_H