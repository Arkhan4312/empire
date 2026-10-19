#include <empire/engine/audio/AudioBusConfig.h>
#include <empire/engine/audio/AudioBusRegistry.h>
#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_set>
#include <utility>

namespace empire::audio {
namespace {
constexpr float kFadeEpsilon = 1.0e-4f;
float clamp01(float v) noexcept {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}
}  // namespace

AudioBusRegistry::AudioBusRegistry() = default;
AudioBusRegistry::~AudioBusRegistry() {
    uninit();
}
// Binding
AudioResult<void> AudioBusRegistry::init(ma_engine* engine) {
    if (engine == nullptr) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }
    uninit();

    std::vector<BusHandle> ordered;
    ordered.reserve(m_handles.size());
    std::unordered_set<std::uint32_t> visited;

    std::function<void(BusHandle)> visit = [&](BusHandle h) {
        if (!h.isValid()) {
            return;
        }
        if (visited.count(h.index)) {
            return;
        }
        AudioBus* b = m_handles.get(h);
        if (b == nullptr) {
            return;
        }
        visit(b->parent);
        visited.insert(h.index);
        ordered.push_back(h);
    };
    m_handles.forEach([&](BusHandle h, AudioBus&) { visit(h); });
    for (BusHandle h : ordered) {
        AudioBus* b = m_handles.get(h);
        if (b == nullptr) {
            continue;
        }

        ma_sound_group* parentGroup = nullptr;
        if (b->parent.isValid()) {
            if (AudioBus* p = m_handles.get(b->parent)) {
                parentGroup = p->group.get();
            }
        }

        auto r = b->group.init(engine, 0, parentGroup);
        if (!r.isOk()) {
            GAME_LOG_CH_ERROR(channels::kBus,
                              "group init failed for bus '{}': {}", b->name,
                              describe(r.error()));
            uninit();
            return r;
        }
    }
    applyAll();
    GAME_LOG_CH_DEBUG(channels::kBus, "initialized {} bus(es)", ordered.size());
    return AudioResult<void>::ok();
}

void AudioBusRegistry::uninit() noexcept {
    m_handles.forEach([](BusHandle, AudioBus& b) { b.group.uninit(); });
}

// Registry

AudioResult<void> AudioBusRegistry::initDefaults() {
    for (const auto& d : kDefaultBuses) {
        auto r = ensure(d.name, d.parent);
        if (!r.isOk()) {
            GAME_LOG_CH_ERROR(channels::kBus,
                              "InitDefaults: ensure('{}') failed: {}", d.name,
                              describe(r.error()));
            return AudioResult<void>::err(r.error());
        }
        if (AudioBus* b = m_handles.get(r.value())) {
            if (b->volume == 1.0f && b->currentVolume == 1.0f &&
                !b->isUserSet) {
                b->volume = d.defaultVolume;
                b->currentVolume = d.defaultVolume;
            }
        }
    }
    return AudioResult<void>::ok();
}

AudioResult<BusHandle> AudioBusRegistry::ensure(std::string_view name,
                                                std::string_view parent) {
    using Res = AudioResult<BusHandle>;
    if (name.empty()) {
        return Res::err(AudioError::InvalidArgument);
    }
    // The by-name map uses std::string keys; the extra temporary is
    // negligible and keep the code simple. Not a hot path.
    const std::string key(name);

    if (auto it = m_byName.find(key); it != m_byName.end()) {
        AudioBus* b = m_handles.get(it->second);
        if (b == nullptr) {
            return Res::err(AudioError::Internal);
        }
        BusHandle reqParent{};
        if (!parent.empty()) {
            auto pit = m_byName.find(std::string(parent));
            if (pit == m_byName.end()) {
                return Res::err(AudioError::BusNotFound);
            }
            reqParent = pit->second;
        }
        if (b->parent != reqParent) {
            return Res::err(AudioError::BusAlreadyExists);
        }
        return Res::ok(it->second);
    }

    BusHandle par{};
    if (!parent.empty()) {
        auto pit = m_byName.find(std::string(parent));
        if (pit == m_byName.end()) {
            GAME_LOG_CH_ERROR(channels::kBus,
                              "ensure('{}'): parent '{}' not found", key,
                              parent);
            return Res::err(AudioError::BusNotFound);
        }
        par = pit->second;
    }

    AudioBus b;
    b.name = key;
    b.parent = par;
    const BusHandle h = m_handles.insert(std::move(b));
    m_byName[key] = h;

    if (par.isValid()) {
        if (AudioBus* p = m_handles.get(par)) {
            p->children.push_back(h);
        }
    }
    return Res::ok(h);
}

bool AudioBusRegistry::contains(std::string_view name) const noexcept {
    return m_byName.find(std::string(name)) != m_byName.end();
}

BusHandle AudioBusRegistry::find(std::string_view name) const noexcept {
    auto it = m_byName.find(std::string(name));
    return it == m_byName.end() ? kInvalidBus : it->second;
}

// volume, mute, solo

void AudioBusRegistry::setVolume(BusHandle h, float v) {
    AudioBus* b = m_handles.get(h);
    if (b == nullptr) {
        return;
    }
    v = clamp01(v);
    b->volume = v;
    b->currentVolume = v;
    b->isUserSet = true;
    b->fadeSeconds = 0.0f;
    apply(h);
}

float AudioBusRegistry::volume(BusHandle h) const {
    auto* self = const_cast<AudioBusRegistry*>(this);
    const AudioBus* b = self->m_handles.get(h);
    return b ? b->volume : 0.0f;
}

