#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

#include <empire/engine/util/Handle.h>

namespace {
    struct PlayerTag;
    using PlayerHandle = empire::util::Handle<PlayerTag>;
    using Players = empire::util::HandleTable<PlayerTag, std::string>;
} // namespace

TEST_CASE("Handle: default is invalid", "[handle]") {
    PlayerHandle h{};
    REQUIRE_FALSE(h.isValid());
    REQUIRE_FALSE(static_cast<bool>(h));
    h.invalidate();
    REQUIRE_FALSE(h.isValid());
}

TEST_CASE("Handle: insert/get", "[handle]") {
    Players t;
    auto h = t.insert("alice");
    REQUIRE(h.isValid());
    REQUIRE(t.size() == 1);

    auto* p = t.get(h);
    REQUIRE(p != nullptr);
    REQUIRE(*p == "alice");
}

TEST_CASE("Handle: erase invalidates and bumps generation", "[handle]") {
    Players t;
    auto h = t.insert("alice");
    const auto oldGen = h.generation;

    REQUIRE(t.erase(h));
    REQUIRE(t.get(h) == nullptr);
    REQUIRE(t.size() == 0);

    // Insert reuses slot — handle must have a different generation.
    auto h2 = t.insert("bob");
    REQUIRE(h2.index == h.index);
    REQUIRE(h2.generation != oldGen);
    REQUIRE(t.get(h) == nullptr);   // old handle still invalid
    REQUIRE(t.get(h2) != nullptr);
    REQUIRE(*t.get(h2) == "bob");
}

TEST_CASE("Handle: erase twice returns false", "[handle]") {
    Players t;
    auto h = t.insert("alice");
    REQUIRE(t.erase(h));
    REQUIRE_FALSE(t.erase(h));
}

TEST_CASE("Handle: isValid on foreign table", "[handle]") {
    Players t1, t2;
    auto h = t1.insert("alice");
    REQUIRE_FALSE(t2.isValid(h));
    REQUIRE(t2.get(h) == nullptr);
}

TEST_CASE("Handle: forEach visits live slots", "[handle]") {
    Players t;
    auto a = t.insert("alice");
    auto b = t.insert("bob");
    auto c = t.insert("carol");
    t.erase(b);

    std::vector<std::string> seen;
    t.forEach([&](PlayerHandle, std::string& name) { seen.push_back(name); });

    REQUIRE(seen.size() == 2);
    REQUIRE(std::find(seen.begin(), seen.end(), "alice") != seen.end());
    REQUIRE(std::find(seen.begin(), seen.end(), "carol") != seen.end());
    REQUIRE(std::find(seen.begin(), seen.end(), "bob") == seen.end());
}

TEST_CASE("Handle: clear wipes everything", "[handle]") {
    Players t;
    auto a = t.insert("alice");
    t.clear();
    REQUIRE(t.empty());
    REQUIRE(t.get(a) == nullptr);
}