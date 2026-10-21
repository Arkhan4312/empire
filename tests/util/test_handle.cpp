#include <empire/engine/util/Handle.h>

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <utility>
#include <vector>

using namespace empire::util;

namespace {
struct TestTag {};
using TestHandle = Handle<TestTag>;
}  // namespace

TEST_CASE("Handle: default-constructed is invalid", "[util][handle]") {
    TestHandle h{};
    REQUIRE_FALSE(h.isValid());
    REQUIRE_FALSE(static_cast<bool>(h));
    REQUIRE(h.index == TestHandle::kInvalidIndex);
    REQUIRE(h.generation == 0u);
}

TEST_CASE("Handle: invalidate clears both fields", "[util][handle]") {
    TestHandle h{5u, 3u};
    REQUIRE(h.isValid());
    h.invalidate();
    REQUIRE_FALSE(h.isValid());
    REQUIRE(h.index == TestHandle::kInvalidIndex);
    REQUIRE(h.generation == 0u);
}

TEST_CASE("Handle: equality compares both fields", "[util][handle]") {
    const TestHandle a{1u, 1u};
    const TestHandle b{1u, 1u};
    const TestHandle c{1u, 2u};
    const TestHandle d{2u, 1u};

    REQUIRE(a == b);
    REQUIRE(a != c);
    REQUIRE(a != d);
}

// -------------------------------------------------------------------------
// HandleTable
// -------------------------------------------------------------------------

TEST_CASE("HandleTable: insert returns valid handle", "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto h = table.insert(42);

    REQUIRE(h.isValid());
    REQUIRE(table.size() == 1u);
    REQUIRE_FALSE(table.empty());
    REQUIRE(table.isValid(h));
    REQUIRE(table.get(h) != nullptr);
    REQUIRE(*table.get(h) == 42);
}

TEST_CASE("HandleTable: multiple inserts produce distinct handles",
          "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto a = table.insert(1);
    const auto b = table.insert(2);
    const auto c = table.insert(3);

    REQUIRE(a.index != b.index);
    REQUIRE(b.index != c.index);
    REQUIRE(table.size() == 3u);
    REQUIRE(*table.get(a) == 1);
    REQUIRE(*table.get(b) == 2);
    REQUIRE(*table.get(c) == 3);
}

TEST_CASE("HandleTable: erase invalidates handle", "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto h = table.insert(7);

    REQUIRE(table.erase(h));
    REQUIRE(table.size() == 0u);
    REQUIRE(table.empty());
    REQUIRE_FALSE(table.isValid(h));
    REQUIRE(table.get(h) == nullptr);

    // Second erase is a no-op
    REQUIRE_FALSE(table.erase(h));
    REQUIRE(table.size() == 0u);
}

TEST_CASE("HandleTable: generation bumps on slot reuse", "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto old = table.insert(1);
    const std::uint32_t oldGen = old.generation;

    table.erase(old);
    const auto fresh = table.insert(2);

    // Same slot index, different generation
    REQUIRE(fresh.index == old.index);
    REQUIRE(fresh.generation != oldGen);

    // Stale handle must never resolve
    REQUIRE(table.get(old) == nullptr);
    REQUIRE_FALSE(table.isValid(old));
    REQUIRE(table.isValid(fresh));
    REQUIRE(*table.get(fresh) == 2);
}

TEST_CASE("HandleTable: get/isValid reject out-of-range and stale handles",
          "[util][handle]") {
    HandleTable<TestTag, int> table;
    table.insert(1);

    REQUIRE(table.get(TestHandle{}) == nullptr);
    REQUIRE(table.get(TestHandle{999u, 1u}) == nullptr);
    REQUIRE_FALSE(table.isValid(TestHandle{}));
    REQUIRE_FALSE(table.isValid(TestHandle{999u, 1u}));

    // Wrong generation for an existing slot
    const auto h = table.insert(2);
    REQUIRE(table.get(TestHandle{h.index, h.generation + 1u}) == nullptr);
    REQUIRE_FALSE(table.isValid(TestHandle{h.index, h.generation + 1u}));
}

TEST_CASE("HandleTable: forEach visits only alive slots in index order",
          "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto a = table.insert(10);
    const auto b = table.insert(20);
    const auto c = table.insert(30);
    REQUIRE(table.erase(b));

    std::vector<std::pair<std::uint32_t, int>> seen;
    table.forEach([&](TestHandle h, int& v) { seen.emplace_back(h.index, v); });

    REQUIRE(seen.size() == 2u);
    REQUIRE(seen[0].first == a.index);
    REQUIRE(seen[0].second == 10);
    REQUIRE(seen[1].first == c.index);
    REQUIRE(seen[1].second == 30);
}

TEST_CASE("HandleTable: const forEach exposes const refs", "[util][handle]") {
    HandleTable<TestTag, int> table;
    table.insert(7);
    table.insert(3);

    const auto& ctable = table;
    int sum = 0;
    ctable.forEach([&](TestHandle, const int& v) { sum += v; });

    REQUIRE(sum == 10);
}

TEST_CASE("HandleTable: mutation via get()", "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto h = table.insert(1);

    *table.get(h) = 99;
    REQUIRE(*table.get(h) == 99);

    // const get must also see the update
    const auto& ctable = table;
    REQUIRE(*ctable.get(h) == 99);
}

TEST_CASE("HandleTable: clear resets everything", "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto h = table.insert(1);
    table.insert(2);

    table.clear();

    REQUIRE(table.size() == 0u);
    REQUIRE(table.empty());
    REQUIRE(table.get(h) == nullptr);
    REQUIRE_FALSE(table.isValid(h));
}

TEST_CASE("HandleTable: reuse after erase keeps size accounting correct",
          "[util][handle]") {
    HandleTable<TestTag, int> table;
    const auto a = table.insert(1);
    table.insert(2);
    REQUIRE(table.erase(a));
    REQUIRE(table.size() == 1u);

    const auto c = table.insert(3);
    REQUIRE(table.size() == 2u);
    REQUIRE(c.index == a.index);  // slot reused
}

TEST_CASE("HandleTable: reserve does not change behaviour", "[util][handle]") {
    HandleTable<TestTag, int> table;
    table.reserve(128);

    const auto a = table.insert(1);
    const auto b = table.insert(2);

    REQUIRE(table.size() == 2u);
    REQUIRE(table.get(a) != nullptr);
    REQUIRE(table.get(b) != nullptr);
}