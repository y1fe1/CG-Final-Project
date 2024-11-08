// Define More header if needed
#include "application.h"

// Constructor
Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , texturePath("resources/texture/brickwall.jpg")
    // , texturePath("resources/celestial_bodies/moon.jpg")
    , m_texture(RESOURCE_ROOT "resources/texture/checkerboard.png")
    , m_projectionMatrix(glm::perspective(glm::radians(80.0f), m_window.getAspectRatio(), 0.1f, 30.0f))
    , m_viewMatrix(glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0)))
    , m_modelMatrix(1.0f)
    , cameras{
        { &m_window, glm::vec3(1.2f, 1.1f, 0.9f), -glm::vec3(1.2f, 1.1f, 0.9f) },
        { &m_window, glm::vec3(3.8f, 1.0f, 0.06f), -glm::vec3(3.8f, 1.0f, 0.06f) }
    }
    ,
    // init PBR material
    m_PbrMaterial{ glm::vec3{ 0.8, 0.6, 0.4 }, 1.0f, 0.2f, 1.0f }

    //init skyboxTex
    , skyboxTexture(celestialFaces)
    , celestialSkyboxTexture(celestialFaces)
    //init hdrTex and hdr cubemap
    , hdrTextureMap(hdrSamplePath)
    , hdrCubeMap(RENDER_HDR_CUBE_MAP)
    , hdrIrradianceMap(RENDER_HDR_IRRIDIANCE_MAP)
    , hdrPrefilteredMap(RENDER_PRE_FILTER_HDR_MAP)
    , BRDFTexture(BRDF_2D_TEXTURE)

    // SSAO Buffer Generation
    , m_diffuseTex(RESOURCE_ROOT "resources/texture/2k_earth_map_diffuse.jpg")
    , m_specularTex(RESOURCE_ROOT "resources/texture/2k_earth_map_specular.png")
    , gPos(SSAO_GBUFFER_POS)
    , gNor(SSAO_GBUFFER_NOR)
    , gCol(SSAO_GBUFFER_COL)
    , ssaoColorBuff(SSAO_COLOR_BUFF)
    , ssaoColorBlurBuff(SSAO_COLOR_BLUR)
    , ssaoNoiseTex()
    , trackball{ &m_window, glm::radians(50.0f) }
    
    // inits for hierarchical rendering
    , celestialBodies { CelestialBody::Sun(), CelestialBody::Earth(), CelestialBody::Moon() }
    , sun_light { }
{
    // lights must be initialized here since light is still struct not class
    lights.push_back(
        { glm::vec3(1, 3, -2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    lights.push_back(
        { glm::vec3(-1, 3, 2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    initCelestialTextures();

    // init normal material
    m_Material.kd = glm::vec3{ 0.5f, 0.5f, 1.0f };
    m_Material.ks = glm::vec3{ 0.1f, 1.0f, 0.1f };
    m_Material.shininess = 3.0f;

    selectedCamera = &cameras.at(curCameraIndex);
    selectedLight = &lights.at(curLightIndex);

    glm::vec3 look_at = { 0.0, 1.0, -1.0 };
    glm::vec3 rotations = { 0.2, 0.0, 0.0 };

    float dist = 1.0;

    m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS)
            onKeyPressed(key, mods);
        else if (action == GLFW_RELEASE)
            onKeyReleased(key, mods);
        });

    // m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
    m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
        if (action == GLFW_PRESS)
            onMouseClicked(button, mods);
        else if (action == GLFW_RELEASE)
            onMouseReleased(button, mods);
    });

    // generate env maps
    initPBRTexures();
    generateSkyBox();
    generateHdrMap();

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    glm::ivec2 windowSizes = m_window.getWindowSize();
    glViewport(0, 0, windowSizes.x, windowSizes.y);

    initMaterialTexture();

    // ===  Create All Shader ===
    try {
        ShaderBuilder debugShader;
        debugShader.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        debugShader.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/debug_frag.glsl");
        m_debugShader = debugShader.build();

        ShaderBuilder defaultBuilder;
        defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
        m_defaultShader = defaultBuilder.build();

        ShaderBuilder shadowBuilder;
        shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
        shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
        m_shadowShader = shadowBuilder.build();

        ShaderBuilder multiLightBuilder;
        multiLightBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        multiLightBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/multi_light_shader_frag.glsl");
        m_multiLightShader = multiLightBuilder.build();

        ShaderBuilder PbrBuilder;
        PbrBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        PbrBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/PBR_Shader_frag.glsl");
        m_pbrShader = PbrBuilder.build();

        // set up light Shader
        ShaderBuilder lightShaderBuilder;
        lightShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/lights/light_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lights/light_frag.glsl");
        m_lightShader = lightShaderBuilder.build();

        ShaderBuilder skyBoxBuilder;
        skyBoxBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/skybox_frag.glsl");
        m_skyBoxShader = skyBoxBuilder.build();

        ShaderBuilder hdrSkyBoxBuilder;
        hdrSkyBoxBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/skybox_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/hdr_skybox_frag.glsl");
        m_hdrSkyBoxShader = hdrSkyBoxBuilder.build();

        ShaderBuilder borderShader;
        borderShader.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/border_vert.glsl");
        borderShader.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/border_frag.glsl");
        m_borderShader = borderShader.build();

        ShaderBuilder pointShader;
        pointShader.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/border_vert.glsl");
        pointShader.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/border_frag.glsl");
        m_pointShader = pointShader.build();

        ShaderBuilder postProcessShader;
        postProcessShader.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/postProcess_vert.glsl");
        postProcessShader.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/postProcess_frag.glsl");
        m_postProcessShader = postProcessShader.build();

        /*
        ShaderBuilder ssaoBuilder;
        ssaoBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/lights/light_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lights/light_frag.glsl");
        m_shaderSSAO = ssaoBuilder.build();

        ShaderBuilder ssaoBlurBuilder;
        ssaoBlurBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/lights/light_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lights/light_frag.glsl");
        m_shaderSSAOBlur = ssaoBuilder.build();
        */

        initPostProcess();
        applyNormalTexture();
    }
    catch (ShaderLoadingException e) {
        std::cerr << e.what() << std::endl;
    }

    // === Create Shadow Texture ===
    try {
        //m_shadowTex = ShadowTexture(SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
        m_shadowTex = ShadowTexture(WINDOW_WIDTH, WINDOW_HEIGHT);
    }
    catch (shadowLoadingException e) {
        std::cerr << e.what() << std::endl;
    }
}

void Application::applyNormalTexture()
{
    // Normal texture image
    int width, height, sourceNumChannels;
    stbi_uc* pixels = stbi_load(RESOURCE_ROOT "resources/normal_mapping/checkerboard_normal.png", &width, &height, &sourceNumChannels, STBI_rgb);
    // stbi_uc* pixels = stbi_load(RESOURCE_ROOT "resources/normal_mapping/brickwall_normal.png", &width, &height, &sourceNumChannels, STBI_rgb);

    // Normal texture
    glGenTextures(1, &normalTex);
    glBindTexture(GL_TEXTURE_2D, normalTex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    stbi_image_free(pixels);
}

void Application::update() {
    while (!m_window.shouldClose()) {
        m_window.updateInput();
        windowSizes = m_window.getWindowSize(); 

        m_materialChangedByUser = false;

        this->imgui();
        selectedCamera->updateInput();
        m_viewMatrix = selectedCamera->viewMatrix();

        if (usePostProcess) {
            // 绑定自定义的帧缓冲对象
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostProcess);
        }
        else {
            // 绑定默认帧缓冲对象（屏幕）
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearDepth(1.0);

        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        // Camera, trackball and orientation matrices
        const glm::vec3 cameraPos = selectedCamera->cameraPos();
        const glm::mat4 model{ 1.0f };

        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;

        glm::mat4 lightViewMatrix = glm::lookAt(selectedLight->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightMVP = m_projectionMatrix * lightViewMatrix;
        
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        // Either render the (simplified) solar system, or another scene.
        if (showSolarSystem)
        {
            // Render the solar system
            renderSolarSystem();
        }
        
        if (!showSolarSystem) {
            // Render the other scene

            #pragma region Mesh render loop
            // Mesh render loop
            for (GPUMesh& mesh : m_meshes) {

                //shadow maps generates the shadows
                #pragma region shadow Map Genereates
                    if (false)
                    {
                        mesh.drawShadowMap(m_shadowShader, lightMVP, m_shadowTex.getFramebuffer(), SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
                    }
                #pragma endregion

                // set new Material every time it is updated
                GLuint newUBOMaterial;
                GPUMaterial gpuMat = GPUMaterial(m_Material);
                genUboBufferObj(gpuMat, newUBOMaterial);
                mesh.setUBOMaterial(newUBOMaterial);

                // generate UBO for shadowSetting
                GLuint shadowSettingUbo;
                genUboBufferObj(shadowSettings, shadowSettingUbo);

                //// Draw mesh into depth buffer but disable color writes.
                //glDepthMask(GL_TRUE);
                //glDepthFunc(GL_LEQUAL);
                //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

                //m_debugShader.bind();
                //glUniformMatrix4fv(m_debugShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                //glUniformMatrix3fv(m_debugShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                //glUniformMatrix4fv(m_debugShader.getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                //mesh.drawBasic(m_debugShader);

                //// Draw the mesh again for each light / shading model.
                //glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Enable color writes.
                //glDepthMask(GL_FALSE); // Disable depth writes.
                //glDepthFunc(GL_EQUAL); // Only draw a pixel if it's depth matches the value stored in the depth buffer.
                //glEnable(GL_BLEND); // Enable blending.
                //glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                // ssao will set a completely different render pipeline
                if (ssaoEnabled) 
                {
                    genDeferredRenderBuffer(defRenderBufferGenerated);

                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                    m_selShader = &m_shaderGeometryPass;

                    // GeoMetryPass
                    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
                    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glDisable(GL_BLEND);

                    m_selShader->bind();

                    glUniform1i(m_selShader->getUniformLocation("ignoreLightDirection"), GL_FALSE);
                    glUniform1f(m_selShader->getUniformLocation("sunlightStrength"), 1.0f);
                    glUniformMatrix4fv(m_selShader->getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(view));
                    glUniformMatrix4fv(m_selShader->getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

                    for (GLuint i = 0; i < objectPositions.size(); ++i) {

                        auto modelMatrix = glm::translate(model, objectPositions[i]);
                        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.5f));
                        glUniformMatrix4fv(m_selShader->getUniformLocation("model"), 1, GL_FALSE, glm::value_ptr(modelMatrix));

                        auto normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
                        glUniformMatrix3fv(m_selShader->getUniformLocation("normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrix));

                        m_diffuseTex.bind(GL_TEXTURE10);
                        glUniform1i(m_selShader->getUniformLocation("texture_diffuse"), 10);

                        m_specularTex.bind(GL_TEXTURE11);
                        glUniform1i(m_selShader->getUniformLocation("texture_specular"), 11);

                        mesh.drawBasic(*m_selShader);
                    }
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);

                    
                    // Lighting Pass
                    m_selShader = &m_shaderLightingPass;
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    m_selShader->bind();

                    gPos.bind(GL_TEXTURE0);
                    gNor.bind(GL_TEXTURE1);
                    gCol.bind(GL_TEXTURE2);

                    glUniform1i(m_selShader->getUniformLocation("gPosition"), true ? 0 : -1);
                    glUniform1i(m_selShader->getUniformLocation("gNormal"), true ? 1 : -1);
                    glUniform1i(m_selShader->getUniformLocation("gAlbedoSpec"), true ? 2 : -1);

                    for (auto& light : lights) {
                        light.radius = calculateLightRadius(light);
                    }

                    genUboBufferObj(lights, lightUBO, MAX_LIGHT_CNT);  // pass the light into the buffer

                    m_selShader->bindUniformBlock("lights", 3, lightUBO);

                    glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos));

                    renderQuad(quadVAO, quadVBO, quadVertices, 20);

                    // copy depth buffer to default framebuffer's depth buffer
                    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
                    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); 

                    glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    
                    // render Light at the end 
                    m_selShader = &m_deferredLightShader;
                    m_selShader->bind();


                    for (const Light& light : lights) {

                        auto modelMatrix = glm::translate(model, light.position);
                        modelMatrix = glm::scale(modelMatrix, glm::vec3(0.125f));
                        glUniformMatrix4fv(m_selShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(projection * view * modelMatrix));
                        glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
                        glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(light.position));

                        renderHDRCubeMap(cubeVAO, cubeVBO, hdrMapVertices, 288);
                    }

                    defRenderLightGen = true;

                    /*
                    m_deferredDebugShader.bind();
                    gNor.bind(GL_TEXTURE0);
                    glUniform1i(m_deferredDebugShader.getUniformLocation("fboDebug"), true ? 0 : -1);

                    renderQuad(quadVAO, quadVBO, quadVertices, 20);
                    */
                }

                else 
                {
                    if (defRenderLightGen) {
                        lights.clear();

                        // lights must be initialized here since light is still struct not class
                        lights.push_back(
                            { glm::vec3(1, 3, -2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
                        );

                        lights.push_back(
                            { glm::vec3(-1, 3, 2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
                        );

                        defRenderLightGen = false;
                    }

                    GLuint PbrUBO;

                    if (multiLightShadingEnabled) {
                        m_selShader = usePbrShading ? &m_pbrShader : &m_multiLightShader;
                    }
                    else {
                        m_selShader = &m_defaultShader;
                    }

                    m_selShader->bind();

                    // Set up matrices and view position
                    glUniformMatrix4fv(m_selShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
                    glUniformMatrix3fv(m_selShader->getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
                    glUniformMatrix4fv(m_selShader->getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
                    glUniformMatrix4fv(m_selShader->getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
                    glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos));
                    glUniform1i(m_selShader->getUniformLocation("ignoreLightDirection"), GL_FALSE);
                    glUniform1f(m_selShader->getUniformLocation("sunlightStrength"), 1.0f);

                    // Texture and material settings
                    bool hasTexCoords = mesh.hasTextureCoords();
                    m_texture.bind(hasTexCoords ? GL_TEXTURE0 : 0);

                    // PBR Material Texture
                    auto bindSlot = GL_TEXTURE10;
                    for (auto& pbrTexture : m_pbrTextures) {
                        pbrTexture.bind(bindSlot);
                        bindSlot++;
                    }

                    hasTexCoords = hasTexCoords && textureEnabled;
                    glUniform1i(m_selShader->getUniformLocation("hasTexCoords"), hasTexCoords);
                    glUniform1i(m_selShader->getUniformLocation("colorMap"), hasTexCoords ? 0 : -1);
                    glUniform1i(m_selShader->getUniformLocation("useMaterial"),m_useMaterial);

                    // Pass in shadow settings as UBO
                    m_selShader->bindUniformBlock("shadowSetting", 2, shadowSettingUbo);

                    glBindVertexArray(mesh.getVao());
                    m_shadowTex.bind(GL_TEXTURE1);
                    glUniform1i(m_selShader->getUniformLocation("texShadow"), 1);
                    glBindVertexArray(0);

                    //// Restore default depth test settings and disable blending.
                    //glDepthFunc(GL_LEQUAL);
                    //glDepthMask(GL_TRUE);
                    //glDisable(GL_BLEND);

                    // pass in env map
                    glBindVertexArray(mesh.getVao());
                    selectedSkybox->bind(GL_TEXTURE20);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    glUniform1i(m_selShader->getUniformLocation("SkyBox"), 20);
                    glUniform1i(m_selShader->getUniformLocation("useEnvMap"), envMapEnabled);
                    glBindVertexArray(0);

                    if (useNormalMapping) {
                        glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_TRUE);
                        glActiveTexture(GL_TEXTURE3);
                        glBindTexture(GL_TEXTURE_2D, normalTex);
                        glUniform1i(m_selShader->getUniformLocation("normalTex"), 3);
                    } else {
                        glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_FALSE);
                    }


                    // Generate UBOs and draw
                    drawMultiLightShader(mesh, multiLightShadingEnabled);
                    
                    int lightsCnt = static_cast<int>(lights.size());
                    glBindVertexArray(mesh.getVao());
                    m_lightShader.bind();
                    {
                        const glm::vec4 screenPos = mvpMatrix * glm::vec4(selectedLight->position, 1.0f);
                        const glm::vec3 color = selectedLight->color;

                        glPointSize(40.0f);
                        glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
                        glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(color));
                        glDrawArrays(GL_POINTS, 0, 1);
                    }

                    for (const Light& light : lights) {
                        const glm::vec4 screenPos = mvpMatrix * glm::vec4(light.position, 1.0f);

                        glPointSize(10.0f);
                        glUniform4fv(m_lightShader.getUniformLocation("pos"), 1, glm::value_ptr(screenPos));
                        glUniform3fv(m_lightShader.getUniformLocation("color"), 1, glm::value_ptr(light.color));
                        glDrawArrays(GL_POINTS, 0, 1);
                    }
                    glBindVertexArray(0);

                    mesh.drawBasic(m_lightShader);

                    if (render_minimap)
                    {
                        renderMiniMapItem(m_modelMatrix);
                    }
                }
            }
            #pragma endregion
        }

        //Draw Env Map
        drawEnvMap(envMapEnabled, hdrMapEnabled);//MARK

        if (render_minimap)
        {
            renderMiniMap();
        }
        
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        if (usePostProcess) {
            // 绑定默认帧缓冲对象，将结果绘制到屏幕
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            runPostProcess();
            glFinish();
        }

        /*glDisable(GL_DEPTH_TEST);
        m_brdfShader.bind();
        renderQuad(quadVAO,quadVBO,quadVertices,20);
        glEnable(GL_DEPTH_TEST);*/

        m_window.swapBuffers();
    }

    glDeleteTextures(1, &normalTex);
}





// ==== Below are helper functions that are used during the update loop ==== //


/**
 * Generates a VAO and VBO for a skybox.
 */
void Application::generateSkyBox()
{
    selectedSkybox = &skyboxTexture;

    // === Create SkyBox Vertices ===

    glEnable(GL_DEPTH_TEST);

    try {
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);

        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

        // Set vertex attribute pointers
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        glBindVertexArray(0);
    }

    catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
}

/**
 * Initializes the HDR map parameters and images.
 */
void Application::generateHdrMap()
{
    // === Create HDR FrameBuffer ===
    glDepthFunc(GL_LEQUAL);

    try {
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 1024, 1024);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        // initialize hdrTexMap should be here

        // init cube to render hdr map

        // This Shader convert HDR Texture we got and put it onto the hdr_cube_map
        ShaderBuilder hdrToCubeMapShaderBuilder;
        hdrToCubeMapShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/hdr_to_cube_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/hdr_to_cube_frag.glsl");
        m_hdrToCubeShader = hdrToCubeMapShaderBuilder.build();

        m_hdrToCubeShader.bind();
        glUniformMatrix4fv(m_hdrToCubeShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));

        hdrTextureMap.bind(GL_TEXTURE0);
        glUniform1i(m_hdrToCubeShader.getUniformLocation("equirectangularMap"), 0);

        //glViewport(0, 0, 1024, 1024);//VIEWPORT
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (GLuint i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(m_hdrToCubeShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, hdrCubeMap.getTextureRef(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderHDRCubeMap(cubeVAO, cubeVBO, hdrMapVertices, 288);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //// let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
        //glBindFramebuffer(GL_TEXTURE_CUBE_MAP, hdrCubeMap.getTextureRef());
        //glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        //glBindFramebuffer(GL_TEXTURE_CUBE_MAP, 0);

        // integral convolution to create hdr irridiance map

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);  // rebuild buffer for irridiance map

        ShaderBuilder hdrToIrradianceShaderBuilder;
        hdrToIrradianceShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/hdr_to_cube_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/hdr_cube_to_irradiance_frag.glsl");
        m_hdrToIrradianceShader = hdrToIrradianceShaderBuilder.build();

        m_hdrToIrradianceShader.bind();
        glUniformMatrix4fv(m_hdrToCubeShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));

        hdrCubeMap.bind(GL_TEXTURE0);
        glUniform1i(m_hdrToIrradianceShader.getUniformLocation("environmentMap"), 0);

        glViewport(0, 0, 32, 32);
        glViewport(0, 0, 32, 32);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        for (GLuint i = 0; i < 6; ++i)
        {
            glUniformMatrix4fv(m_hdrToIrradianceShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, hdrIrradianceMap.getTextureRef(), 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderHDRCubeMap(cubeVAO, cubeVBO, hdrMapVertices, 288);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // generate hdr prefiltered Map
        ShaderBuilder hdrPrefilteredShaderBuilder;
        hdrPrefilteredShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/hdr_to_cube_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/hdr_cube_to_prefilter_frag.glsl");
        m_hdrPrefilterShader = hdrPrefilteredShaderBuilder.build();

        m_hdrPrefilterShader.bind();

        glUniformMatrix4fv(m_hdrPrefilterShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(captureProjection));

        hdrCubeMap.bind(GL_TEXTURE0);
        glUniform1i(m_hdrPrefilterShader.getUniformLocation("environmentMap"), 0);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        GLuint maxMipLevels = 5;
        for (GLuint mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            GLuint mipWidth = static_cast<GLuint>(128 * std::pow(0.5, mip));
            GLuint mipHeight = static_cast<GLuint>(128 * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            glUniform1i(m_hdrPrefilterShader.getUniformLocation("roughness"), roughness);

            for (GLuint i = 0; i < 6; ++i)
            {
                glUniformMatrix4fv(m_hdrPrefilterShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(captureViews[i]));
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, hdrPrefilteredMap.getTextureRef(), mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                renderHDRCubeMap(cubeVAO, cubeVBO, hdrMapVertices, 288);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        // Gen BRDF Texture
        ShaderBuilder brdfShaderBuilder;
        brdfShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/texture_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/brdf_frag.glsl");
        m_brdfShader = brdfShaderBuilder.build();

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, BRDFTexture.getTextureRef(), 0);

        glViewport(0, 0, 512, 512);

        m_brdfShader.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderQuad(quadVAO, quadVBO, quadVertices, 20);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }
}

/**
 * generate deferred rendering buffer before the pipeline begin, this should be only excuted once
 */
void Application::genDeferredRenderBuffer(bool& defRenderBufferGenerated)
{
    // deferred rendering pipeline generation
    if (!defRenderBufferGenerated) {
        try
        {
            ShaderBuilder shaderGeometryPassBuilder;
            shaderGeometryPassBuilder
                .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/deferred_render/gGeo_shader_vert.glsl")
                .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/deferred_render/gGeo_shader_frag.glsl");
            m_shaderGeometryPass = shaderGeometryPassBuilder.build();

            ShaderBuilder shaderLightingPassBuilder;
            shaderLightingPassBuilder
                .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/deferred_render/deferred_gLight_vert.glsl")
                .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/deferred_render/deferred_gLight_frag.glsl");
            m_shaderLightingPass = shaderLightingPassBuilder.build();


            ShaderBuilder deferredLightShaderBuilder;
            deferredLightShaderBuilder
                .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/lights/deferred_light_vert.glsl")
                .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/lights/light_frag.glsl");
            m_deferredLightShader = deferredLightShaderBuilder.build();

            ShaderBuilder deferredFBOdebugShaderBuilder;
            deferredFBOdebugShaderBuilder
                .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/deferred_render/deferred_fbo_debug_vert.glsl")
                .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/deferred_render/deferred_fbo_debug_frag.glsl");
            m_deferredDebugShader = deferredFBOdebugShaderBuilder.build();

            glGenFramebuffers(1, &gBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

            if (gPos.gBufferCode == SSAO_GBUFFER_POS)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPos.getTextureRef(), 0);
            if (gNor.gBufferCode == SSAO_GBUFFER_NOR)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNor.getTextureRef(), 0);
            if (gCol.gBufferCode == SSAO_GBUFFER_COL)
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gCol.getTextureRef(), 0);

            GLuint attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
            glDrawBuffers(3, attachments);

            // render buffer
            glGenRenderbuffers(1, &renderDepth);
            glBindRenderbuffer(GL_RENDERBUFFER, renderDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderDepth);

            // finally check if framebuffer is complete
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "Framebuffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            //genSSAOFrameBuffer();

            lights.clear();
            for (GLint i = 0; i < MAX_LIGHT_CNT; ++i) {

                float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
                float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
                float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
                // also calculate random color
                float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
                float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)
                float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5); // between 0.5 and 1.)

                auto lightPos = glm::vec3(xPos, yPos, zPos);
                lights.push_back(
                    { lightPos,glm::vec3(rColor, gColor, bColor),-lightPos,false,false }
                );
            }
        }
        catch (std::runtime_error e)
        {
            std::cerr << e.what() << std::endl;
        }

        defRenderBufferGenerated = true;
    }
}

void Application::deferredRenderPipeLine()
{

}

// unused
void Application::genSSAOFrameBuffer()
{
    glGenFramebuffers(1, &ssaoFBO);
    glGenFramebuffers(1, &ssaoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);

    if (ssaoColorBuff.gBufferCode == SSAO_COLOR_BUFF)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBuff.getTextureRef(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoBlurFBO);
    if (ssaoColorBlurBuff.gBufferCode == SSAO_COLOR_BLUR)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoColorBlurBuff.getTextureRef(), 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "SSAO Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;

    // generate sample kernel
    std::vector<glm::vec3> ssaoKernel = generateSSAOKernel();

    // generate noise
    std::vector<glm::vec3> ssaoNoise = generateSSAONoise();

    ssaoNoiseTex = std::move(ssaoBufferTex(SSAO_NOISE_TEX, ssaoNoise));
}

/**
 * ImGui menu.
 */
void Application::imgui() {
    ImGui::Begin("Assignment 2 Demo");

    ImGui::Separator();
    ImGui::Text("Minimap");
    ImGui::Checkbox("Show Minimap", &render_minimap);

    ImGui::Separator();
    ImGui::Text("Hierarchical Rendering");
    ImGui::Checkbox("Show Solar System", &showSolarSystem);
    ImGui::SliderFloat("Sunlight Strength", &sunlight_strength, 0.0f, 5.0f);
    ImGui::Checkbox("Move Celestial Bodies", &moveCelestialBodies);

    if (showSolarSystem) { selectedSkybox = &celestialSkyboxTexture; }
    else { selectedSkybox = &skyboxTexture; }

    ImGui::Separator();
    ImGui::Text("Displacement mapping");
    if (ImGui::Checkbox("Enable Normal Mapping", &useNormalMapping))
    {
        useParallaxMapping = false;
    }
    if (ImGui::Checkbox("Enable Parallax Mapping", &useParallaxMapping))
    {
        useNormalMapping = false;
    }
    ImGui::Text("Normal and parallax mapping are only applied to applicable meshes.");

    ImGui::Separator();
    ImGui::Text("Environment Map parameters");
    ImGui::Checkbox("Enable Environment Map", &envMapEnabled); // only for single light now
    ImGui::Checkbox("Enable HDR Environment", &hdrMapEnabled);

    ImGui::Separator();

    ImGui::Checkbox("Enable Material for model", &m_useMaterial);

    ImGui::Combo("Material Setting", &curMaterialIndex, materialModelNames.data(), static_cast<size_t>(MaterialModel::CNT));
    ImGui::Text("Material parameters");

    if (static_cast<MaterialModel>(curMaterialIndex) == MaterialModel::NORMAL) {
        ImGui::Separator();
        // Color pickers for Kd and Ks
        ImGui::ColorEdit3("Kd", &m_Material.kd[0]);
        ImGui::ColorEdit3("Ks", &m_Material.ks[0]);

        ImGui::SliderFloat("Shininess", &m_Material.shininess, 0.0f, 100.f);
        /* ImGui::SliderInt("Toon Discretization", &m_Material.toonDiscretize, 1, 10);
         ImGui::SliderFloat("Toon Specular Threshold", &m_Material.toonSpecularThreshold, 0.0f, 1.0f);*/

        this->usePbrShading = false;

        ImGui::Separator();
        ImGui::Text("Texture parameters");

        ImGui::Checkbox("Enable Texture for model", &textureEnabled);

        std::strncpy(file_path_buffer, texturePath.c_str(), sizeof(file_path_buffer) - 1);
        ImGui::InputText("Texture Path:", file_path_buffer, sizeof(file_path_buffer));
        if (ImGui::Button("Regenerate Texture")) {
            try {
                texturePath = file_path_buffer;
                m_texture = Texture(RESOURCE_ROOT + texturePath);
            }
            catch (textureLoadingException e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
    else if (static_cast<MaterialModel>(curMaterialIndex) == MaterialModel::PBR) {
        ImGui::Separator();
        // Color pickers for Kd and Ks
        ImGui::ColorEdit3("Albedo", &m_PbrMaterial.albedo[0]);
        ImGui::SliderFloat("Metallic", &m_PbrMaterial.metallic,0.0f,1.0f);
        ImGui::SliderFloat("Roughness", &m_PbrMaterial.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &m_PbrMaterial.ao, 0.0f, 1.0f);

        this->usePbrShading = true;
        this->multiLightShadingEnabled = true;

        ImGui::Separator();
        ImGui::Text("PBR Texture parameters");

        ImGui::Checkbox("Enable Texture for model", &textureEnabled);

        std::strncpy(file_path_buffer, hdrSamplePath.string().c_str(), sizeof(file_path_buffer) - 1);
        ImGui::InputText("Texture Path:", file_path_buffer, sizeof(file_path_buffer));
    }

    ImGui::Separator();
    
    std::vector<std::string> itemStrings;
    std::vector<const char*> itemCStrings;

    int tempSelectedItem = static_cast<int>(curCameraIndex);

    if (ImGui::CollapsingHeader("Cameras")) {
        ImGui::Text("Cameras:");
        for (size_t i = 0; i < cameras.size(); i++) {
            itemStrings.push_back("Camera " + std::to_string(i));
        }

        for (const auto& string : itemStrings) {
            itemCStrings.push_back(string.c_str());
        }

        if (ImGui::ListBox("Cameras", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
            selectedCamera->setUserInteraction(false);
            curCameraIndex = static_cast<size_t>(tempSelectedItem);
            selectedCamera = &cameras.at(curCameraIndex);
            selectedCamera->setUserInteraction(true);
        }

        selectedCamera = &cameras[curCameraIndex];
        ImGui::Text("Selected Camera Index: %d", curCameraIndex);
        ImGui::Checkbox("Use Bezier", &selectedCamera->m_useBezier);
        ImGui::Checkbox("Use Constant Speed", &selectedCamera->m_bezierConstantSpeed);
        ImGui::DragFloat3("P0", &selectedCamera->P0.x);
        ImGui::DragFloat3("P1", &selectedCamera->P1.x);
        ImGui::DragFloat3("P2", &selectedCamera->P2.x);
        ImGui::DragFloat3("P3", &selectedCamera->P3.x);

        // Button for clearing Camera
        if (ImGui::Button("Reset Cameras")) {
            //resetObjList(cameras,defaultLight);
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Lights")) {
        ImGui::Text("Lights");
        ImGui::Checkbox("MultiLightShading", &multiLightShadingEnabled);

        itemStrings.clear();
        itemCStrings.clear();
        tempSelectedItem = static_cast<int>(curLightIndex);

        for (size_t i = 0; i < lights.size(); i++) {
            auto string = "Light " + std::to_string(i);
            itemStrings.push_back(string);
        }

        for (const auto& string : itemStrings) {
            itemCStrings.push_back(string.c_str());
        }

        if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
            curLightIndex = static_cast<size_t>(tempSelectedItem);
        }

        selectedLight = &lights[curLightIndex];
        ImGui::Text("Selected Light Index: %d", curLightIndex);

        ImGui::ColorEdit3("Light Color", &selectedLight->color[0]);

        ImGui::InputFloat3("Position", &selectedLight->position[0]);

        if (ImGui::Button("Add Lights")) {
            lights.push_back(Light{ glm::vec3(1, 3, -2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ });
        }

        if (ImGui::Button("Remove Lights")) {
            lights.erase(lights.begin() + curLightIndex);
            if (curLightIndex >= lights.size()) {
                curLightIndex = lights.size() - 1;
            }
        }

        // Button for clearing lights
        if (ImGui::Button("Reset Lights")) {
            //resetObjList(lights,defaultLight);
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Enable Alternative rendering process")) {
        ImGui::Checkbox("Switch to Deferred Rendering Pipeline", &ssaoEnabled);
        ImGui::Checkbox("usePostProcess", &usePostProcess);
    }

    ImGui::End();
}

// === Register input === //
void Application::onKeyPressed(int key, int mods) {
    std::cout << "Key pressed: " << key << std::endl;
}

void Application::onKeyReleased(int key, int mods) {
    std::cout << "Key released: " << key << std::endl;
}

void Application::onMouseMove(const glm::dvec2& cursorPos) {
    std::cout << "Mouse at position: " << cursorPos.x << " " << cursorPos.y << std::endl;
}

void Application::onMouseClicked(int button, int mods) {
    std::cout << "Pressed mouse button: " << button << std::endl;
}

void Application::onMouseReleased(int button, int mods) {
    std::cout << "Released mouse button: " << button << std::endl;
}

/**
 * Renders all meshes in the minimap, given a model matrix.
 * This should be called for each item that is to be rendered.
 * It does render all loaded meshes, so this works best if
 * there is only a single mesh (e.g. one sphere) that is rendered
 * multiple times in different ways.
 */
void Application::renderMiniMapItem(glm::mat4 modelMatrix)
{
    GLint previousVBO;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousVBO);

    glViewport(800, 800, 200, 200); // Make it to up-right
    glDisable(GL_DEPTH);

    // 使用小地图的视图矩阵和投影矩阵渲染场景
    m_selShader->bind();
    const glm::mat4 mvpMatrix = minimap.projectionMatrix() * minimap.viewMatrix() * modelMatrix;
    const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));
    glUniformMatrix4fv(m_selShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    glUniformMatrix4fv(m_selShader->getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
    glUniformMatrix4fv(m_selShader->getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(minimap.cameraPos()));
    // glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(glm::vec3(0.0f, 80.0f, 0.0f)));

    // 渲染小地图内容
    for (GPUMesh& mesh : m_meshes) {
        if (usePbrShading) {
            mesh.drawPBR(*m_selShader, PbrUBO, lightUBO);
        }
        else {
            mesh.draw(*m_selShader);
        }
    }

    // 恢复主视口
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH);
    glBindBuffer(GL_ARRAY_BUFFER, previousVBO);
}

/**
 * Renders the minimap borders and "player camera" location.
 */
void Application::renderMiniMap() {
    GLint previousVBO;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousVBO);

    glViewport(800, 800, 200, 200); // Make it to up-right
    glDisable(GL_DEPTH);

    const glm::mat4 minimapVP = minimap.projectionMatrix() * minimap.viewMatrix() * m_modelMatrix;
    glm::vec4 cameraPosInMinimap = minimapVP * glm::vec4(selectedCamera->cameraPos(), 1.0f);

    cameraPosInMinimap /= cameraPosInMinimap.w; // 透视除法，得到标准化设备坐标
    
    drawCameraPositionOnMinimap(cameraPosInMinimap);

    // 恢复主视口
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    drawMiniMapBorder();
    glEnable(GL_DEPTH);
    glBindBuffer(GL_ARRAY_BUFFER, previousVBO);
}

/**
 * Renders the minimap border.
 */
void Application::drawMiniMapBorder() {
    // Disable Depth Test to make sure our Border will not be covered
    glDisable(GL_DEPTH_TEST);

    // Use Polygon Mode to draw Map Border
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Define Map Border 
    float borderLeft = 800.0f / 1024.0f * 2.0f - 1.0f;
    float borderRight = (800.0f + 200.0f) / 1024.0f * 2.0f - 1.0f;
    float borderBottom = 800.0f / 1024.0f * 2.0f - 1.0f;
    float borderTop = (800.0f + 200.0f) / 1024.0f * 2.0f - 1.0f;

    // Set Vertex data
    float borderVertices[] = {
        borderLeft,  borderBottom, 0.0f,  
        borderRight, borderBottom, 0.0f,  
        borderRight, borderTop,    0.0f,  
        borderLeft,  borderTop,    0.0f   
    };

    // Use Border Shader to set color
    m_borderShader.bind();
    glUniform3f(m_borderShader.getUniformLocation("color"), 1.0f, 1.0f, 0.0f); // Set Color to Yellow

    GLuint borderVBO, borderVAO;
    glGenVertexArrays(1, &borderVAO);
    glGenBuffers(1, &borderVBO);
    glBindVertexArray(borderVAO);

    glBindBuffer(GL_ARRAY_BUFFER, borderVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(borderVertices), borderVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Draw rectangle
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    // Clean
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &borderVBO);
    glDeleteVertexArrays(1, &borderVAO);

    // Reset
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
}

/**
 * Renders the position of the "player camera" on the minimap as a block.
 */
void Application::drawCameraPositionOnMinimap(const glm::vec4& cameraPosInMinimap) {
    glDisable(GL_DEPTH_TEST); // Make sure our dot will not be covered

    float x = cameraPosInMinimap.x;
    float y = cameraPosInMinimap.y;

    // Set position data of our main camera
    float pointVertices[] = { x, y, 0.0f };

    GLuint pointVAO, pointVBO;
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);

    glBindVertexArray(pointVAO);

    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointVertices), pointVertices, GL_STATIC_DRAW);

    // Set VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Use Point Shader
    m_pointShader.bind();
    glUniform3f(m_pointShader.getUniformLocation("color"), 1.0f, 0.0f, 0.0f); // Set point to red

    // Draw Dot
    glPointSize(6.0f);
    glDrawArrays(GL_POINTS, 0, 1);

    // Clean
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDeleteBuffers(1, &pointVBO);
    glDeleteVertexArrays(1, &pointVAO);

    glEnable(GL_DEPTH_TEST);
}

/**
 * Initializes the post-processing settings.
 */
void Application::initPostProcess() {
    // 创建帧缓冲对象
    glGenFramebuffers(1, &framebufferPostProcess);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostProcess);

    // 创建颜色纹理附件
    //glActiveTexture(GL_TEXTURE7); // 激活 GL_TEXTURE7
    glGenTextures(1, &texturePostProcess);
    glBindTexture(GL_TEXTURE_2D, texturePostProcess);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturePostProcess, 0);

    // 创建深度渲染缓冲附件
    glGenRenderbuffers(1, &depthbufferPostProcess);
    glBindRenderbuffer(GL_RENDERBUFFER, depthbufferPostProcess);
    //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WINDOW_WIDTH, WINDOW_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbufferPostProcess);

    // 检查帧缓冲对象是否完整
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Framebuffer is not complete!" << std::endl;

    // 解绑帧缓冲对象，防止意外修改
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/**
 * Runs the post-processing pipeline.
 */
