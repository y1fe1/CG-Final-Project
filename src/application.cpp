// Define More header if needed
#include "application.h"

// Constructor
Application::Application()
    : m_window("Final Project", glm::ivec2(1024, 1024), OpenGLVersion::GL41)
    , m_texture(RESOURCE_ROOT "resources/checkerboard.png")
    , m_projectionMatrix(glm::perspective(glm::radians(80.0f), 1.0f, 0.1f, 30.0f))
    , m_viewMatrix(glm::lookAt(glm::vec3(-1, 1, -1), glm::vec3(0), glm::vec3(0, 1, 0)))
    , m_modelMatrix(1.0f)
    , cameras{
        { &m_window, glm::vec3(1.2f, 1.1f, 0.9f), -glm::vec3(1.2f, 1.1f, 0.9f) },
        { &m_window, glm::vec3(3.8f, 1.0f, 0.06f), -glm::vec3(1.8f, 1.0f, 0.5f) }
    }
{
    //Camera defaultCamera = Camera(&m_window);

    // lights must be initialized here since light is still struct not class
    lights.push_back(
        { glm::vec3(0, 0, 3), glm::vec3(1), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    lights.push_back(
        { glm::vec3(0, 0, 2), glm::vec3(2), -glm::vec3(0, 0, 3), false, false, /*std::nullopt*/ }
    );

    m_Material.ks = glm::vec3{ 0.5f, 0.5f, 1.0f };
    m_Material.kd = glm::vec3{ 0.1, 1.0, 0.1 };


    selectedCamera = &cameras.at(curCameraIndex);
    selectedLight = &lights.at(curLightIndex);

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

    m_meshes = GPUMesh::loadMeshGPU(RESOURCE_ROOT "resources/dragon.obj");

    try {
        ShaderBuilder defaultBuilder;
        defaultBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shader_vert.glsl");
        defaultBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/shader_frag.glsl");
        m_defaultShader = defaultBuilder.build();

        ShaderBuilder shadowBuilder;
        shadowBuilder.addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/shadow_vert.glsl");
        shadowBuilder.addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "Shaders/shadow_frag.glsl");
        m_shadowShader = shadowBuilder.build();

        // set up light Shader
        ShaderBuilder lightShaderBuilder;
        lightShaderBuilder
            .addStage(GL_VERTEX_SHADER, RESOURCE_ROOT "shaders/light_vert.glsl")
            .addStage(GL_FRAGMENT_SHADER, RESOURCE_ROOT "shaders/light_frag.glsl");
        m_lightShader = lightShaderBuilder.build();

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

        initPostProcess();
    }
    catch (ShaderLoadingException e) {
        std::cerr << e.what() << std::endl;
    }
}

void Application::update() {
    int dummyInteger = 0;
    while (!m_window.shouldClose()) {
        m_window.updateInput();
        this->imgui();

        selectedCamera->updateInput();
        m_viewMatrix = selectedCamera->viewMatrix();

        if (usePostProcess) {
            // 绑定自定义的帧缓冲对象
            glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostProcess);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
        else {
            // 绑定默认帧缓冲对象（屏幕）
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            //glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        for (GPUMesh& mesh : m_meshes) {

            // set new Material every time it is updated
            GLuint newUBOMaterial;
            genUboBufferObj(m_Material, newUBOMaterial);
            mesh.setUBOMaterial(newUBOMaterial);

            m_defaultShader.bind();
            glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
            glUniformMatrix3fv(m_defaultShader.getUniformLocation("normalModelMatrix"), 1, GL_FALSE, glm::value_ptr(normalModelMatrix));

            if (mesh.hasTextureCoords()) {
                m_texture.bind(GL_TEXTURE0);
                glUniform1i(m_defaultShader.getUniformLocation("colorMap"), 0);
                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_TRUE);
                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), GL_FALSE);
            }
            else {
                glUniform1i(m_defaultShader.getUniformLocation("hasTexCoords"), GL_FALSE);
                glUniform1i(m_defaultShader.getUniformLocation("useMaterial"), m_useMaterial);
            }

            genUboBufferObj(selectedLight, lightUBO);
            mesh.draw(m_defaultShader);
            renderMiniMap();
            
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

        if (usePostProcess) {
            // 绑定默认帧缓冲对象，将结果绘制到屏幕

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            runPostProcess();
            glFinish();
        }

        m_window.swapBuffers();
    }
}

