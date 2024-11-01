#pragma once

#include "mesh.h"
#include "Textures/texture.h"
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

inline const int WIDTH = 1920;
inline const int HEIGHT = 1080;

struct textureLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct PBRMaterial {
    glm::vec3 albedo;
    float metallic;
    float roughness;
    float ao; // Ambiend Occlusion

    float _UNUSE_PADDING0;

    PBRMaterial(const glm::vec3& _albedo, float _metallic, float _roughness, float _ao): 
        albedo(_albedo), 
        metallic(_metallic), 
        roughness(_roughness), 
        ao(_ao), 
        _UNUSE_PADDING0(0.0f) {
        // padding is set to 0.0f, but it can be ignored since its only purpose is alignment
    }
};

enum class MaterialModel {
    NORMAL,
    PBR,
    CNT,
};

inline std::array<const char*, 2> materialModelNames{ "normal","PBR Material" };


    #pragma region LightRelated
struct Light {
    glm::vec3 position;
    float _UNUSE_PADDING0;

    glm::vec3 color;
    float _UNUSE_PADDING1;

    glm::vec3 direction;
    float _UNUSE_PADDING2;

    bool is_spotlight;
    bool has_texture;
    uint8_t _UNUSE_PADDING3[2];

    Light(): 
        position(glm::vec3(0.0f)), _UNUSE_PADDING0(0.0f),

        color(glm::vec3(1.0f)), _UNUSE_PADDING1(0.0f),

        direction(-glm::vec3(0.0f, 0.0f, 1.0f)), _UNUSE_PADDING2(0.0f),

        is_spotlight(false), has_texture(false),_UNUSE_PADDING3{ 0, 0 } 
    {}

    Light(glm::vec3 pos, glm::vec3 col, glm::vec3 dir, bool spotlight, bool texture):
        position(pos), _UNUSE_PADDING0(0.0f),

        color(col), _UNUSE_PADDING1(0.0f),

        direction(dir), _UNUSE_PADDING2(0.0f),

        is_spotlight(spotlight), has_texture(texture),_UNUSE_PADDING3{ 0, 0 } 
    {}
    //std::shared_ptr<Texture> texture; light Texture not used yet
};

inline const int MAXLIGHT = 10;
inline size_t curLightIndex = 0;

inline Light defaultLight = { glm::vec3(0, 0, 3), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/};

#pragma endregion

    #pragma region ShadowRelated
struct shadowLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct shadowSetting {
    alignas(4) bool shadowEnabled = 0;  // 4 bytes
    alignas(4) bool pcfEnabled = 0;     // 4 bytes
    alignas(4) bool _UNUSE_PADDING0 = 0; // Additional 8 bytes for 16-byte alignment
    alignas(4) bool _UNUSE_PADDING1 = 0;
};

struct ShadowTexture {
public:
    ShadowTexture() {
        m_texture = INVALID;
    }

    ShadowTexture(const int SHADOWTEX_WIDTH, const int SHADOWTEX_HEIGHT) {
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // Set behavior for when texture coordinates are outside the [0, 1] range.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Set interpolation for texture sampling (GL_NEAREST for no interpolation).
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &m_frameBuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_texture, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    ShadowTexture(const ShadowTexture&) = delete;

    ShadowTexture(ShadowTexture&& other) : m_texture(other.m_texture)
    {
        other.m_texture = INVALID;
    }

    ~ShadowTexture() {
        if (m_texture != INVALID)
            glDeleteTextures(1, &m_texture);
    }

    ShadowTexture& operator=(const ShadowTexture&) = delete;
    ShadowTexture& operator=(ShadowTexture&&) = default;

    void bind(GLint textureSlot) {
        glActiveTexture(textureSlot);
        glBindTexture(GL_TEXTURE_2D, m_texture);
    }

    GLuint& getFramebuffer() {
        return m_frameBuffer;
    }

private:
    static constexpr GLuint INVALID = 0xFFFFFFFF;
    GLuint m_texture{ INVALID };
    GLuint m_frameBuffer{ INVALID };
};
#pragma endregion



template <typename T>
void resetObjList(std::vector<T>& objects, T& defaultObject) {
    objects.clear();
    objects.push_back(defaultObject);
}

template <typename T> 
void genUboBufferObj(std::vector<T>& objLists, GLuint& selUboBuffer, int maxObjListCnt) {
    glGenBuffers(1, &selUboBuffer);
    if (selUboBuffer == 0) {
        // Handle error if buffer generation failed
        std::cerr << "Error generating UBO" << std::endl;
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, selUboBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(T) * maxObjListCnt, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(T) * objLists.size(), objLists.data());
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

template <typename T>
void genUboBufferObj(T& object, GLuint& selUboBuffer) {
    glGenBuffers(1, &selUboBuffer);

    if (selUboBuffer == 0) {
        // Handle error if buffer generation failed
        std::cerr << "Error generating UBO" << std::endl;
        return;
    }

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

inline void renderHDRCubeMap(GLuint& cubeVAO, GLuint& cubeVBO, const float* vertices, size_t vertexCount) {

    // init if nessccary
    if (cubeVAO == 0)
    {
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

inline void renderQuad(GLuint& quadVAO, GLuint& quadVBO, const float* vertices, size_t vertexCount) {
    if (quadVAO == 0)
    {
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

inline const float quadVertices[] = {
    // positions        // texture Coords
    -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
};