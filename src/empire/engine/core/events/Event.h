#pragma once

#include <cstdint>

namespace empire::events {

enum class EventCategory : std::uint32_t {
    None = 0,
    Core = 1u << 0,
    Input = 1u << 1,
    Audio = 1u << 2,
    Render = 1u << 3,
    Asset = 1u << 4,
    Physics = 1u << 5,
    Game = 1u << 6,
    UI = 1u << 7,
    Debug = 1u << 8,
};

constexpr EventCategory operator|(EventCategory a, EventCategory b) noexcept {
    return static_cast<EventCategory>(static_cast<std::uint32_t>(a) |
                                      static_cast<std::uint32_t>(b));
}
constexpr bool anyOf(EventCategory e, EventCategory mask) noexcept {
    return (static_cast<std::uint32_t>(e) & static_cast<std::uint32_t>(mask)) !=
           0;
}

enum class EventPriority : std::uint8_t {
    Immediate = 0,
    High = 1,
    Normal = 2,
    Low = 3,
    Idle = 4,
};

class Event {
public:
    explicit Event(EventCategory cat = EventCategory::None) noexcept
        : m_category(cat) {
    }
    virtual ~Event() = default;

    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    Event(Event&& other) noexcept : m_category(other.m_category), m_handled(other.m_handled) {}
    Event& operator=(Event&& other) noexcept {
        if (this != &other) {
            m_category = other.m_category;
            m_handled = other.m_handled;
        }
        return *this;
    }
    EventCategory category() const noexcept {
        return m_category;
    }

    void consume() noexcept {
        m_handled = true;
    }
    bool isConsumed() const noexcept {
        return m_handled;
    }

private:
    EventCategory m_category;
    bool m_handled = false;
};
}  // namespace empire::events