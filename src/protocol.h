#pragma once

#include "mesh.h"
#include "texture.h"
// Always include window first (because it includes glfw, which includes GL which needs to be included AFTER glew).
// Can't wait for modules to fix this stuff...
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glad/glad.h>
// Include glad before glfw3
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <imgui/imgui.h>
DISABLE_WARNINGS_POP()
#include <framework/shader.h>
#include <framework/window.h>
#include <functional>

#include <iostream>
#include <vector>


// Lets Define Useful Struct Here

struct shadingData {
    // Diffuse (Lambert)
    glm::vec3 kd{ 0.5f };
    // Specular (Phong/Blinn Phong)
    glm::vec3 ks{ 0.5f };
    float shininess = 3.0f;
    // Toon
    int toonDiscretize = 4;
    float toonSpecularThreshold = 0.49f;
};

inline size_t curLightIndex = 0;

struct Light {

    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 direction;

    bool is_spotlight;
    bool has_texture;

    std::optional<Texture> texture;
};

inline Light defaultLight = { glm::vec3(0, 0, 3), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, std::nullopt };


template <typename T>
void resetObjList(std::vector<T>& objects, T& defaultObject) {
    objects.clear();
    objects.push_back(defaultObject);
}


