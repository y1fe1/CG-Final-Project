#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <framework/image.h>

#include <iostream>
#include "cubeMapTexture.h"

cubeMapTex::cubeMapTex(std::vector<std::filesystem::path> filePaths)
{
    // Create a texture on the GPU and bind it for parameter setting
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);

    for (int i = 0; i < filePaths.size(); ++i) {

        try {
            auto filePath = filePaths[i];
            Image cpuTexture{ filePath };

                // Define GPU texture parameters and upload corresponding data based on number of image channels
            switch (cpuTexture.channels) {
                case 1:
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RED, cpuTexture.width, cpuTexture.height, 0, GL_RED, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                    break;
                case 3:
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, cpuTexture.width, cpuTexture.height, 0, GL_RGB, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                    break;
                case 4:
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, cpuTexture.width, cpuTexture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpuTexture.get_data());
                    break;
                default:
                    std::cerr << "Number of channels read for texture is not supported" << std::endl;
                    throw std::exception();
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        catch (ImageLoadingException e){
            std::cerr << e.what() << std::endl;
        }
    }
}

void cubeMapTex::bind(GLint textureSlot)
{
    glActiveTexture(textureSlot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
}