void Application::imgui() {
    ImGui::Begin("Assignment 2 Demo");

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
    ImGui::Checkbox("Use Bezier", &selectedCamera->m_useBezier);
    ImGui::Checkbox("Use Constant Speed", &selectedCamera->m_bezierConstantSpeed);
    ImGui::DragFloat3("P0", &selectedCamera->P0.x);
    ImGui::DragFloat3("P1", &selectedCamera->P1.x);
    ImGui::DragFloat3("P2", &selectedCamera->P2.x);
    ImGui::DragFloat3("P3", &selectedCamera->P3.x);

    ImGui::Checkbox("usePostProcess", &usePostProcess);
    // Button for clearing Camera
    if (ImGui::Button("Reset Cameras")) {
        //resetObjList(cameras,defaultLight);
    }

    ImGui::Separator();
    ImGui::Text("Material parameters");
    ImGui::SliderFloat("Shininess", &m_Material.shininess, 0.0f, 100.f);

    ImGui::Separator();
    // Color pickers for Kd and Ks
    ImGui::ColorEdit3("Kd", &m_Material.kd[0]);
    ImGui::ColorEdit3("Ks", &m_Material.ks[0]);

   /* ImGui::SliderInt("Toon Discretization", &m_Material.toonDiscretize, 1, 10);
    ImGui::SliderFloat("Toon Specular Threshold", &m_Material.toonSpecularThreshold, 0.0f, 1.0f);*/

    ImGui::Separator();
    ImGui::Text("Shadow modes");
    ImGui::Checkbox("Shadow Enabled", &shadowSettings.shadowEnabled);
    ImGui::Checkbox("PCF Enabled", &shadowSettings.pcfEnabled);

    ImGui::Separator();
    ImGui::Text("Lights");

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
        lights.push_back(Light{ glm::vec3(0.4f, 1.2f, 0.0f), glm::vec3(1) });
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

    ImGui::Separator();
    ImGui::Checkbox("Use material if no texture", &m_useMaterial);
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



void Application::renderMiniMap() {

    glViewport(800, 800, 200, 200); // Make it to up-right

    // 使用小地图的视图矩阵和投影矩阵渲染场景
    m_defaultShader.bind();
    const glm::mat4 mvpMatrix = minimap.projectionMatrix() * minimap.viewMatrix() * m_modelMatrix;
    glUniformMatrix4fv(m_defaultShader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));

    // 渲染小地图内容
    for (GPUMesh& mesh : m_meshes) {
        mesh.draw(m_defaultShader);
    }

    const glm::mat4 minimapVP = minimap.projectionMatrix() * minimap.viewMatrix() * m_modelMatrix;
    glm::vec4 cameraPosInMinimap = minimapVP * glm::vec4(selectedCamera->cameraPos(), 1.0f);
    cameraPosInMinimap /= cameraPosInMinimap.w; // 透视除法，得到标准化设备坐标
    
    drawCameraPositionOnMinimap(cameraPosInMinimap);

    // 恢复主视口
    glViewport(0, 0, 1024, 1024);
    drawMiniMapBorder();
}


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

//void Application::initPostProcess() {
//
//    glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostProcess);
//
//    glGenTextures(1, &texturePostProcess);
//    glBindTexture(GL_TEXTURE_2D, texturePostProcess);
//    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
//    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturePostProcess, 0);
//
//    glGenRenderbuffers(1, &depthbufferPostProcess);
//    glBindRenderbuffer(GL_RENDERBUFFER, depthbufferPostProcess);
//    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, WIDTH, HEIGHT);
//    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbufferPostProcess);
//
//}
// 在构造函数或初始化函数中
void Application::initPostProcess() {
    // 创建帧缓冲对象
    glGenFramebuffers(1, &framebufferPostProcess);
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferPostProcess);

    // 创建颜色纹理附件
    //glActiveTexture(GL_TEXTURE1); // 激活 GL_TEXTURE1
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


void Application::runPostProcess() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // 使用后期处理着色器
    m_postProcessShader.bind();

    // 激活并绑定纹理单元
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texturePostProcess); // 绑定自定义帧缓冲对象的颜色纹理附件

    // 设置着色器中的采样器
    glUniform1i(m_postProcessShader.getUniformLocation("scene"), 0);


    glDisable(GL_DEPTH_TEST);
    // 渲染全屏四边形，应用后期处理效果
    renderFullScreenQuad();
    glEnable(GL_DEPTH_TEST);

}

void Application::renderFullScreenQuad() {
    static unsigned int quadVAO = 0;
    static unsigned int quadVBO;
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (quadVAO == 0) {
        float quadVertices[] = {
            // 位置        // 纹理坐标
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        // 设置 VAO 和 VBO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW );

        // 位置属性
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        // 纹理坐标属性
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),  (void*)(2 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}
