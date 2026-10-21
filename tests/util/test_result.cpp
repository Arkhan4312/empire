#include <empire/engine/util/Result.h>

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <string>
#include <utility>

using namespace empire::util;

namespace {
enum class ErrCode { A, B };

struct MoveOnly {
    int value = 0;
    explicit MoveOnly(int v) : value(v) {
    }
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
};
}  // namespace

// -------------------------------------------------------------------------
// Result<T, E>
// -------------------------------------------------------------------------

TEST_CASE("Result<T,E>: ok construction", "[util][result]") {
    auto r = Result<int, ErrCode>::ok(42);
    REQUIRE(r.isOk());
    REQUIRE_FALSE(r.isErr());
    REQUIRE(static_cast<bool>(r));
    REQUIRE(r.value() == 42);
    REQUIRE(*r == 42);
}

TEST_CASE("Result<T,E>: err construction", "[util][result]") {
    auto r = Result<int, ErrCode>::err(ErrCode::A);
    REQUIRE_FALSE(r.isOk());
    REQUIRE(r.isErr());
    REQUIRE_FALSE(static_cast<bool>(r));
    REQUIRE(r.error() == ErrCode::A);
}

TEST_CASE("Result<T,E>: operator-> forwards to value", "[util][result]") {
    struct Foo {
        int x;
    };
    auto r = Result<Foo, ErrCode>::ok(Foo{42});
    REQUIRE(r->x == 42);

    const auto cr = Result<Foo, ErrCode>::ok(Foo{7});
    REQUIRE(cr->x == 7);
}

TEST_CASE("Result<T,E>: value() rvalue moves out", "[util][result]") {
    auto r = Result<std::string, ErrCode>::ok(std::string{"hello"});
    std::string s = std::move(r).value();
    REQUIRE(s == "hello");
}

TEST_CASE("Result<T,E>: supports move-only T", "[util][result]") {
    auto r = Result<MoveOnly, ErrCode>::ok(MoveOnly{5});
    REQUIRE(r.isOk());
    MoveOnly m = std::move(r).value();
    REQUIRE(m.value == 5);
}

TEST_CASE("Result<T,E>: valueOr returns value when ok", "[util][result]") {
    auto ok = Result<int, ErrCode>::ok(5);
    REQUIRE(ok.valueOr(99) == 5);
}

TEST_CASE("Result<T,E>: valueOr returns fallback when err", "[util][result]") {
    auto err = Result<int, ErrCode>::err(ErrCode::B);
    REQUIRE(err.valueOr(99) == 99);
}

TEST_CASE("Result<T,E>: valueOr rvalue overload moves", "[util][result]") {
    auto ok = Result<std::string, ErrCode>::ok(std::string{"ok"});
    REQUIRE(std::move(ok).valueOr(std::string{"fallback"}) == "ok");

    auto err = Result<std::string, ErrCode>::err(ErrCode::A);
    REQUIRE(std::move(err).valueOr(std::string{"fallback"}) == "fallback");
}

TEST_CASE("Result<T,E>: error() returns stored error", "[util][result]") {
    auto r = Result<int, ErrCode>::err(ErrCode::B);
    REQUIRE(r.error() == ErrCode::B);

    const auto& cr = r;
    REQUIRE(cr.error() == ErrCode::B);
}

// -------------------------------------------------------------------------
// Result<void, E>
// -------------------------------------------------------------------------

TEST_CASE("Result<void,E>: ok", "[util][result]") {
    auto r = Result<void, ErrCode>::ok();
    REQUIRE(r.isOk());
    REQUIRE_FALSE(r.isErr());
    REQUIRE(static_cast<bool>(r));
}

TEST_CASE("Result<void,E>: err", "[util][result]") {
    auto r = Result<void, ErrCode>::err(ErrCode::B);
    REQUIRE_FALSE(r.isOk());
    REQUIRE(r.isErr());
    REQUIRE(r.error() == ErrCode::B);
}

TEST_CASE("Result<void,E>: error() rvalue", "[util][result]") {
    auto r = Result<void, std::string>::err(std::string{"boom"});
    std::string e = std::move(r).error();
    REQUIRE(e == "boom");
}