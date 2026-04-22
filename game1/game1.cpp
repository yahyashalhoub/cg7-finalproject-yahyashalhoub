#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <string>

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ========== Õ«·… «··⁄»… ==========
enum GameState { PLAYING, WIN, LOSE };
GameState gameState = PLAYING;
int score = 0;
const int WIN_SCORE = 5;  //  „ «· €ÌÌ— „‰ 10 ≈·Ï 5
float gameTimer = 120.0f;

// ========== ‘Œ’Ì… «··«⁄» („«—ÌÊ) ==========
struct Player {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;
    float rotation;
    bool onGround;
    float walkCycle;
};
Player player;

// ========== «·⁄„·«  «·–Â»Ì… ==========
struct Coin {
    glm::vec3 position;
    bool active;
    float rotation;
    float bobOffset;
};
std::vector<Coin> coins;
const int TOTAL_COINS = 10;  // ≈Ã„«·Ì «·⁄„·«  ›Ì «·„” ÊÏ

// ========== «·„‰’«  («·„—»⁄«  «·»‰Ì…) ==========
struct Platform {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
};
std::vector<Platform> platforms;

// ========== «·√⁄œ«¡ ==========
struct Enemy {
    glm::vec3 position;
    glm::vec3 velocity;
    bool active;
    float patrolTimer;
};
std::vector<Enemy> enemies;

// ========== ﬂ«„Ì—« ==========
glm::vec3 cameraPos = glm::vec3(0.0f, 8.0f, 15.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// ========== ›Ì“Ì«¡ ==========
const float GRAVITY = -15.0f;
const float JUMP_FORCE = 7.5f;  // “Ì«œ… ﬁÊ… «·ﬁ›“ ﬁ·Ì·«
const float PLAYER_SPEED = 6.0f;

// ========== OpenGL Objects ==========
unsigned int cubeVAO = 0, cubeVBO = 0;
unsigned int sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
unsigned int cylinderVAO = 0, cylinderVBO = 0, cylinderEBO = 0;
unsigned int shaderProgram;
int sphereIndexCount = 0;
int cylinderIndexCount = 0;

// ========== œÊ«· „”«⁄œ… ==========
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

bool checkCollision(glm::vec3 posA, glm::vec3 sizeA, glm::vec3 posB, glm::vec3 sizeB) {
    return (fabs(posA.x - posB.x) < (sizeA.x + sizeB.x) / 2) &&
        (fabs(posA.y - posB.y) < (sizeA.y + sizeB.y) / 2) &&
        (fabs(posA.z - posB.z) < (sizeA.z + sizeB.z) / 2);
}

unsigned int compileShader(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexSrc, NULL);
    glCompileShader(vertex);

    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentSrc, NULL);
    glCompileShader(fragment);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// ========== Shaders ==========
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
out vec3 FragPos;
out vec3 Normal;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 objectColor;
uniform vec3 lightPos;
void main() {
    float ambient = 0.5;
    vec3 lightColor = vec3(1.0, 1.0, 0.95);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 result = (ambient + diff) * objectColor * lightColor;
    FragColor = vec4(result, 1.0);
}
)";

// ==========  Ê·Ìœ «·√‘ﬂ«· ==========
void generateSphere(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius, unsigned int sectors, unsigned int stacks) {
    vertices.clear(); indices.clear();

    float x, y, z, xy;
    float nx, ny, nz, lengthInv = 1.0f / radius;
    float sectorStep = 2.0f * 3.14159f / sectors;
    float stackStep = 3.14159f / stacks;
    float sectorAngle, stackAngle;

    for (unsigned int i = 0; i <= stacks; ++i) {
        stackAngle = 3.14159f / 2.0f - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for (unsigned int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;

            vertices.push_back(x); vertices.push_back(y); vertices.push_back(z);
            vertices.push_back(nx); vertices.push_back(ny); vertices.push_back(nz);
        }
    }

    unsigned int k1, k2;
    for (unsigned int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;
        for (unsigned int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) { indices.push_back(k1); indices.push_back(k2); indices.push_back(k1 + 1); }
            if (i != (stacks - 1)) { indices.push_back(k1 + 1); indices.push_back(k2); indices.push_back(k2 + 1); }
        }
    }
}

