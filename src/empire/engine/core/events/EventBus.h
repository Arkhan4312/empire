#pragma once

#include <empire/engine/core/events/Event.h>
#include <empire/engine/util/Handle.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace empire::events {

struct SubscriptionTag {};
using SubscriptionId = util::Handle<SubscriptionTag>;
inline constexpr SubscriptionId kInvalidSubscriptionId{};
namespace {
SubscriptionId nextId(std::atomic<std::uint32_t>& counter) noexcept {
    std::uint32_t idx = counter.fetch_add(1, std::memory_order_relaxed);
    if (idx == util::Handle<SubscriptionTag>::kInvalidIndex) {
        idx = counter.fetch_add(1, std::memory_order_relaxed);
    }
    return SubscriptionId{idx, 1};
}
}  // namespace
class EventBus {
public:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;

    template <typename E, typename Fn>
    [[nodiscard]] SubscriptionId subscribe(Fn&& fn, int priority = 0,
                                           bool receiveIfHandled = false) {
        static_assert(std::is_base_of_v<Event, E>);

        auto sub = std::make_shared<Subscription>();
        sub->priority = priority;
        sub->receiveIfHandled = receiveIfHandled;
        sub->isCategory = false;
        sub->type = typeid(E);
        sub->fn = [fn = std::forward<Fn>(fn)](Event& e) mutable {
            std::invoke(fn, static_cast<E&>(e));
        };

        std::lock_guard lock(m_mutex);
        sub->id = nextId(m_nextIndex);
        const SubscriptionId id = sub->id;
        m_byIndex.emplace(id.index, sub);

        auto& vec = m_byType[typeid(E)];
        vec.push_back(id);
        std::stable_sort(vec.begin(), vec.end(),
                         [this](SubscriptionId a, SubscriptionId b) {
                             return m_byIndex[a.index]->priority >
                                    m_byIndex[b.index]->priority;
                         });
        return id;
    }

    [[nodiscard]] SubscriptionId subscribeCategories(
        EventCategory mask, std::function<void(Event&)> fn, int priority = 0,
        bool receiveIfHandled = true);

    void unsubscribe(SubscriptionId id);

    template <typename E>
    void publish(E& event) {
        static_assert(std::is_base_of_v<Event, E>);
        dispatch(typeid(E), event);
    }

    template <typename E>
    void publishDeferred(std::unique_ptr<E> event,
                         EventPriority prio = EventPriority::Normal) {
        static_assert(std::is_base_of_v<Event, E>);
        if (!event) return;
        std::lock_guard lock(m_deferredMutex);
        auto& q = m_deferredQueues[static_cast<std::size_t>(prio)];
        q.push({typeid(E), std::move(event)});
    }

    template <typename E, typename... Args>
    void emplaceDeferred(EventPriority prio, Args&&... args) {
        static_assert(std::is_base_of_v<Event, E>);
        publishDeferred(std::make_unique<E>(std::forward<Args>(args)...), prio);
    }

    template <typename E, typename... Args>
    void emplaceDeferred(Args&&... args) {
        emplaceDeferred<E>(EventPriority::Normal, std::forward<Args>(args)...);
    }

    void pumpDeferred(std::size_t maxEvents = 1024);

    std::size_t pendingDeferred() const noexcept;
    void clear();

private:
    struct Subscription {
        SubscriptionId id{};
        int priority = 0;
        bool receiveIfHandled = false;
        bool isCategory = false;
        std::type_index type = typeid(void);
        EventCategory categoryMask = EventCategory::None;
        std::atomic<bool> alive{true};
        std::function<void(Event&)> fn;
    };

    struct Deferred {
        std::type_index type;
        std::unique_ptr<Event> event;
    };

    void dispatch(std::type_index type, Event& event);

    std::atomic<std::uint32_t> m_nextIndex{1};

    mutable std::mutex m_mutex;
    std::unordered_map<std::uint32_t, std::shared_ptr<Subscription>> m_byIndex;
    std::unordered_map<std::type_index, std::vector<SubscriptionId>> m_byType;
    std::vector<SubscriptionId> m_byCategory;

    mutable std::mutex m_deferredMutex;
    std::array<std::queue<Deferred>, 5> m_deferredQueues;
};

EventBus* defaultEventBus() noexcept;
void setDefaultEventBus(EventBus* bus) noexcept;

class ScopedEventBus {
public:
    explicit ScopedEventBus(EventBus* bus) noexcept
        : m_prev(defaultEventBus()) {
        setDefaultEventBus(bus);
    }
    ~ScopedEventBus() {
        setDefaultEventBus(m_prev);
    }
    ScopedEventBus(const ScopedEventBus&) = delete;
    ScopedEventBus& operator=(const ScopedEventBus&) = delete;

private:
    EventBus* m_prev;
};
}  // namespace empire::events