#pragma once

#include <cstdint>

enum class EventType : uint32_t {
    NONE,
    KEYBOARD_PRESS,
};

struct Event {
    virtual constexpr EventType getType() const { return EventType::NONE; }
};

struct KeyboardPressEvent : public Event {
    int keyType;
    int keyAction;
    int mods;

    virtual constexpr EventType getType() const { return EventType::KEYBOARD_PRESS; }
};
