#pragma once

#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include <iostream>
#include "hdrTexture.h"


hdrTexture::hdrTexture()
{
    m_texture = INVALID;
}

hdrTexture::hdrTexture(std::filesystem::path filePath)
{
    stbi_set_flip_vertically_on_load(true);

    int width, height, nrComponents;
    float* data = stbi_loadf(filePath.string().c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {
        // Create a texture on the GPU and bind it for parameter setting
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data); // note how we specify the texture's data value to be float

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);


        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }
}

void hdrTexture::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_2D, m_texture);
}