void generateCylinder(std::vector<float>& vertices, std::vector<unsigned int>& indices,
    float radius, float height, unsigned int sectors) {
    vertices.clear(); indices.clear();

    float sectorStep = 2.0f * 3.14159f / sectors;
    float halfHeight = height / 2.0f;

    for (unsigned int i = 0; i <= sectors; ++i) {
        float angle = i * sectorStep;
        float x = radius * cosf(angle);
        float z = radius * sinf(angle);

        vertices.push_back(x); vertices.push_back(-halfHeight); vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(halfHeight); vertices.push_back(z);
        vertices.push_back(x); vertices.push_back(0.0f); vertices.push_back(z);
    }

    for (unsigned int i = 0; i < sectors; ++i) {
        unsigned int base = i * 4;
        indices.push_back(base); indices.push_back(base + 4); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 4); indices.push_back(base + 6);
        indices.push_back(base + 1); indices.push_back(base + 3); indices.push_back(base + 5);
        indices.push_back(base + 5); indices.push_back(base + 3); indices.push_back(base + 7);
    }
}

// ========== œÊ«· «·—”„ ==========
void renderCube(glm::vec3 position, glm::vec3 size, glm::vec3 color, float rotate = 0.0f) {
    glBindVertexArray(cubeVAO);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, size);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void renderSphere(glm::vec3 position, float radius, glm::vec3 color) {
    glBindVertexArray(sphereVAO);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::scale(model, glm::vec3(radius * 2.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void renderCylinder(glm::vec3 position, float radius, float height, glm::vec3 color, float rotate = 0.0f) {
    glBindVertexArray(cylinderVAO);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, position);
    model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(radius * 2.0f, height, radius * 2.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(color));
    glDrawElements(GL_TRIANGLES, cylinderIndexCount, GL_UNSIGNED_INT, 0);
}

// ========== HUD ==========
void renderHUD() {
    std::string status;
    std::string colorCode;

    if (gameState == WIN) {
        status = "!!! YOU WIN !!!";
        colorCode = "\033[1;32m";
    }
    else if (gameState == LOSE) {
        status = "!!! GAME OVER !!!";
        colorCode = "\033[1;31m";
    }
    else {
        status = "PLAYING";
        colorCode = "\033[1;36m";
    }

    std::cout << "\r" << colorCode
        << "[SCORE: " << score << "/" << WIN_SCORE << "] "
        << "[TIME: " << (int)gameTimer << "s] "
        << "[WASD: Move] [SPACE: Jump] [R: Restart] [ESC: Exit] "
        << "[" << status << "]"
        << "\033[0m" << std::flush;
}

// ==========  ÂÌ∆… «··⁄»… ==========
void initGame() {
    gameState = PLAYING;
    score = 0;
    gameTimer = 120.0f;

    // «··«⁄» („«—ÌÊ)
    player.position = glm::vec3(0.0f, 2.0f, 5.0f);
    player.velocity = glm::vec3(0.0f);
    player.color = glm::vec3(1.0f, 0.2f, 0.2f);
    player.rotation = 0.0f;
    player.onGround = false;
    player.walkCycle = 0.0f;

    // «·„‰’«  («·„—»⁄«  «·»‰Ì…)
    platforms.clear();
    // √—÷Ì… —∆Ì”Ì…
    platforms.push_back({ glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(20.0f, 1.0f, 30.0f), glm::vec3(0.55f, 0.35f, 0.15f) });
    // „‰’«  „— ›⁄… (»‰Ì…)
    platforms.push_back({ glm::vec3(-4.0f, 1.5f, 0.0f), glm::vec3(3.0f, 0.5f, 3.0f), glm::vec3(0.6f, 0.4f, 0.15f) });
    platforms.push_back({ glm::vec3(5.0f, 1.5f, -2.0f), glm::vec3(3.0f, 0.5f, 3.0f), glm::vec3(0.6f, 0.4f, 0.15f) });
    platforms.push_back({ glm::vec3(-3.0f, 3.0f, -5.0f), glm::vec3(3.0f, 0.5f, 3.0f), glm::vec3(0.65f, 0.45f, 0.2f) });
    platforms.push_back({ glm::vec3(6.0f, 3.0f, 4.0f), glm::vec3(3.0f, 0.5f, 3.0f), glm::vec3(0.65f, 0.45f, 0.2f) });
    platforms.push_back({ glm::vec3(-6.0f, 4.5f, 3.0f), glm::vec3(2.0f, 0.5f, 2.0f), glm::vec3(0.7f, 0.5f, 0.25f) });
    platforms.push_back({ glm::vec3(3.0f, 4.5f, -6.0f), glm::vec3(2.0f, 0.5f, 2.0f), glm::vec3(0.7f, 0.5f, 0.25f) });
    platforms.push_back({ glm::vec3(0.0f, 6.0f, 7.0f), glm::vec3(2.0f, 0.5f, 2.0f), glm::vec3(0.75f, 0.55f, 0.3f) });
    platforms.push_back({ glm::vec3(-7.0f, 2.5f, -8.0f), glm::vec3(2.0f, 0.5f, 2.0f), glm::vec3(0.6f, 0.4f, 0.15f) });
    platforms.push_back({ glm::vec3(7.0f, 2.5f, 8.0f), glm::vec3(2.0f, 0.5f, 2.0f), glm::vec3(0.6f, 0.4f, 0.15f) });
    // Ãœ—«‰ ÕœÊœÌ…
    platforms.push_back({ glm::vec3(-11.0f, 2.0f, 0.0f), glm::vec3(1.0f, 4.0f, 30.0f), glm::vec3(0.5f, 0.3f, 0.1f) });
    platforms.push_back({ glm::vec3(11.0f, 2.0f, 0.0f), glm::vec3(1.0f, 4.0f, 30.0f), glm::vec3(0.5f, 0.3f, 0.1f) });
    platforms.push_back({ glm::vec3(0.0f, 2.0f, -15.0f), glm::vec3(22.0f, 4.0f, 1.0f), glm::vec3(0.5f, 0.3f, 0.1f) });
    platforms.push_back({ glm::vec3(0.0f, 2.0f, 15.0f), glm::vec3(22.0f, 4.0f, 1.0f), glm::vec3(0.5f, 0.3f, 0.1f) });

    // «·⁄„·«  - Ê÷⁄Â« ›Êﬁ «·„‰’« 
    coins.clear();
    glm::vec3 coinPositions[TOTAL_COINS] = {
        glm::vec3(-4.0f, 2.2f, 0.0f),     // ›Êﬁ √Ê· „‰’…
        glm::vec3(5.0f, 2.2f, -2.0f),     // ›Êﬁ À«‰Ì „‰’…
        glm::vec3(-3.0f, 3.7f, -5.0f),    // ›Êﬁ À«·À „‰’…
        glm::vec3(6.0f, 3.7f, 4.0f),      // ›Êﬁ —«»⁄ „‰’…
        glm::vec3(-6.0f, 5.2f, 3.0f),     // ›Êﬁ Œ«„” „‰’…
        glm::vec3(3.0f, 5.2f, -6.0f),     // ›Êﬁ ”«œ” „‰’…
        glm::vec3(0.0f, 6.7f, 7.0f),      // ›Êﬁ ”«»⁄ „‰’…
        glm::vec3(-7.0f, 3.2f, -8.0f),    // ›Êﬁ À«„‰ „‰’…
        glm::vec3(7.0f, 3.2f, 8.0f),      // ›Êﬁ  «”⁄ „‰’…
        glm::vec3(2.0f, 1.2f, -3.0f)      // ⁄·Ï «·√—÷
    };

    for (int i = 0; i < TOTAL_COINS; i++) {
        coins.push_back({ coinPositions[i], true, 0.0f, (float)(i * 0.5f) });
    }

    // «·√⁄œ«¡
    enemies.clear();
    enemies.push_back({ glm::vec3(-2.0f, 1.0f, -2.0f), glm::vec3(1.5f, 0.0f, 0.0f), true, 0.0f });
    enemies.push_back({ glm::vec3(3.0f, 1.0f, 3.0f), glm::vec3(-1.5f, 0.0f, 0.0f), true, 1.0f });
}

// ========== «·œ«·… «·—∆Ì”Ì… ==========
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Mario 3D - Jump & Collect (5 Coins to Win)", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);

    shaderProgram = compileShader(vertexShaderSource, fragmentShaderSource);

    // »Ì«‰«  «·„ﬂ⁄»
    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // »Ì«‰«  «·ﬂ—…
    std::vector<float> sphereVerts;
    std::vector<unsigned int> sphereInds;
    generateSphere(sphereVerts, sphereInds, 0.5f, 32, 32);
    sphereIndexCount = (int)sphereInds.size();

    glGenVertexArrays(1, &sphereVAO);
    glGenBuffers(1, &sphereVBO);
    glGenBuffers(1, &sphereEBO);
    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, sphereVerts.size() * sizeof(float), sphereVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereInds.size() * sizeof(unsigned int), sphereInds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // »Ì«‰«  «·√”ÿÊ«‰…
    std::vector<float> cylVerts;
    std::vector<unsigned int> cylInds;
    generateCylinder(cylVerts, cylInds, 0.3f, 1.0f, 16);
    cylinderIndexCount = (int)cylInds.size();

    glGenVertexArrays(1, &cylinderVAO);
    glGenBuffers(1, &cylinderVBO);
    glGenBuffers(1, &cylinderEBO);
    glBindVertexArray(cylinderVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinderVBO);
    glBufferData(GL_ARRAY_BUFFER, cylVerts.size() * sizeof(float), cylVerts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinderEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylInds.size() * sizeof(unsigned int), cylInds.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    initGame();
    srand((unsigned int)time(0));

    std::cout << "\n\033[1;35m========================================" << std::endl;
    std::cout << "      MARIO 3D - COLLECT 5 COINS         " << std::endl;
    std::cout << "========================================\033[0m" << std::endl;
    std::cout << "Collect 5 coins to WIN!" << std::endl;
    std::cout << "WASD: Move | SPACE: Jump" << std::endl;
    std::cout << "Jump on brown platforms to reach coins!" << std::endl;
    std::cout << "R: Restart | ESC: Exit" << std::endl;
    std::cout << "========================================\n" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // ===== INPUT =====
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            initGame();

        if (gameState == PLAYING) {
            float speed = PLAYER_SPEED * deltaTime;
            glm::vec3 moveDir(0.0f);

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
                moveDir.z -= 1.0f;
                player.rotation = 180.0f;
                player.walkCycle += deltaTime * 8.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
                moveDir.z += 1.0f;
                player.rotation = 0.0f;
                player.walkCycle += deltaTime * 8.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                moveDir.x -= 1.0f;
                player.rotation = 90.0f;
                player.walkCycle += deltaTime * 8.0f;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                moveDir.x += 1.0f;
                player.rotation = -90.0f;
                player.walkCycle += deltaTime * 8.0f;
            }

            if (glm::length(moveDir) > 0.0f) {
                moveDir = glm::normalize(moveDir);
                player.velocity.x = moveDir.x * speed / deltaTime;
                player.velocity.z = moveDir.z * speed / deltaTime;
            }
            else {
                player.velocity.x *= 0.8f;
                player.velocity.z *= 0.8f;
            }

            // ﬁ›“
            if (player.onGround && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                player.velocity.y = JUMP_FORCE;
                player.onGround = false;
            }

            // Ã«–»Ì…
            player.velocity.y += GRAVITY * deltaTime;

            //  ÕœÌÀ «·„Êﬁ⁄
            glm::vec3 newPos = player.position;
            newPos.x += player.velocity.x * deltaTime;
            newPos.z += player.velocity.z * deltaTime;
            newPos.y += player.velocity.y * deltaTime;

            //  ’«œ„
            player.onGround = false;
            glm::vec3 playerSize = glm::vec3(0.8f, 1.6f, 0.8f);

            for (auto& plat : platforms) {
                if (checkCollision(glm::vec3(newPos.x, player.position.y, player.position.z), playerSize, plat.position, plat.size)) {
                    if (player.velocity.x > 0) newPos.x = plat.position.x - plat.size.x / 2 - playerSize.x / 2;
                    else if (player.velocity.x < 0) newPos.x = plat.position.x + plat.size.x / 2 + playerSize.x / 2;
                    player.velocity.x = 0;
                }
                if (checkCollision(glm::vec3(player.position.x, player.position.y, newPos.z), playerSize, plat.position, plat.size)) {
                    if (player.velocity.z > 0) newPos.z = plat.position.z - plat.size.z / 2 - playerSize.z / 2;
                    else if (player.velocity.z < 0) newPos.z = plat.position.z + plat.size.z / 2 + playerSize.z / 2;
                    player.velocity.z = 0;
                }
            }

            for (auto& plat : platforms) {
                if (checkCollision(glm::vec3(newPos.x, newPos.y, newPos.z), playerSize, plat.position, plat.size)) {
                    if (player.velocity.y < 0) {
                        newPos.y = plat.position.y + plat.size.y / 2 + playerSize.y / 2;
                        player.velocity.y = 0;
                        player.onGround = true;
                    }
                    else if (player.velocity.y > 0) {
                        newPos.y = plat.position.y - plat.size.y / 2 - playerSize.y / 2;
                        player.velocity.y = 0;
                    }
                }
            }

            player.position = newPos;

            // ÕœÊœ
            player.position.x = glm::clamp(player.position.x, -9.0f, 9.0f);
            player.position.z = glm::clamp(player.position.z, -13.0f, 13.0f);

            if (player.position.y < -5.0f) {
                gameState = LOSE;
            }

            // Ã„⁄ «·⁄„·« 
            for (auto& coin : coins) {
                if (coin.active) {
                    coin.rotation += deltaTime * 120.0f;
                    float bob = sinf(currentFrame * 3.0f + coin.bobOffset) * 0.1f;
                    glm::vec3 originalPos = coin.position;
                    coin.position.y += bob * deltaTime;

                    if (checkCollision(player.position, playerSize, coin.position, glm::vec3(0.5f))) {
                        coin.active = false;
                        score++;
                        std::cout << "\n\033[1;33mCoin collected! Score: " << score << "/" << WIN_SCORE << "\033[0m" << std::endl;
                        if (score >= WIN_SCORE) {
                            gameState = WIN;
                        }
                    }
                }
            }

            // «·√⁄œ«¡
            for (auto& enemy : enemies) {
                if (!enemy.active) continue;

                enemy.patrolTimer += deltaTime;
                if (enemy.patrolTimer > 2.0f) {
                    enemy.velocity.x = -enemy.velocity.x;
                    enemy.patrolTimer = 0.0f;
                }

                enemy.position.x += enemy.velocity.x * deltaTime;

                if (checkCollision(player.position, playerSize, enemy.position, glm::vec3(0.9f, 0.6f, 0.9f))) {
                    if (player.velocity.y < 0 && player.position.y > enemy.position.y) {
                        enemy.active = false;
                        player.velocity.y = JUMP_FORCE * 0.7f;
                        std::cout << "\n\033[1;32mEnemy defeated!\033[0m" << std::endl;
                    }
                    else {
                        gameState = LOSE;
                    }
                }
            }

            gameTimer -= deltaTime;
            if (gameTimer <= 0.0f) {
                gameState = LOSE;
            }
        }

        //  ÕœÌÀ «·ﬂ«„Ì—«
        cameraPos = player.position + glm::vec3(0.0f, 6.0f, 12.0f);
        cameraTarget = player.position + glm::vec3(0.0f, 1.0f, 0.0f);

        // ===== RENDER =====
        glClearColor(0.4f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, cameraUp);

        glUseProgram(shaderProgram);
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(5.0f, 15.0f, 10.0f)));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

        // —”„ «·„‰’« 
        for (auto& plat : platforms) {
            renderCube(plat.position, plat.size, plat.color);
        }

        // —”„ «·⁄„·« 
        for (auto& coin : coins) {
            if (coin.active) {
                renderSphere(coin.position, 0.35f, glm::vec3(1.0f, 0.85f, 0.1f));
            }
        }

        // —”„ «·√⁄œ«¡
        for (auto& enemy : enemies) {
            if (enemy.active) {
                renderSphere(enemy.position, 0.5f, glm::vec3(0.2f, 0.8f, 0.2f));
                renderSphere(enemy.position + glm::vec3(0.25f, 0.15f, 0.3f), 0.12f, glm::vec3(1.0f));
                renderSphere(enemy.position + glm::vec3(-0.25f, 0.15f, 0.3f), 0.12f, glm::vec3(1.0f));
            }
        }

        // —”„ „«—ÌÊ
        renderSphere(player.position + glm::vec3(0.0f, 1.3f, 0.0f), 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));
        renderSphere(player.position + glm::vec3(0.0f, 0.95f, 0.0f), 0.28f, glm::vec3(1.0f, 0.8f, 0.6f));
        renderCylinder(player.position + glm::vec3(0.0f, 0.45f, 0.0f), 0.35f, 0.9f, glm::vec3(1.0f, 0.1f, 0.1f));
        renderCylinder(player.position + glm::vec3(0.0f, 0.0f, 0.0f), 0.3f, 0.5f, glm::vec3(0.1f, 0.3f, 0.8f));

        float legSwing = sinf(player.walkCycle) * 0.2f;
        renderCylinder(player.position + glm::vec3(-0.18f, -0.2f, legSwing), 0.1f, 0.4f, glm::vec3(0.1f, 0.2f, 0.6f));
        renderCylinder(player.position + glm::vec3(0.18f, -0.2f, -legSwing), 0.1f, 0.4f, glm::vec3(0.1f, 0.2f, 0.6f));

        renderHUD();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteVertexArrays(1, &sphereVAO);
    glDeleteBuffers(1, &sphereVBO);
    glDeleteBuffers(1, &sphereEBO);
    glDeleteVertexArrays(1, &cylinderVAO);
    glDeleteBuffers(1, &cylinderVBO);
    glDeleteBuffers(1, &cylinderEBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}