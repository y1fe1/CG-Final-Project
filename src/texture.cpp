#include "texture.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include <iostream>

Texture::Texture(std::filesystem::path filePath)
{
    // Load image from disk to CPU memory.
    // Image class is defined in <framework/image.h>
    Image cpuTexture { filePath };

    // Create a texture on the GPU and bind it for parameter setting
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    // Set behavior for when texture coordinates are outside the [0, 1] range (wrap around).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set interpolation for texture sampling (bilinear interpolation across mip-maps).
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Define GPU texture parameters and upload corresponding data based on number of image channels
    switch (cpuTexture.channels) {
        case 1:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, cpuTexture.width, cpuTexture.height, 0, GL_RED, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, cpuTexture.width, cpuTexture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cpuTexture.width, cpuTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpuTexture.get_data());
            break;
        default:
            std::cerr << "Number of channels read for texture is not supported" << std::endl;
            throw std::exception();
    }

    // Generate mip-maps
    glGenerateMipmap(GL_TEXTURE_2D);
}

Texture::Texture(Texture&& other)
    : m_texture(other.m_texture)
{
    other.m_texture = INVALID;
}

Texture::~Texture()
{
    if (m_texture != INVALID)
        glDeleteTextures(1, &m_texture);
}

void Texture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}
