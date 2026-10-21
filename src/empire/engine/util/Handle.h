#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace empire::util {

// Handle<Tag>
// POD. Fast, trivial serialize, defoult operator==.
// Index == 0 - invalid, Index == 1 - first valid handle.
template <typename Tag>
struct Handle {
    using tag_type = Tag;

    static constexpr std::uint32_t kInvalidIndex = 0;

    std::uint32_t index = kInvalidIndex;
    std::uint32_t generation = 0;

    constexpr bool isValid() const noexcept {
        return index != kInvalidIndex;
    }
    constexpr explicit operator bool() const noexcept {
        return isValid();
    }

    constexpr void invalidate() noexcept {
        index = kInvalidIndex;
        generation = 0;
    }

    friend constexpr bool operator==(Handle, Handle) noexcept = default;
};

// HadleTable<Tag,Value>
// Slot-based storage with generational validation.
template <typename Tag, typename Value>
class HandleTable {
public:
    using HandleType = Handle<Tag>;
    HandleTable() {
        m_slots.emplace_back();
    }
    HandleType insert(Value value) {
        if (!m_free.empty()) {
            const std::uint32_t idx = m_free.back();
            m_free.pop_back();
            Slot& s = m_slots[idx];
            s.value.emplace(std::move(value));
            s.alive = true;
            ++m_live;
            return HandleType{idx, s.generation};
        }
        const std::uint32_t idx = static_cast<std::uint32_t>(m_slots.size());
        Slot s;
        s.generation = 1;
        s.value.emplace(std::move(value));
        s.alive = true;
        m_slots.push_back(std::move(s));
        ++m_live;
        return HandleType{idx, 1};
    }

    Value* get(HandleType h) noexcept {
        if (!isValid(h)) {
            return nullptr;
        }
        return &(*m_slots[h.index].value);
    }
    const Value* get(HandleType h) const noexcept {
        if (!isValid(h)) {
            return nullptr;
        }
        return &(*m_slots[h.index].value);
    }

    bool erase(HandleType h) noexcept {
        if (!isValid(h)) {
            return false;
        }
        Slot& s = m_slots[h.index];
        s.value.reset();
        s.alive = false;
        ++s.generation;
        if (s.generation == 0) {
            s.generation = 1;
        }
        m_free.push_back(h.index);
        --m_live;
        return true;
    }

    bool isValid(HandleType h) const noexcept {
        if (!h.isValid() || h.index >= m_slots.size()) {
            return false;
        }
        const Slot& s = m_slots[h.index];
        return s.alive && s.generation == h.generation;
    }

    void clear() noexcept {
        m_slots.clear();
        m_slots.emplace_back();
        m_free.clear();
        m_live = 0;
    }

    std::size_t size() const noexcept {
        return m_live;
    }
    bool empty() const noexcept {
        return m_live == 0;
    }

    template <typename F>
    void forEach(F&& f) {
        for (std::size_t i = 0; i < m_slots.size(); ++i) {
            Slot& s = m_slots[i];
            if (!s.alive) {
                continue;
            }
            f(HandleType{static_cast<std::uint32_t>(i), s.generation},
              *s.value);
        }
    }
    template <typename F>
    void forEach(F&& f) const {
        for (std::size_t i = 0; i < m_slots.size(); ++i) {
            const Slot& s = m_slots[i];
            if (!s.alive) continue;
            f(HandleType{static_cast<std::uint32_t>(i), s.generation},
              *s.value);  // *s.value → const Value& (optional<Value> is const)
        }
    }

    void reserve(std::size_t n) {
        m_slots.reserve(n);
        m_free.reserve(n);
    }

private:
    struct Slot {
        std::optional<Value> value;
        std::uint32_t generation = 1;
        bool alive = false;
    };
    std::vector<Slot> m_slots;
    std::vector<std::uint32_t> m_free;
    std::size_t m_live = 0;
};
}  // namespace empire::util