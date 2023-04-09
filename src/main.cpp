#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadCubemap(vector<std::string> faces);

unsigned int loadTexture(char const * path);

void renderQuad();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
float exposure = 0.5f;
bool bloom = true;
bool bloomKeyPressed = false;

// camera

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool abduct = false; // alien abduction check
    bool flashlight = false;
    float cowHeight = -0.7f;
    bool CameraMouseMovementUpdateEnabled = true;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, -0.7f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    // stbi_set_flip_vertically_on_load(true);

    programState = new ProgramState;
    //programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    // Init Imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_BLEND);

    // build and compile shaders
    // -------------------------
    Shader objectShader("resources/shaders/object.vs", "resources/shaders/object.fs");
    Shader skyboxShader("resources/shaders/skybox.vs", "resources/shaders/skybox.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader bloomShader("resources/shaders/bloom.vs", "resources/shaders/bloom.fs");

    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    float transparentVertices[] = {
            // positions         // texture Coords (swapped y coordinates because texture is flipped upside down)
            0.0f,  0.5f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f, 1.0f,
            0.0f, -0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,

            0.0f,  0.5f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f, 1.0f,
            1.0f, -0.5f,  0.0f,  1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            1.0f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f, 1.0f
    };

    // skybox VAO
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    vector<std::string> faces
            {
                    FileSystem::getPath("resources/textures/skybox/right.jpg"),
                    FileSystem::getPath("resources/textures/skybox/left.jpg"),
                    FileSystem::getPath("resources/textures/skybox/top.jpg"),
                    FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
                    FileSystem::getPath("resources/textures/skybox/front.jpg"),
                    FileSystem::getPath("resources/textures/skybox/back.jpg")
            };
    unsigned int cubemapTexture = loadCubemap(faces);
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    // transparent VAO
    unsigned int transparentVAO, transparentVBO;
    glGenVertexArrays(1, &transparentVAO);
    glGenBuffers(1, &transparentVBO);
    glBindVertexArray(transparentVAO);
    glBindBuffer(GL_ARRAY_BUFFER, transparentVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(transparentVertices), transparentVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    glBindVertexArray(0);
    unsigned int transparentTexture = loadTexture(FileSystem::getPath("resources/textures/Corn.png").c_str());

    vector<glm::vec3> vegetation
            {
                    glm::vec3(-8.0f, -0.8f, -2.0f),
                    glm::vec3(-6.0f, -0.8f, -2.0f),
                    glm::vec3(-4.0f, -0.8f, -2.0f),
                    glm::vec3(-2.0f, -0.8f, -2.0f),
                    glm::vec3(0.0f, -0.8f, -2.0f),
                    glm::vec3(2.0f, -0.8f, -2.0f),
                    glm::vec3(4.0f, -0.8f, -2.0f),
                    glm::vec3(6.0f, -0.8f, -2.0f),
                    glm::vec3(8.0f, -0.8f, -2.0f),
                    glm::vec3(10.0f, -0.8f, -2.0f),
                    glm::vec3(12.0f, -0.8f, -2.0f),

                    glm::vec3(-8.0f, -0.8f, -3.0f),
                    glm::vec3(-6.0f, -0.8f, -3.0f),
                    glm::vec3(-4.0f, -0.8f, -3.0f),
                    glm::vec3(-2.0f, -0.8f, -3.0f),
                    glm::vec3(0.0f, -0.8f, -3.0f),
                    glm::vec3(2.0f, -0.8f, -3.0f),
                    glm::vec3(4.0f, -0.8f, -3.0f),
                    glm::vec3(6.0f, -0.8f, -3.0f),
                    glm::vec3(8.0f, -0.8f, -3.0f),
                    glm::vec3(10.0f, -0.8f, -3.0f),
                    glm::vec3(12.0f, -0.8f, -3.0f),

                    glm::vec3(-8.0f, -0.8f, -4.0f),
                    glm::vec3(-6.0f, -0.8f, -4.0f),
                    glm::vec3(-4.0f, -0.8f, -4.0f),
                    glm::vec3(-2.0f, -0.8f, -4.0f),
                    glm::vec3(0.0f, -0.8f, -4.0f),
                    glm::vec3(2.0f, -0.8f, -4.0f),
                    glm::vec3(4.0f, -0.8f, -4.0f),
                    glm::vec3(6.0f, -0.8f, -4.0f),
                    glm::vec3(8.0f, -0.8f, -4.0f),
                    glm::vec3(10.0f, -0.8f, -4.0f),
                    glm::vec3(12.0f, -0.8f, -4.0f),

                    glm::vec3(-8.0f, -0.8f, -5.0f),
                    glm::vec3(-6.0f, -0.8f, -5.0f),
                    glm::vec3(-4.0f, -0.8f, -5.0f),
                    glm::vec3(-2.0f, -0.8f, -5.0f),
                    glm::vec3(0.0f, -0.8f, -5.0f),
                    glm::vec3(2.0f, -0.8f, -5.0f),
                    glm::vec3(4.0f, -0.8f, -5.0f),
                    glm::vec3(6.0f, -0.8f, -5.0f),
                    glm::vec3(8.0f, -0.8f, -5.0f),
                    glm::vec3(10.0f, -0.8f, -5.0f),
                    glm::vec3(12.0f, -0.8f, -5.0f),

                    glm::vec3(-8.0f, -0.8f, -6.0f),
                    glm::vec3(-6.0f, -0.8f, -6.0f),
                    glm::vec3(-4.0f, -0.8f, -6.0f),
                    glm::vec3(-2.0f, -0.8f, -6.0f),
                    glm::vec3(0.0f, -0.8f, -6.0f),
                    glm::vec3(2.0f, -0.8f, -6.0f),
                    glm::vec3(4.0f, -0.8f, -6.0f),
                    glm::vec3(6.0f, -0.8f, -6.0f),
                    glm::vec3(8.0f, -0.8f, -6.0f),
                    glm::vec3(10.0f, -0.8f, -6.0f),
                    glm::vec3(12.0f, -0.8f, -6.0f),

                    glm::vec3(-8.0f, -0.8f, -7.0f),
                    glm::vec3(-6.0f, -0.8f, -7.0f),
                    glm::vec3(-4.0f, -0.8f, -7.0f),
                    glm::vec3(-2.0f, -0.8f, -7.0f),
                    glm::vec3(0.0f, -0.8f, -7.0f),
                    glm::vec3(2.0f, -0.8f, -7.0f),
                    glm::vec3(4.0f, -0.8f, -7.0f),
                    glm::vec3(6.0f, -0.8f, -7.0f),
                    glm::vec3(8.0f, -0.8f, -7.0f),
                    glm::vec3(10.0f, -0.8f, -7.0f),
                    glm::vec3(12.0f, -0.8f, -7.0f)
            };

    vector<glm::vec3> cows {
        glm::vec3(-4.0f, -0.7f, 6.0f),
        glm::vec3(-7.5f, -0.75f, 1.3f),
        glm::vec3(-8.0f, -0.7f, 5.0f)
    };

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    // load models
    // -----------
    Model UFOModel("resources/objects/UFO_Saucer/UFO_Saucer.obj");
    UFOModel.SetShaderTextureNamePrefix("material.");
    Model FieldModel("resources/objects/Field/Field.obj");
    FieldModel.SetShaderTextureNamePrefix("material.");
    Model CowModel("resources/objects/Cow/Cow.obj");
    CowModel.SetShaderTextureNamePrefix("material.");
    Model TruckModel("resources/objects/Truck/Truck.obj");
    TruckModel.SetShaderTextureNamePrefix("material.");

    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

    blurShader.use();
    blurShader.setInt("image", 0);
    bloomShader.use();
    bloomShader.setInt("scene", 0);
    bloomShader.setInt("bloomBlur", 1);

    // load lights
    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(1.0, 1.0, 1.0);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(1.0, 1.0, 1.0);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    // draw in wireframe
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        // render
        // ------
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // HDR
        glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        objectShader.use();

        // Directional light for objects
        objectShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        objectShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f); // 0.05 for gloomy
        objectShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        objectShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // Point light for objects
        pointLight.position = glm::vec3(4.0f, 4.0f, 4.0);
        objectShader.setVec3("pointLight.position", pointLight.position);
        objectShader.setVec3("pointLight.ambient", pointLight.ambient);
        objectShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        objectShader.setVec3("pointLight.specular", pointLight.specular);
        objectShader.setFloat("pointLight.constant", pointLight.constant);
        objectShader.setFloat("pointLight.linear", pointLight.linear);
        objectShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        objectShader.setVec3("viewPosition", programState->camera.Position);
        objectShader.setFloat("material.shininess", 32.0f);

        objectShader.setVec3("spotLights[0].position", glm::vec3(-6.0f, 1.0f, 4.0f));
        objectShader.setVec3("spotLights[0].direction", glm::vec3(0.0f, -1.0f , 0.0f));
        objectShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->abduct){
            objectShader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
            objectShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            objectShader.setVec3("spotLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            objectShader.setVec3("spotLights[0].specular", 0.0f, 0.0f, 0.0f);
        }
        objectShader.setFloat("spotLights[0].constant", 1.0f);
        objectShader.setFloat("spotLights[0].linear", 0.09);
        objectShader.setFloat("spotLights[0].quadratic", 0.032);
        objectShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
        objectShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));

        // flashlight

        objectShader.setVec3("spotLights[1].position", programState->camera.Position);
        objectShader.setVec3("spotLights[1].direction", programState->camera.Front);
        objectShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->flashlight){
            objectShader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
            objectShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            objectShader.setVec3("spotLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            objectShader.setVec3("spotLights[1].specular", 0.0f, 0.0f, 0.0f);
        }
        objectShader.setFloat("spotLights[1].constant", 1.0f);
        objectShader.setFloat("spotLights[1].linear", 0.09);
        objectShader.setFloat("spotLights[1].quadratic", 0.032);
        objectShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(12.5f)));
        objectShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(15.0f)));

        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = programState->camera.GetViewMatrix();
        objectShader.setMat4("projection", projection);
        objectShader.setMat4("view", view);

        // render the loaded UFO model
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(-6.0f, 1.0f, 4.0f));
        model = glm::rotate(model,(float)glfwGetTime(),glm::vec3(0, 1, 0));
        model = glm::rotate(model,glm::radians(90.0f),glm::vec3(0,0,1));
        model = glm::rotate(model,glm::radians(90.0f),glm::vec3(0,1,0));
        model = glm::scale(model, glm::vec3(0.01f));
        objectShader.setMat4("model", model);
        UFOModel.Draw(objectShader);

        // render the loaded Field model
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.3f));
        objectShader.setMat4("model", model);
        FieldModel.Draw(objectShader);

        // render the loaded Cow model (cow to be abducted)
        if(!programState->abduct) {
            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(-6.0f, -0.7f, 4.0f));
            model = glm::scale(model, glm::vec3(0.005f));
            objectShader.setMat4("model", model);
            CowModel.Draw(objectShader);
        }
        else if(programState->cowHeight < 1.3f){
            programState->cowHeight += 0.02f;
            model = glm::mat4(1.0f);
            model = glm::translate(model,glm::vec3(-6.0f, programState->cowHeight, 4.0f));
            model = glm::scale(model, glm::vec3(0.005f));
            objectShader.setMat4("model", model);
            CowModel.Draw(objectShader);
        }

        //render the regular cows
        for (unsigned int i = 0; i < cows.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, cows[i]);
            model = glm::scale(model, glm::vec3(0.005f));
            objectShader.setMat4("model", model);
            CowModel.Draw(objectShader);
        }

        //render the vehicle
        model = glm::mat4(1.0f);
        model = glm::translate(model,glm::vec3(0.0f, -0.77f, 8.0f));
        model = glm::rotate(model,glm::radians(-90.0f),glm::vec3(0,1,0));
        model = glm::rotate(model,glm::radians(-5.0f),glm::vec3(0,0,1));
        model = glm::rotate(model,glm::radians(-15.0f),glm::vec3(1,0,0));
        model = glm::scale(model, glm::vec3(0.03f));
        objectShader.setMat4("model", model);
        TruckModel.Draw(objectShader);

        // vegetation
        glDisable(GL_CULL_FACE);
        blendingShader.use();

        // Directional light for objects
        blendingShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        blendingShader.setVec3("dirLight.ambient", 0.05f, 0.05f, 0.05f); // 0.05 for gloomy
        blendingShader.setVec3("dirLight.diffuse", 0.4f, 0.4f, 0.4f);
        blendingShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // Point light for objects
        pointLight.position = glm::vec3(4.0f, 4.0f, 4.0);
        blendingShader.setVec3("pointLight.position", pointLight.position);
        blendingShader.setVec3("pointLight.ambient", pointLight.ambient);
        blendingShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        blendingShader.setVec3("pointLight.specular", pointLight.specular);
        blendingShader.setFloat("pointLight.constant", pointLight.constant);
        blendingShader.setFloat("pointLight.linear", pointLight.linear);
        blendingShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        blendingShader.setVec3("viewPosition", programState->camera.Position);
        blendingShader.setFloat("material.shininess", 32.0f);

        blendingShader.setVec3("spotLights[0].position", glm::vec3(-6.0f, 1.0f, 4.0f));
        blendingShader.setVec3("spotLights[0].direction", glm::vec3(0.0f, -1.0f , 0.0f));
        blendingShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->abduct){
            blendingShader.setVec3("spotLights[0].ambient", 0.0f, 0.0f, 0.0f);
            blendingShader.setVec3("spotLights[0].diffuse", 1.0f, 1.0f, 1.0f);
            blendingShader.setVec3("spotLights[0].specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            blendingShader.setVec3("spotLights[0].diffuse", 0.0f, 0.0f, 0.0f);
            blendingShader.setVec3("spotLights[0].specular", 0.0f, 0.0f, 0.0f);
        }
        blendingShader.setFloat("spotLights[0].constant", 1.0f);
        blendingShader.setFloat("spotLights[0].linear", 0.09);
        blendingShader.setFloat("spotLights[0].quadratic", 0.032);
        blendingShader.setFloat("spotLights[0].cutOff", glm::cos(glm::radians(12.5f)));
        blendingShader.setFloat("spotLights[0].outerCutOff", glm::cos(glm::radians(15.0f)));

        // flashlight

        blendingShader.setVec3("spotLights[1].position", programState->camera.Position);
        blendingShader.setVec3("spotLights[1].direction", programState->camera.Front);
        blendingShader.setVec3("spotLights[1].ambient", 0.0f, 0.0f, 0.0f);
        if(programState->flashlight){
            blendingShader.setVec3("spotLights[1].diffuse", 1.0f, 1.0f, 1.0f);
            blendingShader.setVec3("spotLights[1].specular", 1.0f, 1.0f, 1.0f);
        }
        else {
            blendingShader.setVec3("spotLights[1].diffuse", 0.0f, 0.0f, 0.0f);
            blendingShader.setVec3("spotLights[1].specular", 0.0f, 0.0f, 0.0f);
        }
        blendingShader.setFloat("spotLights[1].constant", 1.0f);
        blendingShader.setFloat("spotLights[1].linear", 0.09);
        blendingShader.setFloat("spotLights[1].quadratic", 0.032);
        blendingShader.setFloat("spotLights[1].cutOff", glm::cos(glm::radians(12.5f)));
        blendingShader.setFloat("spotLights[1].outerCutOff", glm::cos(glm::radians(15.0f)));

        blendingShader.setMat4("view", view);
        blendingShader.setMat4("projection", projection);

        glBindVertexArray(transparentVAO);
        glBindTexture(GL_TEXTURE_2D, transparentTexture);
        for (unsigned int i = 0; i < vegetation.size(); i++)
        {
            model = glm::mat4(1.0f);
            model = glm::translate(model, vegetation[i]);
            //model = glm::scale(model, glm::vec3(0.5f));
            blendingShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glEnable(GL_CULL_FACE);

        // draw skybox as last
        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        skyboxShader.use();
        view = glm::mat4(glm::mat3(programState->camera.GetViewMatrix())); // remove translation from the view matrix
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);

        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        // 2. blur bright fragments with two-pass Gaussian Blur
        // --------------------------------------------------
        bool horizontal = true, first_iteration = true;
        unsigned int amount = 10;
        blurShader.use();
        for (unsigned int i = 0; i < amount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]); // bind texture of other framebuffer (or scene if first iteration)
            renderQuad();
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 3. now render floating point color buffer to 2D quad and tonemap HDR colors to default framebuffer's (clamped) color range
        // --------------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        bloomShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        bloomShader.setInt("bloom", bloom);
        bloomShader.setFloat("exposure", exposure);
        renderQuad();

        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if(glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        programState->abduct = true;

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
        if(programState->flashlight){
            programState->flashlight = false;
        }
        else{
            programState->flashlight = true;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !bloomKeyPressed)
    {
        bloom = !bloom;
        bloomKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        bloomKeyPressed = false;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int loadTexture(char const * path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}