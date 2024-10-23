#pragma once

#include "protocol.h"
#include "camera.h"
#include "protocol.h"

class Application {
private:
    Window m_window;
    Shader m_defaultShader;
    Shader m_shadowShader;
    std::vector<GPUMesh> m_meshes;
    Texture m_texture;
    bool m_useMaterial{ true };

    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_modelMatrix;

    std::vector<Camera> cameras;
    Camera* selectedCamera;

    std::vector<Light> lights;
    Light* selectedLight;

    void imgui();

public:
    Application();
    void update();
    void onKeyPressed(int key, int mods);
    void onKeyReleased(int key, int mods);
    void onMouseMove(const glm::dvec2& cursorPos);
    void onMouseClicked(int button, int mods);
    void onMouseReleased(int button, int mods);
};