#include "camera.h"
// Suppress warnings in third-party code.
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
DISABLE_WARNINGS_POP()

Camera::Camera(Window* pWindow)
    : Camera(pWindow, glm::vec3(0), glm::vec3(0, 0, -1)){}

Camera::Camera(Window* pWindow, const glm::vec3& pos, const glm::vec3& forward)
    : m_position(pos)
    , m_forward(glm::normalize(forward))
    , m_pWindow(pWindow)
{
}

void Camera::setUserInteraction(bool enabled)
{
    m_userInteraction = enabled;
}

glm::vec3 Camera::cameraPos() const
{
    return m_position;
}

glm::mat4 Camera::viewMatrix() const
{
    if (useLockView) {
        glm::vec3 targetPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 newForward = glm::normalize(targetPosition - m_position); // 计算指向原点的方向
        return glm::lookAt(m_position, m_position + newForward, glm::vec3(0,1,0));
    }
    else return glm::lookAt(m_position, m_position + m_forward, m_up);
}

void Camera::rotateX(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, horAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::rotateY(float angle)
{
    const glm::vec3 horAxis = glm::cross(s_yAxis, m_forward);

    m_forward = glm::normalize(glm::angleAxis(angle, s_yAxis) * m_forward);
    m_up = glm::normalize(glm::cross(m_forward, horAxis));
}

void Camera::updateInput()
{
    constexpr float moveSpeed = 0.05f;
    constexpr float lookSpeed = 0.0035f;
    
    //If Use Bezier Curve
    if (m_useBezier) {
        if (m_bezierConstantSpeed) {
            checkChange();
            if (isChangedPoints) {
                CalculateArcLengthTable();
                isChangedPoints = false;
            }
            bezierTime += bezierSpeed * bezierTimeStep;
            m_position = getBezier(FindParameterByArcLength(bezierTime));
        }
        else {
            bezierTime += bezierTimeStep;
            m_position = getBezier(bezierTime);
        }
        if (bezierTime > 1) bezierTime = 0;
    }
    else if (m_userInteraction) {
        glm::vec3 localMoveDelta{ 0 };
        const glm::vec3 right = glm::normalize(glm::cross(m_forward, m_up));
        if (m_pWindow->isKeyPressed(GLFW_KEY_A))
            m_position -= moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_D))
            m_position += moveSpeed * right;
        if (m_pWindow->isKeyPressed(GLFW_KEY_W))
            m_position += moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_S))
            m_position -= moveSpeed * m_forward;
        if (m_pWindow->isKeyPressed(GLFW_KEY_R))
            m_position += moveSpeed * m_up;
        if (m_pWindow->isKeyPressed(GLFW_KEY_F))
            m_position -= moveSpeed * m_up;

        const glm::dvec2 cursorPos = m_pWindow->getCursorPos();
        const glm::vec2 delta = lookSpeed * glm::vec2(cursorPos - m_prevCursorPos);
        m_prevCursorPos = cursorPos;

        if (m_pWindow->isMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT)) {
            if (delta.x != 0.0f)
                rotateY(delta.x);
            if (delta.y != 0.0f)
                rotateX(delta.y);
        }
    }
    else {
        m_prevCursorPos = m_pWindow->getCursorPos();
    }
}


glm::vec3 Camera::getBezier(float t) {

	float u = 1 - t;
	float uu = u * u;
	float uuu = uu * u;
	float tt = t * t;
	float ttt = tt * t;

	glm::vec3 point = uuu * P0;
	point += 3 * uu * t * P1; 
	point += 3 * u * tt * P2;
	point += ttt * P3;

    return point;
}

void Camera::CalculateArcLengthTable() {
    arcLengthTable.clear();
    arcLengthTable.push_back(0.0f);

    glm::vec3 prevPoint = P0;
    float totalLength = 0.0f;

    for (int i = 1; i < bezierConstantSpeedSampleNumber; ++i) {
        float t = static_cast<float>(i) / (bezierConstantSpeedSampleNumber - 1);
        glm::vec3 currentPoint = getBezier(t);
        float segmentLength = glm::length(currentPoint - prevPoint);
        totalLength += segmentLength;
        arcLengthTable.push_back(totalLength);
        prevPoint = currentPoint;
    }

    // Unify the Length Table
    for (int i = 0; i < bezierConstantSpeedSampleNumber; ++i) {
        arcLengthTable[i] /= totalLength;
    }

    //return arcLengthTable;
}

// Check if there is any changes for Points, if so, update arcLengthTable
void Camera::checkChange() {
    if (P0 != oldP0) {
        oldP0 = P0;
        isChangedPoints = true;
    }
    if (P1 != oldP1) {
        oldP1 = P1;
        isChangedPoints = true;
    }
    if (P2 != oldP2) {
        oldP2 = P2;
        isChangedPoints = true;
    }
    if (P3 != oldP3) {
        oldP3 = P3;
        isChangedPoints = true;
    }
    if (bezierConstantSpeedSampleNumber != oldBezierConstantSpeedSampleNumber) {
        oldBezierConstantSpeedSampleNumber = bezierConstantSpeedSampleNumber;
        isChangedPoints = true;
    }
}

float Camera::FindParameterByArcLength(float distance) {
    int low = 0;
    int high = arcLengthTable.size() - 1;

    while (low < high) {
        int mid = (low + high) / 2;
        if (arcLengthTable[mid] < distance) {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }

    return static_cast<float>(low) / (arcLengthTable.size() - 1);
}
