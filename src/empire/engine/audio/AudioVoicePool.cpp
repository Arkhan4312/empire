#include <empire/engine/audio/AudioChannels.h>
#include <empire/engine/audio/AudioVoicePool.h>
#include <empire/engine/audio/detail/MaResult.h>
#include <empire/engine/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace empire::audio {
float AudioVoicePool::computePan2D(const Spatial2D& src) const noexcept {
    const float dx = src.x - m_listener2D.x;
    const float dy = src.y - m_listener2D.y;
    const float c = std::cos(m_listener2D.angle);
    const float sn = std::sin(m_listener2D.angle);

    const float proj = dx * sn + dy * (-c);
    const float denom = std::max(1.0f, m_listener2D.halfViewWidth);
    float pan = proj / denom;
    return std::clamp(pan, -1.0f, 1.0f);
}
AudioVoicePool::AudioVoicePool() {
    m_pendingEvents.reserve(kMaxVoices);
}
AudioVoicePool::~AudioVoicePool() {
    uninit();
}

// Lifecycle

AudioResult<void> AudioVoicePool::init(ma_engine* engine) {
    if (engine == nullptr) {
        return AudioResult<void>::err(AudioError::NotInitialized);
    }
    // Re-binding: kill everything first. Slots go back to Free with bumped
    // generations so any outstanding handle becomes invalid.
    uninit();
    m_engine = engine;
    return AudioResult<void>::ok();
}

void AudioVoicePool::uninit() noexcept {
    for (auto& s : m_slots) {
        if (s.state != SlotState::Free) {
            // No user callback on device loss / shutdown: the caller is
            // already in a "we're resetting audio" state.
            releaseSlot(s, VoiceStopReason::Finished, /*emitEvent=*/false);
        }
    }
    m_engine = nullptr;
}

// Handle / slot resolution

AudioVoicePool::Slot* AudioVoicePool::slotFromHandle(VoiceHandle h) noexcept {
    if (!h.isValid()) {
        return nullptr;
    }
    const std::uint32_t rawIdx = h.index - 1;
    if (rawIdx >= m_slots.size()) {
        return nullptr;
    }
    Slot& s = m_slots[rawIdx];
    if (s.state == SlotState::Free || s.generation != h.generation) {
        return nullptr;
    }
    return &s;
}

const AudioVoicePool::Slot* AudioVoicePool::slotFromHandle(
    VoiceHandle h) const noexcept {
    return const_cast<AudioVoicePool*>(this)->slotFromHandle(h);
}

bool AudioVoicePool::isPlaying(VoiceHandle h) const noexcept {
    return slotFromHandle(h) != nullptr;
}

// Candidate selection

AudioVoicePool::Slot* AudioVoicePool::findFreeSlot() noexcept {
    for (auto& s : m_slots) {
        if (s.state == SlotState::Free) {
            return &s;
        }
    }
    return nullptr;
}

AudioVoicePool::Slot* AudioVoicePool::findOldestSameSound(
    std::uint32_t key) noexcept {
    Slot* best = nullptr;
    for (auto& s : m_slots) {
        if (s.state != SlotState::Playing) {
            continue;
        }
        if (s.soundGroupKey != key) {
            continue;
        }
        if (best == nullptr || s.startTick < best->startTick) {
            best = &s;
        }
    }
    return best;
}

AudioVoicePool::Slot* AudioVoicePool::findGlobalStealCandidate() noexcept {
    Slot* best = nullptr;
    for (auto& s : m_slots) {
        if (s.state != SlotState::Playing) {
            continue;
        }
        if (best == nullptr) {
            best = &s;
            continue;
        }
        if (s.priority < best->priority) {
            best = &s;
            continue;
        }
        if (s.priority == best->priority && s.startTick < best->startTick) {
            best = &s;
        }
    }
    return best;
}

// Release / arm

void AudioVoicePool::releaseSlot(Slot& s, VoiceStopReason reason,
                                 bool emitEvent) {
    if (s.state == SlotState::Free) {
        return;
    }

    // Snapshot everything we need for the event BEFORE we mutate.
    const std::uint32_t rawIdx =
        static_cast<std::uint32_t>(&s - m_slots.data());
    const std::uint32_t oldGen = s.generation;
    const std::uint32_t key = s.soundGroupKey;

    // Uninit order matters: sound (holds data source) before ref (owns the
    // buffer view the data source points at).
    s.sound.uninit();
    s.ref.uninit();
    s.resource.reset();

    s.state = SlotState::Free;
    s.soundGroupKey = 0;
    s.priority = 0;
    s.startTick = 0;
    s.finished.store(false, std::memory_order_relaxed);
    s.spatial2D.reset();

    ++s.generation;
    if (s.generation == 0) {
        s.generation = 1;  // never let 0 be a valid generation
    }

    if (m_activeCount > 0) {
        --m_activeCount;
    }

    if (emitEvent) {
        m_pendingEvents.push_back(
            VoiceFinishEvent{VoiceHandle{rawIdx + 1, oldGen}, key, reason});
    }
}

