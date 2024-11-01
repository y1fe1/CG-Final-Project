#pragma once
#include <exception>
#include <filesystem>
#include <framework/opengl_includes.h>

class abstractTexture {
public:
	virtual ~abstractTexture() = default;

    abstractTexture() noexcept : m_texture{ INVALID } {}

	abstractTexture(abstractTexture&& other) noexcept
		: m_texture(other.m_texture)
	{
		other.m_texture = INVALID;
	}

    // Delete copy constructor and copy assignment
    abstractTexture(const abstractTexture&) = delete;
    abstractTexture& operator=(const abstractTexture&) = delete;

    // Default move assignment operator
    abstractTexture& operator=(abstractTexture&& other) noexcept {
        if (this != &other) {
            // Release any existing resource
            if (m_texture != INVALID) {
                glDeleteTextures(1, &m_texture);
            }
            // Transfer ownership
            m_texture = other.m_texture;
            other.m_texture = INVALID;
        }
        return *this;
    }


	virtual void bind(GLint) = 0;

	GLuint& getTextureRef() { return m_texture; }

protected:
	static constexpr GLuint INVALID = 0xFFFFFFFF;
	GLuint m_texture{ INVALID };
};