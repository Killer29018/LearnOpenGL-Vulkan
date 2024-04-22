#pragma once

#include <memory>
#include <vector>

#include "Events.hpp"

class EventObserver
{
  public:
    virtual void receiveEvent(const Event* event) {}
};

class EventDispatcher
{
  public:
    virtual void attachObserver(EventObserver* observer);
    virtual void dispatchEvent(const Event* event);

  private:
    std::vector<EventObserver*> m_Listeners;
};
