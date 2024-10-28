#include "mesh.h"
#include <framework/disable_all_warnings.h>
DISABLE_WARNINGS_PUSH()
#include <fmt/format.h>
DISABLE_WARNINGS_POP()
#include <iostream>
#include <vector>

GPUMaterial::GPUMaterial(const Material& material) :
    kd(material.kd),
    ks(material.ks),
    shininess(material.shininess),
    transparency(material.transparency)
{}

GPUMesh::GPUMesh(const Mesh& cpuMesh)
{
    // Create uniform buffer to store mesh material (https://learnopengl.com/Advanced-OpenGL/Advanced-GLSL)
    GPUMaterial gpuMaterial(cpuMesh.material);
    glGenBuffers(1, &m_uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, m_uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(GPUMaterial), &gpuMaterial, GL_STATIC_READ);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Figure out if this mesh has texture coordinates
    m_hasTextureCoords = static_cast<bool>(cpuMesh.material.kdTexture);

    // Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // Create vertex buffer object (VBO)
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.vertices.size() * sizeof(decltype(cpuMesh.vertices)::value_type)), cpuMesh.vertices.data(), GL_STATIC_DRAW);

    // Create index buffer object (IBO)
    glGenBuffers(1, &m_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cpuMesh.triangles.size() * sizeof(decltype(cpuMesh.triangles)::value_type)), cpuMesh.triangles.data(), GL_STATIC_DRAW);

    // Tell OpenGL that we will be using vertex attributes 0, 1 and 2.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    // We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // Reuse all attributes for each instance
    glVertexAttribDivisor(0, 0);
    glVertexAttribDivisor(1, 0);
    glVertexAttribDivisor(2, 0);

    // Each triangle has 3 vertices.
    m_numIndices = static_cast<GLsizei>(3 * cpuMesh.triangles.size());

    // Set up shadow Vao
    initializeShadowVAO();
}

GPUMesh::GPUMesh(GPUMesh&& other)
{
    moveInto(std::move(other));
}

GPUMesh::~GPUMesh()
{
    freeGpuMemory();
}

GPUMesh& GPUMesh::operator=(GPUMesh&& other)
{
    moveInto(std::move(other));
    return *this;
}

std::vector<GPUMesh> GPUMesh::loadMeshGPU(std::filesystem::path filePath, bool normalize) {
    if (!std::filesystem::exists(filePath))
        throw MeshLoadingException(fmt::format("File {} does not exist", filePath.string().c_str()));

    // Generate GPU-side meshes for all sub-meshes
    std::vector<Mesh> subMeshes = loadMesh(filePath, normalize);
    std::vector<GPUMesh> gpuMeshes;

    for (const Mesh& mesh : subMeshes) { 
        gpuMeshes.emplace_back(mesh); 
    }
    
    return gpuMeshes;
}

bool GPUMesh::hasTextureCoords() const
{
    return m_hasTextureCoords;
}

GLuint GPUMesh::getVao(){
    return m_vao;
}

GLuint GPUMesh::getShadowVao()
{
    return m_shadowVao;
}

void GPUMesh::setUBOMaterial(GLuint newUboMaterial)
{
    this->m_uboMaterial = std::move(newUboMaterial);
}



void GPUMesh::draw(const Shader& drawingShader)
{
    // Bind material data uniform (we assume that the uniform buffer objects is always called 'Material')
    // Yes, we could define the binding inside the shader itself, but that would break on OpenGL versions below 4.2
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);

    // Draw the mesh's triangles
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GPUMesh::draw(const Shader& drawingShader, GLuint& drawingUBO, bool multiLightShadingEnabled = false)
{
    // Bind material data uniform
    drawingShader.bindUniformBlock("Material", 0, m_uboMaterial);

    if (!multiLightShadingEnabled) {
        drawingShader.bindUniformBlock("Light", 1, drawingUBO);
    }
    else {
        drawingShader.bindUniformBlock("lights", 1, drawingUBO);
    }

    // Draw the mesh's triangles
    glBindVertexArray(m_vao);

    glViewport(0, 0, WIDTH, HEIGHT);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

void GPUMesh::drawPBR(const Shader& drawingShader, GLuint& PbrUbo, GLuint& drawingUBO)
{
    // Bind material data uniform
    drawingShader.bindUniformBlock("PBR_Material", 0, PbrUbo);
    drawingShader.bindUniformBlock("lights", 1, drawingUBO);

    // Draw the mesh's triangles
    glBindVertexArray(m_vao);

    glViewport(0, 0, WIDTH, HEIGHT);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

void GPUMesh::drawBasic(const Shader& drawingShader)
{
    // Draw the mesh's triangles
    glBindVertexArray(m_vao);

    glViewport(0, 0, WIDTH, HEIGHT);
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);

    glBindVertexArray(0);
}

void GPUMesh::drawShadowMap(const Shader& shadowShader, glm::mat4 lightMVP, GLuint texShadowBuffer, const int SHADOWTEX_WIDTH, const int SHADOWTEX_HEIGHT)
{
    glBindFramebuffer(GL_FRAMEBUFFER, texShadowBuffer);

    // Clear the shadow map and set needed options
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    shadowShader.bind();
    // Set viewport size
    glViewport(0, 0, SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);

    glUniformMatrix4fv(shadowShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(lightMVP));

    // Bind vertex data
    glBindVertexArray(m_vao);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Execute draw command
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);

    // Unbind the off-screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GPUMesh::moveInto(GPUMesh&& other)
{
    freeGpuMemory();
    m_numIndices = other.m_numIndices;
    m_hasTextureCoords = other.m_hasTextureCoords;
    m_ibo = other.m_ibo;
    m_vbo = other.m_vbo;
    m_vao = other.m_vao;
    m_uboMaterial = other.m_uboMaterial;

    other.m_numIndices = 0;
    other.m_hasTextureCoords = other.m_hasTextureCoords;
    other.m_ibo = INVALID;
    other.m_vbo = INVALID;
    other.m_vao = INVALID;
    other.m_uboMaterial = INVALID;
}

void GPUMesh::freeGpuMemory()
{
    if (m_vao != INVALID)
        glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != INVALID)
        glDeleteBuffers(1, &m_vbo);
    if (m_ibo != INVALID)
        glDeleteBuffers(1, &m_ibo);
    if (m_uboMaterial != INVALID)
        glDeleteBuffers(1, &m_uboMaterial);
}

void GPUMesh::initializeShadowVAO()
{
    glGenVertexArrays(1, &m_shadowVao);
    glBindVertexArray(m_shadowVao);

    // Bind VBO and IBO
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);

    // Set vertex attribute for shadow mapping (e.g., only position)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Unbind VAO
    glBindVertexArray(0);
}
