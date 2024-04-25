#pragma once

#include <cstdint>

enum class EventType : uint32_t { NONE, KEYBOARD_PRESS, MOUSE_MOVE };

struct Event {
    virtual constexpr EventType getType() const { return EventType::NONE; }
};

struct KeyboardPressEvent : public Event {
    int keyType;
    int keyAction;
    int mods;

    virtual constexpr EventType getType() const { return EventType::KEYBOARD_PRESS; }
};

struct MouseMoveEvent : public Event {
    double xOffset;
    double yOffset;
    double xPos;
    double yPos;

    virtual constexpr EventType getType() const { return EventType::MOUSE_MOVE; }
};
