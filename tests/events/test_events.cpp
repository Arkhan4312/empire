#include <catch2/catch_test_macros.hpp>

#include <empire/engine/core/events/Event.h>
#include <empire/engine/core/events/EventBus.h>

#include <string>
#include <vector>

using namespace empire::events;

namespace {

    struct PingEvent : Event {
        int id = 0;
        explicit PingEvent(int i = 0)
            : Event(EventCategory::Audio), id(i) {}
    };

    struct PongEvent : Event {
        PongEvent() : Event(EventCategory::Input) {}
    };

    struct MultiCategoryEvent : Event {
        MultiCategoryEvent()
            : Event(EventCategory::Audio | EventCategory::Input) {}
    };

    // Collects handler invocation order.
    struct Recorder {
        std::vector<int> ids;
        void add(int v) {
            ids.push_back(v);
        }
    };

}  // namespace

// =========================================================================
// Event base class
// =========================================================================

TEST_CASE("Event: consume/isConsumed", "[events][event]") {
    PingEvent e;
    REQUIRE_FALSE(e.isConsumed());
    e.consume();
    REQUIRE(e.isConsumed());
}

TEST_CASE("Event: category can be a bitmask combination", "[events][event]") {
    MultiCategoryEvent e;
    REQUIRE(anyOf(e.category(), EventCategory::Audio));
    REQUIRE(anyOf(e.category(), EventCategory::Input));
    REQUIRE_FALSE(anyOf(e.category(), EventCategory::Render));
}

TEST_CASE("EventCategory: anyOf semantics", "[events][event]") {
    const auto mask = EventCategory::Audio | EventCategory::Input;
    REQUIRE(anyOf(EventCategory::Audio, mask));
    REQUIRE(anyOf(EventCategory::Input, mask));
    REQUIRE_FALSE(anyOf(EventCategory::Core, mask));
}

// =========================================================================
// Basic subscribe / publish
// =========================================================================

TEST_CASE("EventBus: typed subscriber receives matching event",
    "[events][bus]") {
    EventBus bus;
    int seenId = -1;

    bus.subscribe<PingEvent>([&](PingEvent& e) { seenId = e.id; });
    PingEvent p{ 42 };
    bus.publish(p);

    REQUIRE(seenId == 42);
}

TEST_CASE("EventBus: typed subscriber ignores other types",
    "[events][bus]") {
    EventBus bus;
    bool called = false;

    bus.subscribe<PingEvent>([&](PingEvent&) { called = true; });
    PongEvent other;
    bus.publish(other);

    REQUIRE_FALSE(called);
}

TEST_CASE("EventBus: multiple subscribers all fire", "[events][bus]") {
    EventBus bus;
    int a = 0, b = 0;

    bus.subscribe<PingEvent>([&](PingEvent&) { ++a; });
    bus.subscribe<PingEvent>([&](PingEvent&) { ++b; });

    PingEvent p;
    bus.publish(p);

    REQUIRE(a == 1);
    REQUIRE(b == 1);
}

// =========================================================================
// Priority ordering
// =========================================================================

TEST_CASE("EventBus: typed handlers run highest priority first",
    "[events][bus][priority]") {
    EventBus bus;
    Recorder rec;

    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(1); }, /*prio=*/10);
    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(2); }, /*prio=*/0);
    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(3); }, /*prio=*/5);

    PingEvent p;
    bus.publish(p);

    REQUIRE(rec.ids == std::vector<int>{1, 3, 2});
}

TEST_CASE("EventBus: equal priorities keep subscribe order",
    "[events][bus][priority]") {
    EventBus bus;
    Recorder rec;

    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(1); }, /*prio=*/5);
    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(2); }, /*prio=*/5);
    bus.subscribe<PingEvent>([&](PingEvent&) { rec.add(3); }, /*prio=*/5);

    PingEvent p;
    bus.publish(p);

    REQUIRE(rec.ids == std::vector<int>{1, 2, 3});
}

// =========================================================================
// consume() interaction
// =========================================================================

TEST_CASE("EventBus: receiveIfHandled=false skips after consume",
    "[events][bus][consume]") {
    EventBus bus;
    bool first = false, second = false;

    bus.subscribe<PingEvent>([&](PingEvent& e) {
        first = true;
        e.consume();
        }, /*prio=*/100);

    bus.subscribe<PingEvent>([&](PingEvent&) { second = true; },
        /*prio=*/0, /*receiveIfHandled=*/false);

    PingEvent p;
    bus.publish(p);

    REQUIRE(first);
    REQUIRE_FALSE(second);
}