void AudioBusRegistry::setMuted(BusHandle h, bool m) {
    AudioBus* b = m_handles.get(h);
    if (b == nullptr) {
        return;
    }
    b->muted = m;
    apply(h);
}

bool AudioBusRegistry::muted(BusHandle h) const {
    auto* self = const_cast<AudioBusRegistry*>(this);
    const AudioBus* b = self->m_handles.get(h);
    return b && b->muted;
}

void AudioBusRegistry::setSolo(BusHandle h, bool s) {
    AudioBus* b = m_handles.get(h);
    if (b == nullptr) {
        return;
    }
    if (b->solo == s) {
        return;
    }
    b->solo = s;
    recomputeSoloState();
    applyAll();
}

bool AudioBusRegistry::solo(BusHandle h) const {
    auto* self = const_cast<AudioBusRegistry*>(this);
    const AudioBus* b = self->m_handles.get(h);
    return b && b->solo;
}

void AudioBusRegistry::clearAllSolo() {
    bool changed = false;
    m_handles.forEach([&](BusHandle, AudioBus& b) {
        if (b.solo) {
            b.solo = false;
            changed = true;
        }
    });
    if (changed) {
        recomputeSoloState();
        applyAll();
    }
}

// Applying

void AudioBusRegistry::applySnapshot(const AudioBusSnapshot& snap,
                                     float fadeSec) {
    if (fadeSec < 0.0f) {
        fadeSec = 0.0f;
    }
    for (const auto& [name, target] : snap.volumes) {
        auto it = m_byName.find(name);
        if (it == m_byName.end()) {
            GAME_LOG_CH_WARN(channels::kBus, "snapshot: unknown bus '{}'",
                             name);
            continue;
        }
        AudioBus* b = m_handles.get(it->second);
        if (b == nullptr) {
            continue;
        }

        const float v = clamp01(target);
        b->volume = v;
        b->isUserSet = true;
        if (fadeSec <= 0.0f) {
            b->currentVolume = v;
            b->fadeSeconds = 0.0f;
            apply(it->second);
        } else {
            b->fadeStartVolume = b->currentVolume;
            b->fadeSeconds = fadeSec;
            b->fadeElapsed = 0.0f;
            // Not applied yet
        }
    }
}

void AudioBusRegistry::apply(BusHandle h) {
    AudioBus* b = m_handles.get(h);
    if (b == nullptr || !b->group.isValid()) {
        return;
    }
    const float eff = (b->muted || b->soloKill) ? 0.0f : b->currentVolume;
    ma_sound_group_set_volume(b->group.get(), eff);
}

void AudioBusRegistry::applyAll() {
    m_handles.forEach([this](BusHandle h, AudioBus&) { apply(h); });
}

void AudioBusRegistry::update(float dt) {
    if (dt <= 0.0f) {
        return;
    }
    m_handles.forEach([&](BusHandle h, AudioBus& b) {
        if (b.fadeSeconds <= 0.0f) {
            return;
        }
        b.fadeElapsed += dt;
        float t = b.fadeElapsed / b.fadeSeconds;
        if (t >= 1.0f) {
            t = 1.0f;
            b.fadeSeconds = 0.0f;
            b.currentVolume = b.volume;
        } else {
            b.currentVolume =
                b.fadeStartVolume + (b.volume - b.fadeStartVolume) * t;
        }
        apply(h);
        if (std::fabs(b.currentVolume - b.volume) <= kFadeEpsilon) {
            b.currentVolume = b.volume;
            b.fadeSeconds = 0.0f;
        }
    });
}

AudioResult<void> AudioBusRegistry::remove(BusHandle h) {
    AudioBus* b = m_handles.get(h);
    if (b == nullptr) {
        return AudioResult<void>::err(AudioError::BusNotFound);
    }
    if (!b->children.empty()) {
        return AudioResult<void>::err(AudioError::InvalidArgument);
    }

    b->group.uninit();

    if (b->parent.isValid()) {
        if (AudioBus* p = m_handles.get(b->parent)) {
            auto& ch = p->children;
            ch.erase(std::remove(ch.begin(), ch.end(), h), ch.end());
        }
    }

    m_byName.erase(b->name);
    m_handles.erase(h);

    recomputeSoloState();
    applyAll();
    return AudioResult<void>::ok();
}

detail::MaSoundGroup* AudioBusRegistry::group(BusHandle h) noexcept {
    AudioBus* b = m_handles.get(h);
    return (b && b->group.isValid()) ? &b->group : nullptr;
}

void AudioBusRegistry::recomputeSoloState() {
    bool anySolo = false;
    m_handles.forEach([&](BusHandle, AudioBus& b) {
        if (b.solo) {
            anySolo = true;
        }
    });
    m_anySolo = anySolo;

    std::function<bool(BusHandle)> visit = [&](BusHandle h) -> bool {
        AudioBus* b = m_handles.get(h);
        if (b == nullptr) {
            return false;
        }
        bool hasSoloInSubtree = false;
        for (BusHandle c : b->children) {
            if (visit(c)) {
                hasSoloInSubtree = true;
            }
        }

        b->soloKill = anySolo && !b->solo && !hasSoloInSubtree;
        return b->solo || hasSoloInSubtree;
    };

    m_handles.forEach([&](BusHandle h, AudioBus& b) {
        if (!b.parent.isValid()) {
            visit(h);
        }
    });
}

}  // namespace empire::audio