#include "minimap.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()

//Minimap::Minimap(Window* pWindow)
//    : Minimap(pWindow, glm::vec3(0), glm::vec3(0, 0, -1)) {}

//Minimap::Minimap(const glm::vec3& pos, const glm::vec3& forward)
//    : m_position(pos)
//    , m_forward(glm::normalize(forward))
//{
//}

glm::vec3 Minimap::cameraPos() const
{
    return m_position;
}

glm::mat4 Minimap::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_forward, m_up);
}

glm::mat4 Minimap::projectionMatrix() const 
{
    //We could modify this later
    //glm::ortho(left, right, bottom, top, near, far)��
    return glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 0.1f, 30.0f);
}
