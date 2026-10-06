#pragma once
#include "scenes/Scene.h"
#include "ui/Widgets.h"

namespace game {

class GameScene : public Scene {
public:
    GameScene() = default;

    void onEnter(AppContext& ctx) override;
    void onExit(AppContext& ctx) override;
    void update(AppContext& ctx, double dt) override;
    void render(AppContext& ctx) override;

private:
    struct FloatingText {
        std::string text;
        glm::vec2 pos{0.0f};
        glm::vec2 velocity{0.0f, -55.0f};
        glm::vec4 color{1.0f};
        float age = 0.0f;
        float life = 1.0f;
        float scale = 1.25f;
    };

    struct Notification {
        std::string text;
        glm::vec4 color{1.0f};
        float age = 0.0f;
        float life = 3.0f;
    };

    void buildUi(AppContext& ctx);
    void refreshLabels(AppContext& ctx);

    void spawnFloater(AppContext& ctx, std::string text, glm::vec4 color);
    void spawnNotification(std::string text, glm::vec4 color);
    void drainEvents(AppContext& ctx);
    void updateTransient(double dt);
    void renderTransient(AppContext& ctx);

    ui::Container m_hud;
    ui::Label* m_resourceLabel = nullptr;
    ui::Label* m_boss = nullptr;
    ui::Label* m_dps = nullptr;

    ui::Container m_actions;
    ui::Button* m_click = nullptr;
    ui::Button* m_upgrade = nullptr;
    ui::Button* m_soldier = nullptr;
    ui::Button* m_tank = nullptr;
    ui::Button* m_plane = nullptr;
    ui::Button* m_menu = nullptr;

    std::vector<FloatingText> m_floaters;
    std::vector<Notification> m_notifications;
};
}  // namespace game