TEST_CASE("EventBus: receiveIfHandled=true still fires after consume",
    "[events][bus][consume]") {
    EventBus bus;
    bool second = false;

    bus.subscribe<PingEvent>([&](PingEvent& e) { e.consume(); }, /*prio=*/100);
    bus.subscribe<PingEvent>([&](PingEvent&) { second = true; },
        /*prio=*/0, /*receiveIfHandled=*/true);

    PingEvent p;
    bus.publish(p);
    REQUIRE(second);
}

// =========================================================================
// Category subscriptions
// =========================================================================

TEST_CASE("EventBus: category subscriber receives matching category",
    "[events][bus][category]") {
    EventBus bus;
    int audioCount = 0, inputCount = 0;

    bus.subscribeCategories(EventCategory::Audio,
        [&](Event&) { ++audioCount; });
    bus.subscribeCategories(EventCategory::Input,
        [&](Event&) { ++inputCount; });

    PingEvent ping;   // Audio
    PongEvent pong;   // Input
    bus.publish(ping);
    bus.publish(pong);

    REQUIRE(audioCount == 1);
    REQUIRE(inputCount == 1);
}

TEST_CASE("EventBus: category subscriber with combined mask fires for any",
    "[events][bus][category]") {
    EventBus bus;
    int count = 0;

    bus.subscribeCategories(EventCategory::Audio | EventCategory::Input,
        [&](Event&) { ++count; });

    PingEvent ping;
    PongEvent pong;
    bus.publish(ping);
    bus.publish(pong);

    REQUIRE(count == 2);
}

TEST_CASE("EventBus: typed and category subscribers both run",
    "[events][bus][category]") {
    EventBus bus;
    bool typedCalled = false, catCalled = false;

    bus.subscribe<PingEvent>([&](PingEvent&) { typedCalled = true; });
    bus.subscribeCategories(EventCategory::Audio,
        [&](Event&) { catCalled = true; });

    PingEvent p;
    bus.publish(p);

    REQUIRE(typedCalled);
    REQUIRE(catCalled);
}

TEST_CASE("EventBus: category subscribers respect priority",
    "[events][bus][category]") {
    EventBus bus;
    Recorder rec;

    bus.subscribeCategories(EventCategory::Audio,
        [&](Event&) { rec.add(1); }, /*prio=*/0);
    bus.subscribeCategories(EventCategory::Audio,
        [&](Event&) { rec.add(2); }, /*prio=*/10);

    PingEvent p;
    bus.publish(p);

    REQUIRE(rec.ids == std::vector<int>{2, 1});
}

// =========================================================================
// Unsubscribe
// =========================================================================

TEST_CASE("EventBus: unsubscribe stops future delivery",
    "[events][bus][unsubscribe]") {
    EventBus bus;
    int count = 0;

    auto id = bus.subscribe<PingEvent>([&](PingEvent&) { ++count; });
    PingEvent p;
    bus.publish(p);
    REQUIRE(count == 1);

    bus.unsubscribe(id);
    bus.publish(p);
    REQUIRE(count == 1);
}

TEST_CASE("EventBus: unsubscribe twice is a no-op",
    "[events][bus][unsubscribe]") {
    EventBus bus;
    auto id = bus.subscribe<PingEvent>([](PingEvent&) {});
    bus.unsubscribe(id);
    bus.unsubscribe(id);  // no crash
    SUCCEED();
}

TEST_CASE("EventBus: unsubscribe invalid handle is safe",
    "[events][bus][unsubscribe]") {
    EventBus bus;
    bus.unsubscribe(kInvalidSubscriptionId);
    SUCCEED();
}

TEST_CASE("EventBus: unsubscribe within handler skips remaining?",
    "[events][bus][unsubscribe]") {
    // Sanity: unsubscribing during dispatch does not crash and further
    // dispatches no-op.
    EventBus bus;
    int called = 0;
    SubscriptionId victim;
    victim = bus.subscribe<PingEvent>([&](PingEvent&) {
        ++called;
        bus.unsubscribe(victim);
        });

    PingEvent p;
    bus.publish(p);
    bus.publish(p);
    bus.publish(p);

    REQUIRE(called == 1);
}

// =========================================================================
// Exception isolation
// =========================================================================

TEST_CASE("EventBus: throwing subscriber does not stop others",
    "[events][bus][exception]") {
    EventBus bus;
    int after = 0;

    bus.subscribe<PingEvent>(
        [](PingEvent&) { throw std::runtime_error("boom"); }, /*prio=*/10);
    bus.subscribe<PingEvent>([&](PingEvent&) { ++after; }, /*prio=*/0);

    PingEvent p;
    REQUIRE_NOTHROW(bus.publish(p));
    REQUIRE(after == 1);
}

// =========================================================================
// Deferred queue
// =========================================================================

