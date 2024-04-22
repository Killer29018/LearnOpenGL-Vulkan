#include "EventHandler.hpp"

void EventDispatcher::attachObserver(EventObserver* observer) { m_Listeners.push_back(observer); }

void EventDispatcher::dispatchEvent(const Event* event)
{
    for (auto it = m_Listeners.begin(); it != m_Listeners.end(); it++)
    {
        (*it)->receiveEvent(event);
    }
}
