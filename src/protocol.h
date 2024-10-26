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
#include <framework/mesh.h>
#include <functional>
#include <array>
#include <iostream>
#include <vector>

#include "camera.h"


inline const int WIDTH = 1200;
inline const int HEIGHT = 800;

struct shadowSetting {
    bool shadowEnabled = false;
    bool pcfEnabled = false;
};

inline const int MAXLIGHT = 10;
inline size_t curLightIndex = 0;

struct Light {

    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 direction;

    bool is_spotlight;
    bool has_texture;

    //std::optional<Texture> texture;
};

inline Light defaultLight = { glm::vec3(0, 0, 3), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/};


template <typename T>
void resetObjList(std::vector<T>& objects, T& defaultObject) {
    objects.clear();
    objects.push_back(defaultObject);
}


template <typename T>
void genUboBufferObj(std::vector<T>& objLists, GLuint& selUboBuffer) {
    glGenBuffers(1, &selUboBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, selUboBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(T) * objLists.size(), objLists.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

template <typename T>
void genUboBufferObj(T& object, GLuint& selUboBuffer) {
    glGenBuffers(1, &selUboBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, selUboBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(T), &object, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}


// Predefined Material definitions for testing, retrieved from http://www.it.hiof.no/~borres/j3d/explain/light/p-materials.html
inline Material brass = { glm::vec3(0.780392f, 0.568627f, 0.113725f), glm::vec3(0.992157f, 0.941176f, 0.807843f), 27.8974f };
inline Material bronze = { glm::vec3(0.714f, 0.4284f, 0.18144f), glm::vec3(0.393548f, 0.271906f, 0.166721f), 25.6f };
inline Material polishedBronze = { glm::vec3(0.4f, 0.2368f, 0.1036f), glm::vec3(0.774597f, 0.458561f, 0.200621f), 76.8f };
inline Material chrome = { glm::vec3(0.4f, 0.4f, 0.4f), glm::vec3(0.774597f, 0.774597f, 0.774597f), 76.8f };
inline Material copper = { glm::vec3(0.7038f, 0.27048f, 0.0828f), glm::vec3(0.256777f, 0.137622f, 0.086014f), 12.8f };
inline Material polishedCopper = { glm::vec3(0.5508f, 0.2118f, 0.066f), glm::vec3(0.580594f, 0.223257f, 0.0695701f), 51.2f };
inline Material gold = { glm::vec3(0.75164f, 0.60648f, 0.22648f), glm::vec3(0.628281f, 0.555802f, 0.366065f), 51.2f };
inline Material polishedGold = { glm::vec3(0.34615f, 0.3143f, 0.0903f), glm::vec3(0.797357f, 0.723991f, 0.208006f), 83.2f };


inline const std::unordered_map<std::string, Material*> materials = {
    { "Brass", &brass },
    { "Bronze", &bronze },
    { "Polished Bronze", &polishedBronze },
    { "Chrome", &chrome },
    { "Copper", &copper },
    { "Polished Copper", &polishedCopper },
    { "Gold", &gold },
    { "Polished Gold", &polishedGold }
};

inline std::vector<std::string> materialNames; // To store material names for ComboBox
inline int selectedMaterialIndex = 0; // Index of the selected material

