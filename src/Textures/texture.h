#pragma once
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <glm/vec3.hpp>
DISABLE_WARNINGS_POP()

#include "absTexture.h"

#define BRDF_2D_TEXTURE 10

struct ImageLoadingException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Texture: public abstractTexture{
public:
    Texture(std::filesystem::path filePath);

    Texture(int textureGenCod); // Constructor specified for generating BRDF Texture

    Texture(const Texture&) = delete;
    Texture(Texture&&) noexcept;
    ~Texture();

    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) = default;

    void bind(GLint textureSlot) override;
};