TEST_CASE("EventBus: publishDeferred does not fire immediately",
    "[events][bus][deferred]") {
    EventBus bus;
    bool called = false;

    bus.subscribe<PingEvent>([&](PingEvent&) { called = true; });
    bus.publishDeferred(std::make_unique<PingEvent>(7));

    REQUIRE_FALSE(called);
    REQUIRE(bus.pendingDeferred() == 1u);
}

TEST_CASE("EventBus: pumpDeferred dispatches pending events",
    "[events][bus][deferred]") {
    EventBus bus;
    Recorder rec;
    bus.subscribe<PingEvent>([&](PingEvent& e) { rec.add(e.id); });

    bus.publishDeferred(std::make_unique<PingEvent>(1));
    bus.publishDeferred(std::make_unique<PingEvent>(2));
    REQUIRE(bus.pendingDeferred() == 2u);

    bus.pumpDeferred();
    REQUIRE(rec.ids == std::vector<int>{1, 2});
    REQUIRE(bus.pendingDeferred() == 0u);
}

TEST_CASE("EventBus: pumpDeferred honours EventPriority ordering",
    "[events][bus][deferred]") {
    EventBus bus;
    Recorder rec;
    bus.subscribe<PingEvent>([&](PingEvent& e) { rec.add(e.id); });

    bus.publishDeferred(std::make_unique<PingEvent>(1), EventPriority::Low);
    bus.publishDeferred(std::make_unique<PingEvent>(2), EventPriority::High);
    bus.publishDeferred(std::make_unique<PingEvent>(3), EventPriority::Normal);
    bus.publishDeferred(std::make_unique<PingEvent>(4), EventPriority::Immediate);

    bus.pumpDeferred();
    // Immediate(0), High(1), Normal(2), Low(3)
    REQUIRE(rec.ids == std::vector<int>{4, 2, 3, 1});
}

TEST_CASE("EventBus: pumpDeferred(maxEvents) limits dispatch",
    "[events][bus][deferred]") {
    EventBus bus;
    int count = 0;
    bus.subscribe<PingEvent>([&](PingEvent&) { ++count; });

    for (int i = 0; i < 5; ++i) {
        bus.publishDeferred(std::make_unique<PingEvent>(i));
    }
    REQUIRE(bus.pendingDeferred() == 5u);

    bus.pumpDeferred(2);
    REQUIRE(count == 2);
    REQUIRE(bus.pendingDeferred() == 3u);

    bus.pumpDeferred();
    REQUIRE(count == 5);
    REQUIRE(bus.pendingDeferred() == 0u);
}

TEST_CASE("EventBus: emplaceDeferred defaults to Normal priority",
    "[events][bus][deferred]") {
    EventBus bus;
    Recorder rec;
    bus.subscribe<PingEvent>([&](PingEvent& e) { rec.add(e.id); });

    bus.emplaceDeferred<PingEvent>(1);  // implicit Normal
    bus.publishDeferred(std::make_unique<PingEvent>(2), EventPriority::High);

    bus.pumpDeferred();
    // High before Normal
    REQUIRE(rec.ids == std::vector<int>{2, 1});
}

TEST_CASE("EventBus: clear drops queued deferred events",
    "[events][bus][deferred]") {
    EventBus bus;
    int count = 0;
    bus.subscribe<PingEvent>([&](PingEvent&) { ++count; });

    bus.publishDeferred(std::make_unique<PingEvent>(1));
    REQUIRE(bus.pendingDeferred() == 1u);

    bus.clear();
    REQUIRE(bus.pendingDeferred() == 0u);

    bus.pumpDeferred();
    REQUIRE(count == 0);
}

TEST_CASE("EventBus: clear also drops subscriptions",
    "[events][bus]") {
    EventBus bus;
    int count = 0;
    bus.subscribe<PingEvent>([&](PingEvent&) { ++count; });

    bus.clear();
    PingEvent p;
    bus.publish(p);
    REQUIRE(count == 0);
}

// =========================================================================
// ScopedEventBus / defaultEventBus
// =========================================================================

TEST_CASE("ScopedEventBus: installs and restores default bus",
    "[events][bus][default]") {
    EventBus* prev = defaultEventBus();

    EventBus bus;
    {
        ScopedEventBus scoped(&bus);
        REQUIRE(defaultEventBus() == &bus);
    }
    REQUIRE(defaultEventBus() == prev);
}

TEST_CASE("ScopedEventBus: nested scopes restore in LIFO order",
    "[events][bus][default]") {
    EventBus a, b;
    EventBus* prev = defaultEventBus();

    {
        ScopedEventBus outer(&a);
        REQUIRE(defaultEventBus() == &a);
        {
            ScopedEventBus inner(&b);
            REQUIRE(defaultEventBus() == &b);
        }
        REQUIRE(defaultEventBus() == &a);
    }
    REQUIRE(defaultEventBus() == prev);
}