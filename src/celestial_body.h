#pragma once

#include "protocol.h"

class CelestialBody
{
private:
    float orbitRadius = 0.0f;

    glm::mat4 matrix = glm::mat4(1.0f);
    float speed;
    float radius;
    float yFactor = 0.0f;
    bool stationary = false;

public:
    CelestialBody() {}

    glm::mat4 getMatrix() { return matrix; }
    float getOrbitRadius() { return orbitRadius; }

    void updateBodyPosition(uint frame, glm::mat4& orbitOriginMatrix, float orbitR)
    {
        if (stationary)
        {
            matrix = glm::scale(translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f)), glm::vec3(radius));
            return;
        }

        float angle = speed * (float)frame * 2.5f;
        glm::vec3 translation = glm::vec3(
            orbitR * cos(angle),
            yFactor * cos(angle),
            orbitR * sin(angle)
        ) + glm::vec3(orbitOriginMatrix[3]);

        glm::mat4 newMatrix = glm::mat4(1.0f);
        newMatrix = glm::scale(translate(newMatrix, translation), glm::vec3(radius));

        // matrix = translate(newMatrix, glm::vec3(orbitOriginMatrix[3]));
        matrix = newMatrix;
    }

    static CelestialBody Sun()
    {
        CelestialBody body{};
        body.orbitRadius = 7.5f;
        body.radius = 2.5f;
        body.stationary = true;
        return body;
    }

    static CelestialBody Earth()
    {
        CelestialBody body{};
        body.orbitRadius = 1.4f;
        body.speed = 0.001f;
        body.radius = 1.0f;
        body.yFactor = -0.2f;
        return body;
    }

    static CelestialBody Moon()
    {
        CelestialBody body{};
        body.speed = 0.003f;
        body.radius = 0.3f;
        body.yFactor = 0.7f;
        return body;
    }
};
