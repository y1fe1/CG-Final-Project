// Suppress warnings in third-party code
#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()
#include <framework/window.h>

class Minimap {
public:
    //Minimap(Window* pWindow);
    //Minimap(const glm::vec3& position, const glm::vec3& forward);

    //void updateInput();
    //void setUserInteraction(bool enabled);
    glm::vec3 cameraPos() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix() const;//Undefined

private:
    //static constexpr glm::vec3 s_yAxis{ 0, 1, 0 };
    glm::vec3 m_position{ 0, 20, 0 };   
    glm::vec3 m_forward{ 0, -1, 0 };    
    glm::vec3 m_up{ 0, 0, -1 };         


    //glm::dvec2 m_prevCursorPos{ 0 };

};