void Application::runPostProcess() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 清除默认帧缓冲区，避免重影
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 使用后期处理着色器
    m_postProcessShader.bind();

    // 激活并绑定纹理单元
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, texturePostProcess); // 绑定自定义帧缓冲对象的颜色纹理附件

    // 设置着色器中的采样器
    glUniform1i(m_postProcessShader.getUniformLocation("scene"), 7);

    glDisable(GL_DEPTH_TEST);

    // 渲染全屏四边形，应用后期处理效果
    renderQuad(quadVAO, quadVBO, quadVertices, 18);

    glEnable(GL_DEPTH_TEST);
}

/**
 * Initializes some PBR textures.
 */
void Application::initPBRTexures() {
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/normal.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/albedo.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/metallic.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/roughness.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/ao.png")));
}

/**
 * Initializes the main material texture.
 */
void Application::initMaterialTexture() {
    // === Create Material Texture if its valid path ===
    std::string textureFullPath = std::string(RESOURCE_ROOT) + texturePath;
    std::vector<Mesh> cpuMeshes = loadMesh(RESOURCE_ROOT "resources/sphere.obj"); //"resources/texture/Cerberus_by_Andrew_Maximov/Cerberus_LP.obj

    if (std::filesystem::exists((textureFullPath))) {
        std::shared_ptr texPtr = std::make_shared<Image>(textureFullPath);
        for (auto& mesh : cpuMeshes) {
            mesh.material.kdTexture = texPtr;
        }
    }
    m_meshes = GPUMesh::loadMeshGPU(cpuMeshes);  // load mesh from mesh list so we have more freedom on setting up each mesh
}

