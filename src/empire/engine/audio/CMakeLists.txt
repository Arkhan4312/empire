empire_add_module(engine_audio
	STATIC
	ALIAS empire::engine::audio
	SOURCES
		AudioError.h				AudioError.cpp
		AudioChannels.h				AudioChannels.cpp
		AudioBusConfig.h
		AudioBus.h
		AudioBusRegistry.h			AudioBusRegistry.cpp
		AudioResource.h				AudioResource.cpp
		AudioResourceCache.h		AudioResourceCache.cpp
		AudioSpatial.h
		AudioVoice.h
		AudioVoicePool.h			AudioVoicePool.cpp
		AudioMusicPlayer.h			AudioMusicPlayer.cpp
		AudioEventLog.h				AudioEventLog.cpp
		AudioDeviceManager.h		AudioDeviceManager.cpp
		AudioSystem.h				AudioSystem.cpp
		detail/Miniaudio.h
		detail/MaResult.h			detail/MaResult.cpp
		detail/MaRaii.h				detail/MaRaii.cpp
		detail/MaAudioBufferRef.h	detail/MaAudioBufferRef.cpp
		miniaudio_impl.cpp
	PUBLIC
		empire::includes
		empire::engine::log
		empire::engine::util
		miniaudio_headers
		nlohmann_json::nlohmann_json
)