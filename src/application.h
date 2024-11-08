#pragma once

#include "protocol.h"
#include <framework/trackball.h>

#include "camera.h"
#include "Textures/cubeMapTexture.h"
#include "Textures/hdrTexture.h"
#include "Textures/ssaoBufferTexture.h"

#include "celestial_body.h"

#define MAX_LIGHT_CNT 10
#include "minimap.h"
#include <stb/stb_image.h>

class Application {
private:

    bool multiLightShadingEnabled = false;
    bool usePbrShading = false;

    int curDiffuseIndex = 0;
    int curSpecularIndex = 0;

    Window m_window;
    Trackball trackball;

    GLuint lightUBO = 0;

    Shader m_debugShader;
    Shader m_defaultShader;
    Shader m_multiLightShader;
    Shader m_pbrShader;
    Shader* m_selShader;

    Shader m_shadowShader;
    Shader m_lightShader;
    Shader m_borderShader;
    Shader m_pointShader;
    Shader m_postProcessShader;

    // Shader m_celestialBodyShader;

    // Definition for SkyBox 
    bool envMapEnabled = false;

    // faces should follow this format
    std::vector<std::filesystem::path> faces = {
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "right.jpg"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "left.jpg"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "top.jpg"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "bottom.jpg"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "front.jpg"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "back.jpg")
    };

    std::vector<std::filesystem::path> celestialFaces = {
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/right.png"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/left.png"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/top.png"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/bottom.png"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/front.png"),
        std::filesystem::path(RESOURCE_ROOT SKYBOX_PATH "deep_sky/back.png")
    };

    cubeMapTex* selectedSkybox;
    cubeMapTex skyboxTexture;
    cubeMapTex celestialSkyboxTexture;
    GLuint skyboxVAO,skyboxVBO;

    Shader m_skyBoxShader;

    void generateSkyBox();

    void generateHdrMap();

    // Definition HDR cubemap settings
    bool hdrMapEnabled = false;

    GLuint captureFBO, captureRBO; // framebuffers

    std::filesystem::path hdrSamplePath{ RESOURCE_ROOT HDR_PATH "Free_Galaxies_8k.hdr" };
    hdrTexture hdrTextureMap;

    cubeMapTex hdrCubeMap;
    GLuint cubeVAO = 0, cubeVBO = 0;

    Shader m_hdrToCubeShader;
    Shader m_hdrSkyBoxShader;

    cubeMapTex hdrIrradianceMap;
    Shader m_hdrToIrradianceShader;

    cubeMapTex hdrPrefilteredMap;
    Shader m_hdrPrefilterShader;

    Texture BRDFTexture;
    Shader m_brdfShader;
    GLuint quadVAO = 0, quadVBO = 0;

    /// SSAO Defintions ///

    Texture m_diffuseTex,m_specularTex;

    bool ssaoEnabled = false;
    bool defRenderBufferGenerated = false;

    bool defRenderLightGen = false;

    // SSAO Processing Shader
    Shader m_shaderGeometryPass;
    Shader m_shaderLightingPass;

    Shader m_deferredLightShader;
    Shader m_deferredDebugShader;

    GLuint gBuffer = 0;
    ssaoBufferTex gPos, gNor, gCol;

    GLuint renderDepth;
    
    void genDeferredRenderBuffer(bool& defRenderBufferGenerated);
    void deferredRenderPipeLine();

    // unused
    GLuint ssaoFBO = 0, ssaoBlurFBO = 0;
    ssaoBufferTex ssaoColorBuff, ssaoColorBlurBuff;
    ssaoBufferTex ssaoNoiseTex;

    Shader m_shaderSSAO;
    Shader m_shaderSSAOBlur;

    void genSSAOFrameBuffer();

    // Definition for model Obejcts includeing texture and Material
    std::vector<GPUMesh> m_meshes;
    Texture m_texture;

    char file_path_buffer[256];
    std::string texturePath;

    std::vector<Texture> m_pbrTextures;

    bool textureEnabled = true;

    Material m_Material;
    PBRMaterial m_PbrMaterial;

    GLuint PbrUBO;

    bool m_useMaterial = true;
    bool m_materialChangedByUser = false;

    int curMaterialIndex = 0;

    // mvp matrices

    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_modelMatrix;


    // Definition for Cameras
    std::vector<Camera> cameras;
    Camera* selectedCamera;

    // Definition for Lights
    std::vector<Light> lights{};
    Light* selectedLight;

    //Shadow
    shadowSetting shadowSettings;
    ShadowTexture m_shadowTex;

    //const int SHADOWTEX_WIDTH = 1024;
    //const int SHADOWTEX_HEIGHT = 1024;
    
    void imgui();

    //Normal mapping
    bool useNormalMapping { false };
    bool useParallaxMapping { false };
    GLuint normalTex;
    void applyNormalTexture();
    
    //Minimap
    void renderMiniMapItem(glm::mat4 modelMatrix);
    void renderMiniMap();
    void drawMiniMapBorder();
    void drawCameraPositionOnMinimap(const glm::vec4& cameraPosInMinimap);
    Minimap minimap;
    bool render_minimap = false;

    //Post-Process Shader
    bool usePostProcess = false;
    void initPostProcess();
    void runPostProcess();
    GLuint framebufferPostProcess;
    GLuint texturePostProcess;
    GLuint depthbufferPostProcess;
    const int WINDOW_WIDTH = 1024;
    const int WINDOW_HEIGHT = 1024;
    glm::ivec2 windowSizes;

    //Re-Structure the code
    void initPBRTexures();
    void initMaterialTexture();

    void drawEnvMap(bool envMapEnabled, bool hdrMapEnabled);
    void drawMultiLightShader(GPUMesh& mesh, bool multiLightShadingEnabled);


    //Hierarchical transformation
    bool showSolarSystem = false;
    glm::uint frame = 0;
    std::array<CelestialBody, 3> celestialBodies;
    std::map<std::string, Texture> celestialTextures;
    void initCelestialTextures();
    Texture* findCelestialTexture(std::string celestialTexturePath);
    void updateFrameNumber();
    void renderSolarSystem();
    bool moveCelestialBodies = false;
    float sunlight_strength = 2.8f;
    Light sun_light;

public:
    Application();
    void update();
    void onKeyPressed(int key, int mods);
    void onKeyReleased(int key, int mods);
    void onMouseMove(const glm::dvec2& cursorPos);
    void onMouseClicked(int button, int mods);
    void onMouseReleased(int button, int mods);

};