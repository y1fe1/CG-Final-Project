// Define More header if needed
#include "application.h"

// Constructor
Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , texturePath("resources/texture/wall.jpg")
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
    , skyboxTexture(faces)
    //init hdrTex and hdr cubemap
    , hdrTextureMap(hdrSamplePath)
    , hdrCubeMap(RENDER_HDR_CUBE_MAP)
    , hdrIrradianceMap(RENDER_HDR_IRRIDIANCE_MAP)
    , hdrPrefilteredMap(RENDER_PRE_FILTER_HDR_MAP)
    , BRDFTexture(BRDF_2D_TEXTURE)

    , trackball{ &m_window, glm::radians(50.0f) }
{
    //Camera defaultCamera = Camera(&m_window);

    // lights must be initialized here since light is still struct not class
    lights.push_back(
        { glm::vec3(1, 3, -2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    lights.push_back(
        { glm::vec3(-1, 3, 2), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/normal.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/albedo.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/metallic.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/roughness.png")));
    m_pbrTextures.emplace_back(std::move(Texture(RESOURCE_ROOT "resources/texture/gold_scuffed/ao.png")));

    //lights.push_back(
    //    { glm::vec3(2, 1, 2), glm::vec3(2), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    //);
    
    //lights.push_back(
    //    { glm::vec3(1,1, 3), glm::vec3(2), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    //);

    // init normal material
    m_Material.kd = glm::vec3{ 0.5f, 0.5f, 1.0f };
    m_Material.ks = glm::vec3{ 0.1f, 1.0f, 0.1f };
    m_Material.shininess = 3.0f;

    selectedCamera = &cameras.at(curCameraIndex);
    selectedLight = &lights.at(curLightIndex);

    glm::vec3 look_at = { 0.0, 1.0, -1.0 };
    glm::vec3 rotations = { 0.2, 0.0, 0.0 };
    auto dist = 1.0;
    trackball.setCamera(look_at, rotations, dist);

    m_window.registerKeyCallback([this](int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS)
            onKeyPressed(key, mods);
        else if (action == GLFW_RELEASE)
            onKeyReleased(key, mods);
        });

    m_window.registerMouseMoveCallback(std::bind(&Application::onMouseMove, this, std::placeholders::_1));
    m_window.registerMouseButtonCallback([this](int button, int action, int mods) {
        if (action == GLFW_PRESS)
            onMouseClicked(button, mods);
        else if (action == GLFW_RELEASE)
            onMouseReleased(button, mods);
        });

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

        glViewport(0, 0, 1024, 1024);

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

        renderQuad(quadVAO,quadVBO,quadVertices,20);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    catch (std::runtime_error e) {
        std::cerr << e.what() << std::endl;
    }

    // then before rendering, configure the viewport to the original framebuffer's screen dimensions
    glm::ivec2 windowSizes = m_window.getWindowSize();
    glViewport(0, 0, windowSizes.x, windowSizes.y);



    // === Create Material Texture if its valid path ===
    std::string textureFullPath = std::string(RESOURCE_ROOT) + texturePath;
    std::vector<Mesh> cpuMeshes = loadMesh(RESOURCE_ROOT "resources/sphere.obj"); //"resources/texture/Cerberus_by_Andrew_Maximov/Cerberus_LP.obj

    if (std::filesystem::exists((textureFullPath))) {
        std::shared_ptr texPtr = std::make_shared<Image>(textureFullPath);
        for (auto& mesh : cpuMeshes) {
            mesh.material.kdTexture = texPtr;
        }
    }

    //m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/sphere.obj");
    m_meshes = GPUMesh::loadMeshGPU(cpuMeshes);  // load mesh from mesh list so we have more freedom on setting up each mesh


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
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl");
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
    }
    catch (ShaderLoadingException e) {
        std::cerr << e.what() << std::endl;
    }

    // === Create Shadow Texture ===
    try {
        m_shadowTex = ShadowTexture(SHADOWTEX_WIDTH, SHADOWTEX_HEIGHT);
    }
    catch (shadowLoadingException e) {
        std::cerr << e.what() << std::endl;
    }
}

void Application::update() {
    while (!m_window.shouldClose()) {
        m_window.updateInput();

        m_materialChangedByUser = false;

        this->imgui();

        //selectedCamera->updateInput();
        m_viewMatrix = selectedCamera->viewMatrix();

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

        //glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
 
        const glm::vec3 cameraPos = trackball.position();
        //const glm::vec3 cameraPos = selectedCamera->cameraPos();
        const glm::mat4 model{ 1.0f };

        //const glm::mat4 view = m_viewMatrix;
        const glm::mat4 view = trackball.viewMatrix();
        const glm::mat4 projection = trackball.projectionMatrix();

        glm::mat4 lightMVP;
        GLfloat near_plane = 0.5f, far_plane = 30.0f;
        glm::mat4 mainProjectionMatrix = m_projectionMatrix;//glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, near_plane, far_plane);
        glm::mat4 lightViewMatrix = glm::lookAt(selectedLight->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightMVP = mainProjectionMatrix * lightViewMatrix;

        glm::mat4 mvpMatrix = projection * view * model;
        //const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        // Actualy Mesh Render Loop
        #pragma region Mesh render loop
        for (GPUMesh& mesh : m_meshes) {
            //shadow maps generates the shadows
        #pragma region shadow Map Genereates
                    if (FALSE)
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

            GLuint PbrUBO;

            if (multiLightShadingEnabled) {
                m_selShader = usePbrShading ? &m_pbrShader : &m_multiLightShader;
            }
            else {
                m_selShader = &m_defaultShader;
            }

            //m_selShader = &m_debugShader;
            m_selShader->bind();

            // Set up matrices and view position
            glUniformMatrix4fv(m_selShader->getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix3fv(m_selShader->getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));
            glUniformMatrix4fv(m_selShader->getUniformLocation("modelMatrix"), 1, GL_FALSE, glm::value_ptr(m_modelMatrix));
            glUniformMatrix4fv(m_selShader->getUniformLocation("lightMVP"), 1, GL_FALSE, glm::value_ptr(lightMVP));
            glUniform3fv(m_selShader->getUniformLocation("viewPos"), 1, glm::value_ptr(cameraPos));

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
            skyboxTexture.bind(GL_TEXTURE20);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glUniform1i(m_selShader->getUniformLocation("SkyBox"), 20);
            glUniform1i(m_selShader->getUniformLocation("useEnvMap"), envMapEnabled);
            glBindVertexArray(0);

            // Generate UBOs and draw
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
        }
#pragma endregion

        // Render Enviroment Mapping at the end
        if (envMapEnabled) {

            glm::mat4 viewModel = glm::mat4(glm::mat3(view));

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
                skyboxTexture.bind(GL_TEXTURE0);

                glUniform1i(m_skyBoxShader.getUniformLocation("skybox"), 0);
                glDrawArrays(GL_TRIANGLES, 0, 36);
                glBindVertexArray(0);
            }

            glDepthFunc(GL_LESS);
        }
        
        /*glDisable(GL_DEPTH_TEST);
        m_brdfShader.bind();
        renderQuad(quadVAO,quadVBO,quadVertices,20);
        glEnable(GL_DEPTH_TEST);*/

        m_window.swapBuffers();
    }
}

void Application::imgui() {
    ImGui::Begin("Assignment 2 Demo");

    ImGui::Separator();

    ImGui::Text("Environment Map parameters");
    ImGui::Checkbox("Enable Environment Map", &envMapEnabled); // only for single light now
    ImGui::Checkbox("Enable HDR Environment", &hdrMapEnabled);

    ImGui::Separator();
    ImGui::Text("Texture parameters");

    ImGui::Checkbox("Enable Texture for model", &textureEnabled);

    std::strncpy(file_path_buffer, texturePath.c_str(), sizeof(file_path_buffer) - 1);
    ImGui::InputText("Texture Path:", file_path_buffer, sizeof(file_path_buffer));
    if(ImGui::Button("Regenerate Texture")) {
        try {
            texturePath = file_path_buffer;
            m_texture = Texture(RESOURCE_ROOT + texturePath);
        }
        catch (textureLoadingException e) {
            std::cerr << e.what() << std::endl;
        }
    }

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
    }
    else if (static_cast<MaterialModel>(curMaterialIndex) == MaterialModel::PBR) {
        ImGui::Separator();
        // Color pickers for Kd and Ks
        ImGui::ColorEdit3("Albedo", &m_PbrMaterial.albedo[0]);
        ImGui::SliderFloat("Metallic", &m_PbrMaterial.metallic,0.0f,1.0f);
        ImGui::SliderFloat("Roughness", &m_PbrMaterial.roughness, 0.0f, 1.0f);
        ImGui::SliderFloat("Ambient Occlusion", &m_PbrMaterial.ao, 0.0f, 1.0f);

        this->usePbrShading = true;
    }

    ImGui::Separator();
    ImGui::Text("Shadow modes");
    ImGui::Checkbox("Shadow Enabled", &shadowSettings.shadowEnabled);
    ImGui::Checkbox("PCF Enabled", &shadowSettings.pcfEnabled);


    ImGui::Separator();
    ImGui::Text("Cameras:");

    std::vector<std::string> itemStrings;
    for (size_t i = 0; i < cameras.size(); i++) {
        itemStrings.push_back("Camera " + std::to_string(i));
    }

    std::vector<const char*> itemCStrings;
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }

    int tempSelectedItem = static_cast<int>(curCameraIndex);
    if (ImGui::ListBox("Cameras", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
        selectedCamera->setUserInteraction(false);
        curCameraIndex = static_cast<size_t>(tempSelectedItem);
        selectedCamera = &cameras.at(curCameraIndex);
        selectedCamera->setUserInteraction(true);
    }

    selectedCamera = &cameras[curCameraIndex];
    ImGui::Text("Selected Camera Index: %d", curCameraIndex);

    // Button for clearing Camera
    if (ImGui::Button("Reset Cameras")) {
        //resetObjList(cameras,defaultLight);
    }

    ImGui::Separator();
    ImGui::Text("Lights");
    ImGui::Checkbox("MultiLightShading", &multiLightShadingEnabled);
    itemStrings.clear();
    for (size_t i = 0; i < lights.size(); i++) {
        auto string = "Light " + std::to_string(i);
        itemStrings.push_back(string);
    }

    itemCStrings.clear();
    for (const auto& string : itemStrings) {
        itemCStrings.push_back(string.c_str());
    }

    tempSelectedItem = static_cast<int>(curLightIndex);
    if (ImGui::ListBox("Lights", &tempSelectedItem, itemCStrings.data(), (int)itemCStrings.size(), 4)) {
        curLightIndex = static_cast<size_t>(tempSelectedItem);
    }

    selectedLight = &lights[curLightIndex];
    ImGui::Text("Selected Light Index: %d", curLightIndex);

    ImGui::Checkbox("Has Texture", &selectedLight->has_texture);
    ImGui::Checkbox("isSpotLight", &selectedLight->is_spotlight);
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

    ImGui::End();
}

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