/**
 * Draws the skybox or HDR cube map.
 */
void Application::drawEnvMap(bool envMapEnabled, bool hdrMapEnabled) {
    if (envMapEnabled) {
        glm::mat4 projection = m_projectionMatrix;
        glm::mat4 viewModel = glm::mat4(glm::mat3(m_viewMatrix));

        glDepthFunc(GL_LEQUAL);
        if (hdrMapEnabled) {
            m_hdrSkyBoxShader.bind();
            glUniformMatrix4fv(m_hdrSkyBoxShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(viewModel));
            glUniformMatrix4fv(m_hdrSkyBoxShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

            hdrCubeMap.bind(GL_TEXTURE0);
            glUniform1i(m_hdrSkyBoxShader.getUniformLocation("hdrEnvMap"), 0);

            renderHDRCubeMap(cubeVAO, cubeVBO, hdrMapVertices, 288);
        }
        else {

            m_skyBoxShader.bind();

            //glm::mat4 projection = glm::perspective(glm::radians(80.0f), float(WIDTH)/float(HEIGHT), 0.1f, 100.0f);
            glUniformMatrix4fv(m_skyBoxShader.getUniformLocation("view"), 1, GL_FALSE, glm::value_ptr(viewModel));
            glUniformMatrix4fv(m_skyBoxShader.getUniformLocation("projection"), 1, GL_FALSE, glm::value_ptr(projection));

            glBindVertexArray(skyboxVAO);
            selectedSkybox->bind(GL_TEXTURE0);

            glUniform1i(m_skyBoxShader.getUniformLocation("skybox"), 0);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }

        glDepthFunc(GL_LESS);
    }
}

/**
 * Draws using the multi-light shader.
 */
void Application::drawMultiLightShader(GPUMesh& mesh,bool multiLightShadingEnabled) {
    if (multiLightShadingEnabled) {
        genUboBufferObj(lights, lightUBO, MAX_LIGHT_CNT);
        glUniform1i(m_selShader->getUniformLocation("LightCount"), static_cast<GLint>(lights.size()));

        if (usePbrShading) {

            genUboBufferObj(m_PbrMaterial, PbrUBO);

            glUniform1i(m_selShader->getUniformLocation("normalMap"), true ? 10 : -1);
            glUniform1i(m_selShader->getUniformLocation("albedoMap"), true ? 11 : -1);
            glUniform1i(m_selShader->getUniformLocation("metallicMap"), true ? 12 : -1);
            glUniform1i(m_selShader->getUniformLocation("roughnessMap"), true ? 13 : -1);
            glUniform1i(m_selShader->getUniformLocation("aoMap"), true ? 14 : -1);

            hdrIrradianceMap.bind(GL_TEXTURE15);
            glUniform1i(m_selShader->getUniformLocation("irradianceMap"), true ? 15 : -1);

            hdrPrefilteredMap.bind(GL_TEXTURE16);
            glUniform1i(m_selShader->getUniformLocation("prefilteredMap"), true ? 16 : -1);

            BRDFTexture.bind(GL_TEXTURE17);
            glUniform1i(m_selShader->getUniformLocation("brdfLUT"), true ? 17 : -1);

            glUniform1i(m_selShader->getUniformLocation("hdrEnvMapEnabled"), hdrMapEnabled);

            mesh.drawPBR(*m_selShader, PbrUBO, lightUBO);
        }
        else {
            mesh.draw(*m_selShader, lightUBO, multiLightShadingEnabled);
        }
    }
    else {
        genUboBufferObj(*selectedLight, lightUBO); // Pass single Light
        mesh.draw(*m_selShader, lightUBO, multiLightShadingEnabled);
    }
}

/**
 * Renders a simplified the solar system.
 * This should only ever be run if the selected mesh
 * is sphere.obj.
 */
void Application::renderSolarSystem() {
    if (moveCelestialBodies || frame == 0)
    {
        // Update the frame number.
        // This is used for the celestial body movement.
        updateFrameNumber();
    }

    m_selShader = &m_defaultShader;

    GLint previousVBO;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousVBO);

    const glm::vec3 cameraPos = trackball.position();
    const glm::mat4 view = m_viewMatrix;
    const glm::mat4 projection = m_projectionMatrix;
    
    // Loop through each registered celestial body for rendering.
    for (size_t i = 0; i < celestialBodies.size(); ++i)
    {
        // Each body revolves around the previous one,
        // except for the Sun, which is stationary and
        // only revolves around itself.
        //
        // We retrieve the previous body's location in space
        // and base the current body's matrix on that.
        CelestialBody& body = celestialBodies[i];

        glm::mat4 orbitOrigin = glm::mat4(1.0f);
        float orbitRadius = 0.0f;
        if (i >= 1)
        {
            orbitOrigin = celestialBodies[i - 1].getMatrix();
            orbitRadius = celestialBodies[i - 1].getOrbitRadius();
        }

        body.updateBodyPosition(frame, orbitOrigin, orbitRadius);
        const glm::mat4 newMatrix = body.getMatrix();
        const glm::vec3 newPos = glm::vec3(newMatrix[3]);
        const glm::mat4 mvpMatrix = projection * view * newMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(newMatrix));

        const glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(0.0f), glm::vec3(orbitOrigin[3]), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 lightMVP = projection * lightViewMatrix;
        
        // Then we draw the actual mesh representing the celestial body.
        for (GPUMesh& mesh : m_meshes) {
            glBindVertexArray(mesh.getVao());

            m_selShader->bind();

            GLuint newUBOMaterial;
            GPUMaterial gpuMat = GPUMaterial(m_Material);
            gpuMat.kd = body.kd();
            gpuMat.ks = glm::vec3(0.0f);    // No specular reflection in space
            gpuMat.shininess = 0.0f;
            genUboBufferObj(gpuMat, newUBOMaterial);
            mesh.setUBOMaterial(newUBOMaterial);

            GLuint shadowSettingUbo;
            genUboBufferObj(shadowSettings, shadowSettingUbo);

            glUniformMatrix4fv(m_selShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix4fv(m_selShader->getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            glUniformMatrix4fv(m_selShader->getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(newMatrix));
            glUniformMatrix4fv(m_selShader->getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
            glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos));
            glUniform1f(m_selShader->getUniformLocation("sunlightStrength"), sunlight_strength);
            glUniform1i(m_selShader->getUniformLocation("useMaterial"), true);

            // Ignore the position of the light with respect to the object if it is the Sun
            glUniform1i(m_selShader->getUniformLocation("ignoreLightDirection"), i == 0);

            m_shadowTex.bind(GL_TEXTURE1);
            glUniform1i(m_selShader->getUniformLocation("texShadow"), 1);
            glUniform1i(m_selShader->getUniformLocation("useEnvMap"), GL_FALSE);

            // Render planet textures.
            if (textureEnabled)
            {
                Texture* bodyTexture = findCelestialTexture(body.getTexturePath() + ".jpg");
                if (bodyTexture != NULL)
                {
                    bool hasTexCoords = mesh.hasTextureCoords();
                    bodyTexture->bind(hasTexCoords ? GL_TEXTURE0 : 0);
                    glUniform1i(m_selShader->getUniformLocation("hasTexCoords"), hasTexCoords);
                    glUniform1i(m_selShader->getUniformLocation("colorMap"), hasTexCoords ? 0 : -1);
                } else {
                    glUniform1i(m_selShader->getUniformLocation("hasTexCoords"), GL_FALSE);
                }

                // Also fetch the celestial body's normal map, if applicable.
                if (useNormalMapping)
                {
                    Texture* normalTexture = findCelestialTexture(body.getTexturePath() + "_normal.png");
                    if (normalTexture != NULL)
                    {
                        glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_TRUE);
                        normalTexture->bind(GL_TEXTURE3);
                        glUniform1i(m_selShader->getUniformLocation("normalTex"), 3);
                    }
                    else
                    {
                        glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_FALSE);
                    }
                }
                else
                {
                    glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_FALSE);
                }
            }
            else
            {
                glUniform1i(m_selShader->getUniformLocation("hasTexCoords"), GL_FALSE);
                glUniform1i(m_selShader->getUniformLocation("colorMap"), -1);
                glUniform1i(m_selShader->getUniformLocation("useNormalMapping"), GL_FALSE);
            }

            if (usePbrShading) {
                mesh.drawPBR(*m_selShader, PbrUBO, lightUBO);
            }
            else {
                sun_light.position  = (i == 0) ? glm::vec3(0.0f) : glm::vec3(translate(inverse(newMatrix), -1.0f * newPos)[3]);
                sun_light.color     = body.kd();
                genUboBufferObj(sun_light, lightUBO); // Pass single Light
                mesh.draw(*m_selShader, lightUBO, false);
            }

            glBindVertexArray(0);
        }

        // If enabled, render each celestial body inside the minimap.
        if (render_minimap)
        {
            renderMiniMapItem(newMatrix);
        }
    }

    // Restore buffer
    glBindBuffer(GL_ARRAY_BUFFER, previousVBO);
}

/**
 * Initializes the textures and normal maps for the celestial bodies.
 */
void Application::initCelestialTextures()
{
    for (auto& body : celestialBodies)
    {
        std::string path = body.getTexturePath();
        try
        {
            celestialTextures[path + ".jpg"] = Texture(std::string(RESOURCE_ROOT) + path + ".jpg");
            celestialTextures[path + "_normal.png"] = Texture(std::string(RESOURCE_ROOT) + path + "_normal.png");
        }
        catch (textureLoadingException e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

/**
 * Retrieves a celestial body texture (image or normal map)
 * based on the given path.
 * 
 * Path should be e.g. "resources/celestial_bodies/moon".
 */
Texture* Application::findCelestialTexture(std::string celestialTexturePath)
{
    if (celestialTextures.find(celestialTexturePath) != celestialTextures.end())
    {
        return &celestialTextures[celestialTexturePath];
    }
    return NULL;
}

/**
 * Updates the frame number.
 */
void Application::updateFrameNumber()
{
    ++frame;
}
