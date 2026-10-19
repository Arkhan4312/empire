#include <empire/engine/audio/AudioChannels.h>

#include <array>
#include <utility>

namespace empire::audio::channels {

void registerDefaults(empire::log::Logger& logger) {
    logger.registerChannel(kDevice, empire::log::Level::Info);
    logger.registerChannel(kEngine, empire::log::Level::Info);
    logger.registerChannel(kResource, empire::log::Level::Info);
    logger.registerChannel(kBus, empire::log::Level::Info);
    logger.registerChannel(kVoice, empire::log::Level::Warn);
    logger.registerChannel(kMusic, empire::log::Level::Info);
    logger.registerChannel(kSpatial, empire::log::Level::Warn);
    logger.registerChannel(kEvent, empire::log::Level::Debug);

}
}  // namespace empire::audio::channels