AudioResult<void> AudioVoicePool::armSlot(Slot& s, const VoiceStartDesc& desc) {
    if (desc.group == nullptr || !desc.group->isValid())
        return AudioResult<void>::err(AudioError::BusNotFound);
    const auto& res = *desc.resource;
    if (!res.isReady())
        return AudioResult<void>::err(AudioError::ResourceNotReady);

    s.resource = desc.resource;
    s.soundGroupKey = desc.soundGroupKey;
    s.priority = desc.priority;
    s.startTick = ++m_tickCounter;
    s.spatial2D = desc.spatial2D;

    if (res.isStreamed()) {
        auto r = s.sound.initFromFile(m_engine, res.streamPath(),
                                      MA_SOUND_FLAG_STREAM, desc.group->get(),
                                      nullptr, nullptr);
        if (!r.isOk()) {
            s.resource.reset();
            return r;
        }
    } else {
        auto rr = s.ref.init(res.format(), res.channels(), res.pcmData(),
                             res.frameCount());
        if (!rr.isOk()) {
            s.resource.reset();
            return rr;
        }
        auto rs = s.sound.initFromDataSource(m_engine, s.ref.dataSource(), 0,
                                             desc.group->get());
        if (!rs.isOk()) {
            s.ref.uninit();
            s.resource.reset();
            return rs;
        }
    }
    ma_sound_set_looping(s.sound.get(), desc.loop ? MA_TRUE : MA_FALSE);
    ma_sound_set_end_callback(s.sound.get(),
                              &AudioVoicePool::endCallbackTrampoline, &s);

    ma_sound_set_volume(s.sound.get(), desc.volume);
    ma_sound_set_pitch(s.sound.get(), desc.pitch);

    if (desc.spatial3D) {
        const auto& sp = *desc.spatial3D;
        ma_sound_set_position(s.sound.get(), sp.px, sp.py, sp.pz);
        ma_sound_set_attenuation_model(s.sound.get(), sp.attenuation);
        ma_sound_set_rolloff(s.sound.get(), sp.rolloff);
        ma_sound_set_min_distance(s.sound.get(), sp.minDistance);
        ma_sound_set_max_distance(s.sound.get(), sp.maxDistance);
        ma_sound_set_doppler_factor(s.sound.get(), sp.dopplerFactor);
        ma_sound_set_pan(s.sound.get(), 0.0f);
    } else if (desc.spatial2D) {
        const float pan = computePan2D(*desc.spatial2D);
        ma_sound_set_pan(s.sound.get(), computePan2D(*desc.spatial2D));
    }

    s.state = SlotState::Playing;
    s.finished.store(false, std::memory_order_relaxed);
    ++m_activeCount;
    const ma_result sr = ma_sound_start(s.sound.get());
    if (sr != MA_SUCCESS) {
        releaseSlot(s, VoiceStopReason::Stopped, false);
        return AudioResult<void>::err(detail::toAudioError(sr));
    }
    return AudioResult<void>::ok();
}

// start / stop

