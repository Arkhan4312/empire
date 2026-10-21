#include <empire/engine/core/events/EventBus.h>

#include <algorithm>

namespace empire::events {

namespace {
std::atomic<EventBus*> g_defaultBus{nullptr};


}  // namespace

SubscriptionId EventBus::subscribeCategories(EventCategory mask,
                                             std::function<void(Event&)> fn,
                                             int priority,
                                             bool receiveIfHandled) {
    auto sub = std::make_shared<Subscription>();
    sub->priority = priority;
    sub->receiveIfHandled = receiveIfHandled;
    sub->isCategory = true;
    sub->categoryMask = mask;
    sub->fn = std::move(fn);

    std::lock_guard lock(m_mutex);
    sub->id = nextId(m_nextIndex);
    const SubscriptionId id = sub->id;
    m_byIndex.emplace(id.index, sub);
    m_byCategory.push_back(id);
    std::stable_sort(m_byCategory.begin(), m_byCategory.end(),
                     [this](SubscriptionId a, SubscriptionId b) {
                         return m_byIndex[a.index]->priority >
                                m_byIndex[b.index]->priority;
                     });
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    if (!id.isValid()) {
        return;
    }
    std::lock_guard lock(m_mutex);

    auto it = m_byIndex.find(id.index);
    if (it == m_byIndex.end()) {
        return;
    }

    auto sub = std::move(it->second);
    sub->alive.store(false, std::memory_order_release);
    m_byIndex.erase(it);

    if (sub->isCategory) {
        std::erase(m_byCategory, id);
    } else {
        if (auto tit = m_byType.find(sub->type); tit != m_byType.end()) {
            std::erase(tit->second, id);
        }
    }
}

void EventBus::pumpDeferred(std::size_t maxEvents) {
    std::size_t processed = 0;
    for (std::size_t p = 0; p < m_deferredQueues.size(); ++p) {
        for (;;) {
            if (processed >= maxEvents) {
                return;
            }
            Deferred d{typeid(void), nullptr};
            {
                std::lock_guard lock(m_deferredMutex);
                auto& q = m_deferredQueues[p];
                if (q.empty()) {
                    break;
                }
                d = std::move(q.front());
                q.pop();
            }
            if (d.event) {
                dispatch(d.type, *d.event);
            }
            ++processed;
        }
    }
}

std::size_t EventBus::pendingDeferred() const noexcept {
    std::lock_guard lock(m_deferredMutex);
    std::size_t total = 0;
    for (auto& q : m_deferredQueues) {
        total += q.size();
    }
    return total;
}

void EventBus::clear() {
    {
        std::lock_guard lock(m_mutex);
        m_byIndex.clear();
        m_byType.clear();
        m_byCategory.clear();
    }
    {
        std::lock_guard lock(m_deferredMutex);
        for (auto& q : m_deferredQueues) {
            std::queue<Deferred> empty;
            q.swap(empty);
        }
    }
}

void EventBus::dispatch(std::type_index type, Event& event) {
    std::vector<std::shared_ptr<Subscription>> typed;
    std::vector<std::shared_ptr<Subscription>> categorized;
    {
        std::lock_guard lock(m_mutex);
        if (auto it = m_byType.find(type); it != m_byType.end()) {
            typed.reserve(it->second.size());
            for (auto h : it->second) {
                if (auto sit = m_byIndex.find(h.index);
                    sit != m_byIndex.end()) {
                    typed.push_back(sit->second);
                }
            }
        }
        categorized.reserve(m_byCategory.size());
        for (auto h : m_byCategory) {
            if (auto sit = m_byIndex.find(h.index); sit != m_byIndex.end()) {
                categorized.push_back(sit->second);
            }
        }
    }

    auto run = [&](const std::shared_ptr<Subscription>& sub, bool catFilter) {
        if (!sub->alive.load(std::memory_order_acquire)) {
            return;
        }
        if (event.isConsumed() && !sub->receiveIfHandled) {
            return;
        }
        if (catFilter && !anyOf(event.category(), sub->categoryMask)) {
            return;
        }
        try {
            sub->fn(event);
        } catch (...) {
        }
    };

    for (auto& s : typed) {
        run(s, /*catFilter=*/false);
    }
    for (auto& s : categorized) {
        run(s, /*catFilter=*/true);
    }
}

EventBus* defaultEventBus() noexcept {
    return g_defaultBus.load(std::memory_order_acquire);
}

void setDefaultEventBus(EventBus* bus) noexcept {
    g_defaultBus.store(bus, std::memory_order_release);
}

}  // namespace empire::events