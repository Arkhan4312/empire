#pragma once

#include <cassert>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace empire::util {

namespace detail {
struct OkTag {};
struct ErrTag {};
}  // namespace detail

// Result<T, E>
template <typename T, typename E>
class [[nodiscard]] Result {
    static_assert(!std::is_void_v<T>, "Use Result<void, E> for void");
    static_assert(!std::is_reference_v<T>);
    static_assert(!std::is_reference_v<E>);

public:
    using value_type = T;
    using error_type = E;

    static Result ok(T value) {
        return Result(detail::OkTag{}, std::move(value));
    }
    static Result err(E error) {
        return Result(detail::ErrTag{}, std::move(error));
    }

    bool isOk() const noexcept {
        return m_storage.index() == 0;
    }
    bool isErr() const noexcept {
        return m_storage.index() == 1;
    }
    explicit operator bool() const noexcept {
        return isOk();
    }

    // Precondition: isOk(). In debug - assert; in release - std::get throws
    T& value() & {
        assert(isOk());
        return std::get<0>(m_storage);
    }
    const T& value() const& {
        assert(isOk());
        return std::get<0>(m_storage);
    }
    T&& value() && {
        assert(isOk());
        return std::get<0>(std::move(m_storage));
    }

    // Precondion: isErr().
    E& error() & {
        assert(isErr());
        return std::get<1>(m_storage);
    }
    const E& error() const& {
        assert(isErr());
        return std::get<1>(m_storage);
    }
    E&& error() && {
        assert(isErr());
        return std::get<1>(std::move(m_storage));
    }

    T* operator->() {
        return &value();
    }

    const T* operator->() const {
        return &value();
    }
    T& operator*() & {
        return value();
    }

    const T& operator*() const& {
        return value();
    }
    T&& operator*() && {
        return std::move(*this).value();
    }

    template <typename U>
    T valueOr(U&& fallback) const& {
        return isOk() ? value() : static_cast<T>(std::forward<U>(fallback));
    }
    template <typename U>
    T valueOr(U&& fallback) && {
        return isOk() ? std::move(*this).value()
                      : static_cast<T>(std::forward<U>(fallback));
    }

private:
    Result(detail::OkTag, T v)
        : m_storage(std::in_place_index<0>, std::move(v)) {
    }
    Result(detail::ErrTag, E e)
        : m_storage(std::in_place_index<1>, std::move(e)) {
    }

    std::variant<T, E> m_storage;
};

// Result<void, E>
template <typename E>
class [[nodiscard]] Result<void, E> {
    static_assert(!std::is_reference_v<E>);

public:
    using value_type = void;
    using error_type = E;

    static Result ok() {
        return Result(true);
    }
    static Result err(E error) {
        return Result(std::move(error));
    }

    bool isOk() const noexcept {
        return !m_error.has_value();
    }
    bool isErr() const noexcept {
        return m_error.has_value();
    }
    explicit operator bool() const noexcept {
        return isOk();
    }

    E& error() & {
        assert(isErr());
        return *m_error;
    }
    const E& error() const& {
        assert(isErr());
        return *m_error;
    }
    E&& error() && {
        assert(isErr());
        return std::move(*m_error);
    }

private:
    explicit Result(bool /*ok*/) : m_error(std::nullopt) {
    }
    explicit Result(E error) : m_error(std::move(error)) {
    }
    std::optional<E> m_error;
};

}  // namespace empire::util