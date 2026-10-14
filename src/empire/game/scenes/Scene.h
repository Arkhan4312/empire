#pragma once
#include <memory>
namespace game {

struct AppContext;

class Scene {
public:
    virtual ~Scene() = default;

    virtual void onEnter(AppContext& ctx) {
        (void)ctx;
    }
    virtual void onExit(AppContext& ctx) {
        (void)ctx;
    }

    virtual void update(AppContext& ctx, double dt) = 0;
    virtual void render(AppContext& ctx) = 0;
};
using ScenePtr = std::unique_ptr<Scene>;
}  // namespace game