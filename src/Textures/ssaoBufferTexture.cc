#pragma once

#include "ssaoBufferTexture.h"
ssaoBufferTex::ssaoBufferTex(TexGenCode textureGenCod) : 
	gBufferCode{textureGenCod}
{
	// Create a texture on the GPU and bind it for parameter setting
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);


	if (textureGenCod == SSAO_GBUFFER_POS)  // generate gpos buffer
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	else if (textureGenCod == SSAO_GBUFFER_NOR) {

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	else if (textureGenCod == SSAO_GBUFFER_COL) {

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	else if (textureGenCod == SSAO_COLOR_BUFF) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	else if (textureGenCod == SSAO_COLOR_BLUR) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, SCREEN_WIDTH, SCREEN_HEIGHT, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

ssaoBufferTex::ssaoBufferTex(TexGenCode textureGenCod, std::vector<glm::vec3> ssaoNoise) :
	gBufferCode{ textureGenCod }
{
	// Create a texture on the GPU and bind it for parameter setting
	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	if (textureGenCod == SSAO_NOISE_TEX) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4, 4, 0, GL_RGB, GL_FLOAT, &ssaoNoise[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
}

ssaoBufferTex::ssaoBufferTex(ssaoBufferTex&& other) noexcept
	: gBufferCode(other.gBufferCode), abstractTexture(std::move(other))
{
	other.gBufferCode = 0;
}


ssaoBufferTex::~ssaoBufferTex()
{
	if (m_texture != INVALID)
		glDeleteTextures(1, &m_texture);
}

void ssaoBufferTex::bind(GLint textureSlot)
{
	glActiveTexture(textureSlot);
	glBindTexture(GL_TEXTURE_2D, m_texture);
}
