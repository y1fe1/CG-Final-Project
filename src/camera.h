// Suppress warnings in third-party code
#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

inline size_t curCameraIndex = 0;

class Camera {
public:
    Camera(Window* pWindow);
    Camera(Window* pWindow, const glm::vec3& position, const glm::vec3& forward);

    void updateInput();
    void setUserInteraction(bool enabled);

    glm::vec3 cameraPos() const;
    glm::mat4 viewMatrix() const;
    bool m_useBezier{ false };
    bool m_bezierConstantSpeed{ false };

    //Bezier params
    glm::vec3 P0 = { 0.0f, 4.0f, 0.0f };
    glm::vec3 P1 = { 0.0f, 6.0f, -8.0f };
    glm::vec3 P2 = { 5.0f, 7.0f, -15.0f };
    glm::vec3 P3 = { 0.0f, 4.0f, 0.0f };
    

    float bezierTimeStep = 0.001f;
    int bezierConstantSpeedSampleNumber = 1000;
    int bezierSpeed = 1;

private:
    void rotateX(float angle);
    void rotateY(float angle);
    float bezierTime = 0;

private:
    glm::vec3 getBezier(float t);
    void CalculateArcLengthTable();
    float FindParameterByArcLength(float distance);

    static constexpr glm::vec3 s_yAxis{ 0, 1, 0 };
    glm::vec3 m_position{ 0 };
    glm::vec3 m_forward{ 0, 0, -1 };
    glm::vec3 m_up{ 0, 1, 0 };

    const Window* m_pWindow;
    bool m_userInteraction{ true };

    glm::dvec2 m_prevCursorPos{ 0 };

    //Constant params
    glm::vec3 oldP0 = { 0.0f, 4.0f, 0.0f };
    glm::vec3 oldP1 = { 0.0f, 6.0f, -8.0f };
    glm::vec3 oldP2 = { 5.0f, 7.0f, -15.0f };
    glm::vec3 oldP3 = { 0.0f, 4.0f, 0.0f };
    int oldBezierConstantSpeedSampleNumber = 1000;
    bool isChangedPoints = { true };
    void checkChange();
    std::vector<float> arcLengthTable;

};


