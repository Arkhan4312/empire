#pragma once

#include <empire/engine/audio/detail/MiniAudio.h>

namespace empire::audio {

struct Listener2D {
    float x = 0.0f;
    float y = 0.0f;
    float angle = 0.0f;
    float halfViewWidth = 640.0f;
};

struct Listener3D {
    float px = 0.0f;
    float py = 0.0f;
    float pz = 0.0f;
    float fx = 0.0f;
    float fy = 0.0f;
    float fz = 0.0f;
    float ux = 0.0f;
    float uy = 0.0f;
    float uz = 0.0f;
};

struct Spatial2D {
    float x = 0.0f;
    float y = 0.0f;
};

struct Spatial3D {
    float px = 0.0f;
    float py = 0.0f;
    float pz = 0.0f;
    ma_attenuation_model attenuation = ma_attenuation_model_inverse;
    float rolloff = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
    float dopplerFactor = 0.0f;
};
}  // namespace empire::audio