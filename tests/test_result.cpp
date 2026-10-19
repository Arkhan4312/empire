#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>

#include <empire/engine/util/Result.h>

using empire::util::Result;

TEST_CASE("Result<int,string>: ok path", "[result]") {
    auto r = Result<int, std::string>::ok(42);
    REQUIRE(r.isOk());
    REQUIRE_FALSE(r.isErr());
    REQUIRE(static_cast<bool>(r));
    REQUIRE(r.value() == 42);
    REQUIRE(*r == 42);
    REQUIRE(r.valueOr(0) == 42);
}

TEST_CASE("Result<int,string>: err path", "[result]") {
    auto r = Result<int, std::string>::err("boom");
    REQUIRE(r.isErr());
    REQUIRE_FALSE(r.isOk());
    REQUIRE_FALSE(static_cast<bool>(r));
    REQUIRE(r.error() == "boom");
    REQUIRE(r.valueOr(7) == 7);
}

TEST_CASE("Result: move-only value type", "[result]") {
    auto r = Result<std::unique_ptr<int>, std::string>::ok(
        std::make_unique<int>(5));
    REQUIRE(r.isOk());
    auto v = std::move(r).value();
    REQUIRE(*v == 5);
}

TEST_CASE("Result: move-only error type", "[result]") {
    auto r = Result<int, std::unique_ptr<std::string>>::err(
        std::make_unique<std::string>("err"));
    REQUIRE(r.isErr());
    auto e = std::move(r).error();
    REQUIRE(*e == "err");
}

TEST_CASE("Result: error() on ok is UB — do not test", "[result][.]") {
    // Intentionally empty: contract requires isOk() before error().
}

TEST_CASE("Result<void,string>: ok", "[result]") {
    auto r = Result<void, std::string>::ok();
    REQUIRE(r.isOk());
    REQUIRE_FALSE(r.isErr());
    REQUIRE(static_cast<bool>(r));
}

TEST_CASE("Result<void,string>: err", "[result]") {
    auto r = Result<void, std::string>::err("bad");
    REQUIRE(r.isErr());
    REQUIRE(r.error() == "bad");
}

TEST_CASE("Result: overwrite by reassignment", "[result]") {
    Result<int, std::string> r = Result<int, std::string>::ok(1);
    r = Result<int, std::string>::err("oops");
    REQUIRE(r.isErr());
    r = Result<int, std::string>::ok(2);
    REQUIRE(r.isOk());
    REQUIRE(r.value() == 2);
}