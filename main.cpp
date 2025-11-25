#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);
void updateCameraDirection();

glm::vec3 cameraPos = glm::vec3(0.0f, 12.0f, 18.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.5f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 playerPos = glm::vec3(-4.0f, 0.6f, 5.0f);

//GRACZ

float yaw = -90.0f;
float pitch = 0.0f;
float speed = 1.0f;
float lastX = 800 / 2.0f;
float lastY = 600 / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ZMIENNE RUCHU
glm::vec3 playerVelocity = glm::vec3(0.0f);
float moveSpeed = 3.35f;      // docelowa prêdkoœæ w jednostkach/s — skaluj wedle potrzeby
float accel = 30.0f;         // szybkoœæ osi¹gania prêdkoœci (im wiêksze, tym szybsze przyspieszenie)
float stopFactor = 1.0f;    // si³a "hamowania" (wyg³adzenie zatrzymania)

const char* vertexShaderSource = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() { gl_Position = projection * view * model * vec4(aPos,1.0); }
)glsl";

const char* fragmentShaderSource = R"glsl(
#version 330 core
out vec4 FragColor;
uniform vec3 color;
void main() { FragColor = vec4(color,1.0); }
)glsl";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Labirynt 10x10", NULL, NULL);
    if (!window)
    {
        std::cout << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
    { 
        std::cout << "Failed to initialize GLAD\n"; return -1; 
    }

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);

    float cubeVertices[] = {
        -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f,0.5f,-0.5f,
         0.5f,0.5f,-0.5f,-0.5f,0.5f,-0.5f,-0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,0.5f, 0.5f,-0.5f,0.5f, 0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f,-0.5f,-0.5f,0.5f,
        -0.5f,0.5f,0.5f,-0.5f,0.5f,-0.5f,-0.5f,-0.5f,-0.5f,
        -0.5f,-0.5f,-0.5f,-0.5f,-0.5f,0.5f,-0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,0.5f,0.5f,-0.5f,0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,0.5f,-0.5f,0.5f,0.5f,0.5f,0.5f,
        -0.5f,-0.5f,-0.5f,0.5f,-0.5f,-0.5f,0.5f,-0.5f,0.5f,
         0.5f,-0.5f,0.5f,-0.5f,-0.5f,0.5f,-0.5f,-0.5f,-0.5f,
        -0.5f,0.5f,-0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f,0.5f,
         0.5f,0.5f,0.5f,-0.5f,0.5f,0.5f,-0.5f,0.5f,-0.5f
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- labirynt 10x10 z wejœciem i wyjœciem ---
    int maze[10][10] = {
        {1,0,1,1,1,1,1,1,1,1},  // wejœcie (0,1)
        {1,0,0,0,1,0,0,0,0,1},
        {1,1,1,0,1,0,1,1,0,1},
        {1,0,0,0,0,0,1,0,0,1},
        {1,0,1,1,1,0,1,0,1,1},
        {1,0,1,0,0,0,1,0,0,1},
        {1,0,1,0,1,1,1,1,0,1},
        {1,0,0,0,0,0,0,1,0,1},
        {1,1,1,1,1,1,0,0,0,1},  // wyjœcie (9,8)
        {1,1,1,1,1,1,1,1,0,1}
    };

    std::vector<glm::vec3> wallPositions;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (maze[i][j] == 1)
                wallPositions.push_back(glm::vec3(j - 5, 0.5f, -(i - 5))); // wycentrowanie labiryntu
        }
    }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //RUCH GRACZA - p³ynny (velocity smoothing)

        // OBLICZANIE KIERUNKÓW XZ
        glm::vec3 forwardXZ(0.0f);
        float fLen = sqrt(cameraFront.x * cameraFront.x + cameraFront.z * cameraFront.z);
        if (fLen > 1e-6f)
            forwardXZ = glm::vec3(cameraFront.x / fLen, 0.0f, cameraFront.z / fLen);
        else
            forwardXZ = glm::vec3(0.0f, 0.0f, -1.0f); // fallback

        glm::vec3 right = glm::normalize(glm::cross(forwardXZ, cameraUp));

        // INPUT W DOCELOW¥ PRÊDKOŒÆ
        glm::vec3 desiredVel(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
            desiredVel += forwardXZ;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
            desiredVel -= forwardXZ;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
            desiredVel -= right;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
            desiredVel += right;

        if (glm::length(desiredVel) > 0.001f)
            desiredVel = glm::normalize(desiredVel) * moveSpeed;
        else
            desiredVel = glm::vec3(0.0f);

        // WYG£ADZENIE
        float blend = glm::clamp(accel * deltaTime, 0.0f, 1.0f);
        playerVelocity += (desiredVel - playerVelocity) * blend;

        // DODATKOWE HAMOWANIE
        if (glm::length(desiredVel) < 0.001f)
        {
            float brake = glm::clamp(stopFactor * deltaTime, 0.0f, 1.0f);
            playerVelocity = glm::mix(playerVelocity, glm::vec3(0.0f), brake);
        }

        // SKALOWANY RUCH PRZEZ DELTATIME
        glm::vec3 move = playerVelocity * deltaTime;

        if (glm::length(move) > 0.0001f) {
            std::cout << "=================\n";
            std::cout << "LINIJKA DO DEBUGU\n";
            std::cout << "move = " << move.x << "," << move.y << "," << move.z << " dt=" << deltaTime << " vel="<< playerVelocity.x << "," << playerVelocity.z << "\n";
            std::cout << "moveSpeed=" << moveSpeed << "\n";
        }

        // KOLIZJA
        auto collides = [&](glm::vec3 newPos)
            {
                float halfSize = 0.4f;
                float wallSize = 0.5f;

                for (int i = 0; i < 10; i++)
                {
                    for (int j = 0; j < 10; j++)
                    {
                        if (maze[i][j] == 1)
                        {
                            glm::vec3 wPos = glm::vec3(j - 5, 0.5f, -(i - 5));

                            if (fabs(newPos.x - wPos.x) < halfSize + wallSize &&
                                fabs(newPos.z - wPos.z) < halfSize + wallSize)
                            {
                                return true;
                            }
                        }
                    }
                }
                return false;
            };

        glm::vec3 newPos = playerPos;
        glm::vec3 testX = newPos + glm::vec3(move.x, 0.0f, 0.0f);
        if (!collides(testX))
            newPos.x = testX.x;

        glm::vec3 testZ = newPos + glm::vec3(0.0f, 0.0f, move.z);
        if (!collides(testZ))
            newPos.z = testZ.z;

        playerPos = newPos;

        processInput(window);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        //STARA KAMERKA TU
        //glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        //NOWA KAMERKA TU
        glm::mat4 view = glm::lookAt(playerPos, playerPos + cameraFront, cameraUp);


        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);

        // pod³oga powiêkszona
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(12.0f, 0.1f, 12.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.3f, 0.8f, 0.3f);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // œciany
        for (auto& pos : wallPositions) {
            model = glm::mat4(1.0f);
            model = glm::translate(model, pos);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3f(glGetUniformLocation(shaderProgram, "color"), 0.8f, 0.3f, 0.3f);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    // DEBUG DO ZMNIEJSZANIA I ZWIÊKSZANIA PRÊDKOŒCI RUCHU
    if (glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) {
        moveSpeed += 1.0f * deltaTime * 60.0f;
        std::cout << "moveSpeed=" << moveSpeed << "\n";
    }
    if (glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) {
        moveSpeed = std::max(0.1f, moveSpeed - 1.0f * deltaTime * 60.0f);
        std::cout << "moveSpeed=" << moveSpeed << "\n";
    }

    float cameraSpeed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) 
        glfwSetWindowShouldClose(window, true);
}


//NOWY CALLBACK

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float dx = xpos - lastX;
    float dy = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.08f;
    dx *= sensitivity;
    dy *= sensitivity;

    yaw += dx;
    pitch += dy;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraDirection();
}

void updateCameraDirection()
{
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(dir);
}