VoiceStartOutcome AudioVoicePool::start(const VoiceStartDesc& desc) {
    VoiceStartOutcome out;
    if (m_engine == nullptr) {
        out.result = VoiceStartResult::NotInitialized;
        return out;
    }
    if (!desc.resource || !desc.resource->isReady() || desc.group == nullptr) {
        out.result = VoiceStartResult::InvalidResource;
        return out;
    }

    std::size_t sameCount = 0;
    for (auto& s : m_slots) {
        if (s.state == SlotState::Playing &&
            s.soundGroupKey == desc.soundGroupKey) {
            ++sameCount;
        }
    }

    Slot* slot = nullptr;
    if (desc.maxConcurrent > 0 && sameCount >= desc.maxConcurrent) {
        slot = findOldestSameSound(desc.soundGroupKey);
        if (slot == nullptr) {
            out.result = VoiceStartResult::PerSoundLimitHit;
            if (m_events) {
                m_events->push(AudioEventKind::VoiceLimitHit,
                               AudioError::Unknown, desc.soundGroupKey);
            }
            return out;
        }
        releaseSlot(*slot, VoiceStopReason::Stolen, /*emitEvent=*/true);
    } else {
        slot = findFreeSlot();
        if (slot == nullptr) {
            Slot* victim = findGlobalStealCandidate();
            if (victim == nullptr) {
                out.result = VoiceStartResult::NoFreeVoice;
                if (m_events) {
                    m_events->push(AudioEventKind::VoicePoolExhausted);
                }
                return out;
            }
            if (victim->priority > desc.priority) {
                out.result = VoiceStartResult::TooLowPriority;
                if (m_events) {
                    m_events->push(AudioEventKind::VoiceLimitHit,
                                   AudioError::Unknown, desc.soundGroupKey);
                }
                return out;
            }
            releaseSlot(*victim, VoiceStopReason::Stolen, /*emitEvent=*/true);
            ++m_stolenTotal;
            if (m_events) {
                m_events->push(AudioEventKind::VoiceStolen, AudioError::Unknown,
                               victim->soundGroupKey);
            }
            slot = victim;
        }
    }

    auto r = armSlot(*slot, desc);
    if (!r.isOk()) {
        out.result = VoiceStartResult::InvalidResource;
        if (m_events) {
            m_events->push(AudioEventKind::ResourceLoadFailed, r.error(),
                           desc.soundGroupKey, 0);
        }
        firePending();
        return out;
    }

    const std::uint32_t rawIdx =
        static_cast<std::uint32_t>(slot - m_slots.data());
    out.handle = VoiceHandle{rawIdx + 1, slot->generation};
    out.result = VoiceStartResult::Started;
    firePending();
    return out;
}

void AudioVoicePool::stop(VoiceHandle h) {
    Slot* s = slotFromHandle(h);
    if (s == nullptr) return;
    releaseSlot(*s, VoiceStopReason::Stopped, /*emitEvent=*/true);
    firePending();
}

void AudioVoicePool::stopByGroupKey(std::uint32_t key) {
    for (auto& s : m_slots) {
        if (s.state == SlotState::Playing && s.soundGroupKey == key) {
            releaseSlot(s, VoiceStopReason::Stopped, /*emitEvent=*/true);
        }
    }
    firePending();
}

// Per-frame

void AudioVoicePool::update(float /*dt*/) {
    for (auto& s : m_slots) {
        if (s.state != SlotState::Playing) {
            continue;
        }

        if (s.spatial2D && s.sound.isValid()) {
            ma_sound_set_pan(s.sound.get(), computePan2D(*s.spatial2D));
        }
    }
    for (auto& s : m_slots) {
        if (s.state == SlotState::Playing &&
            s.finished.load(std::memory_order_acquire)) {
            releaseSlot(s, VoiceStopReason::Finished, true);
        }
    }
    firePending();
}

void AudioVoicePool::firePending() {
    if (m_pendingEvents.empty()) {
        return;
    }

    events::EventBus* bus = m_eventBus ? m_eventBus : events::defaultEventBus();

    std::vector<VoiceFinishEvent> batch;
    batch.swap(m_pendingEvents);
    if (!bus) {
        return;
    }
    for (const auto& ev : batch) {
        bus->emplaceDeferred<VoiceFinishEvent>(ev.handle, ev.soundGroupKey,
                                               ev.reason);
    }
}

// Audio-thread entry point

void AudioVoicePool::endCallbackTrampoline(void* userdata,
                                           ma_sound* /*sound*/) {
    if (userdata == nullptr) {
        return;
    }
    auto* s = static_cast<Slot*>(userdata);
    s->finished.store(true, std::memory_order_release);
}

void AudioVoicePool::updatePosition(VoiceHandle h, const Spatial3D& spatial) {
    Slot* s = slotFromHandle(h);
    if (s == nullptr || !s->sound.isValid()) {
        return;
    }

    ma_sound_set_position(s->sound.get(), spatial.px, spatial.py, spatial.pz);
    ma_sound_set_attenuation_model(s->sound.get(), spatial.attenuation);
    ma_sound_set_rolloff(s->sound.get(), spatial.rolloff);
    ma_sound_set_min_distance(s->sound.get(), spatial.minDistance);
    ma_sound_set_max_distance(s->sound.get(), spatial.maxDistance);
    ma_sound_set_doppler_factor(s->sound.get(), spatial.dopplerFactor);
}
}  // namespace empire::audio