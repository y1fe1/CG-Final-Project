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

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClearDepth(1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        const glm::mat4 mvpMatrix = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
        const glm::mat3 normalModelMatrix = glm::inverseTranspose(glm::mat3(m_modelMatrix));

        for (GPUMesh& mesh : m_meshes) {
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

            glBindVertexArray(mesh.getVao());

            // We tell OpenGL what each vertex looks like and how they are mapped to the shader using the names
            // NOTE: Usually this can be stored in the VAO, since the locations would be the same in all shaders by using the layout(location = ...) qualifier in the shaders, however this does not work on apple devices.
            glVertexAttribPointer(m_defaultShader.getAttributeLocation("position"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
            glVertexAttribPointer(m_defaultShader.getAttributeLocation("normal"), 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            glViewport(0, 0, WIDTH, HEIGHT);

            genUboBufferObj(selectedLight, lightUBO);
            mesh.draw(m_defaultShader);
            
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

            mesh.draw(m_lightShader);
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

