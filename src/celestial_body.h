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
    bool rotateAroundAxis = false;

    glm::vec3 _kd = glm::vec3(1.0f);

    std::string texturePath;

public:
    CelestialBody() {}

    glm::mat4 getMatrix() { return matrix; }
    float getOrbitRadius() { return orbitRadius; }
    std::string getTexturePath() { return texturePath; }
    glm::vec3 kd() { return _kd; }

    void updateBodyPosition(uint frame, glm::mat4& orbitOriginMatrix, float orbitR)
    {
        glm::mat4 newMatrix = glm::mat4(1.0f);
        float angle = glm::radians(speed * (float)frame);

        if (stationary)
        {
            newMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(radius));
        }
        else
        {
            glm::vec3 translation = glm::vec3(
                orbitR * cos(angle),
                yFactor * cos(angle),
                orbitR * sin(angle)
            ) + glm::vec3(orbitOriginMatrix[3]);

            newMatrix = glm::scale(translate(newMatrix, translation), glm::vec3(radius));
        }

        if (rotateAroundAxis)
        {
            // Rotation around the body's own axis
            glm::vec3 rotationAxis = glm::vec3(0.15, -1, -0.15);
            newMatrix = glm::rotate(newMatrix, angle * 4, rotationAxis);
        }

        matrix = newMatrix;
    }

    static CelestialBody Sun()
    {
        CelestialBody body{};
        body.texturePath = "resources/celestial_bodies/sun";
        body._kd = glm::vec3(0.998f, 0.898f, 0.439f);
        body.orbitRadius = 12.5f;
        body.radius = 3.0f;
        body.stationary = true;
        body.speed = 0.005f;
        body.rotateAroundAxis = true;
        return body;
    }

    static CelestialBody Earth()
    {
        CelestialBody body{};
        body.texturePath = "resources/celestial_bodies/earth";
        body.orbitRadius = 1.9f;
        body.speed = 0.1f;
        body.radius = 1.0f;
        body.yFactor = -0.2f;
        body.rotateAroundAxis = true;
        return body;
    }

    static CelestialBody Moon()
    {
        CelestialBody body{};
        body.texturePath = "resources/celestial_bodies/moon";
        body.speed = 1.0f;
        body.radius = 0.3f;
        body.yFactor = 0.7f;
        return body;
    }
};
