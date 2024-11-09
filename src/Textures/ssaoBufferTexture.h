#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include "absTexture.h"

#define SSAO_GBUFFER_POS 1
#define SSAO_GBUFFER_NOR 2
#define SSAO_GBUFFER_COL 3
#define SSAO_COLOR_BUFF 4
#define SSAO_COLOR_BLUR 5
#define SSAO_NOISE_TEX 6

typedef int TexGenCode;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;

class ssaoBufferTex : public abstractTexture {
public:
    ssaoBufferTex() = default;

    ssaoBufferTex(TexGenCode textureGenCod); // Constructor specified for generating BRDF Texture
    ssaoBufferTex(TexGenCode textureGenCod, std::vector<glm::vec3> ssaoNoise);

    ssaoBufferTex(const ssaoBufferTex&) = delete;

    ssaoBufferTex(ssaoBufferTex&& other) noexcept;

    ~ssaoBufferTex();

    ssaoBufferTex& operator=(const ssaoBufferTex&) = delete;
    ssaoBufferTex& operator=(ssaoBufferTex&&) = default;

    void bind(GLint textureSlot) override;
    
    int gBufferCode = 0;
};