#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"

using namespace std;

// ---------- STRUCTS ----------
struct MeshData {
    float* vertices;
    int vertexCount;
};

struct Objeto {
    GLuint VAO;
    int vertexCount;

    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

// ---------- GLOBAL ----------
Objeto objetos[4];
int totalObjetos = 3;
int selecionado = 0;

enum Modo { ROTATE, TRANSLATE, SCALE };
Modo modoAtual = ROTATE;

bool perspective = true;

// câmera
Camera camera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// ---------- SHADERS ----------
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 overrideColor;

out vec4 finalColor;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    finalColor = vec4(color * overrideColor, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec4 finalColor;
out vec4 color;

void main() {
    color = finalColor;
}
)";

// ---------- LOADER OBJ ----------
MeshData loadOBJ(const char* path)
{
    namespace fs = std::filesystem;
    fs::path objFile = fs::path(__FILE__).parent_path() / path;

    ifstream file(objFile);
    if (!file.is_open()) {
        cout << "Erro ao abrir OBJ: " << objFile.string() << endl;
        return { nullptr, 0 };
    }

    vector<glm::vec3> tempV;
    vector<glm::vec3> tempVN;
    vector<int> vIndex;
    vector<int> vnIndex;

    auto parseIndex = [&](const string& token, int size)->int {
        if (token.empty()) return -1;
        int index = stoi(token);
        if (index < 0)
            index = size + index;
        else
            index -= 1;
        return index;
    };

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#')
            continue;

        stringstream ss(line);
        string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            tempV.push_back(vertex);
        }
        else if (type == "vn") {
            glm::vec3 normal;
            ss >> normal.x >> normal.y >> normal.z;
            tempVN.push_back(normal);
        }
        else if (type == "f") {
            vector<string> faceVerts;
            string vertexToken;
            while (ss >> vertexToken) {
                faceVerts.push_back(vertexToken);
            }

            if (faceVerts.size() < 3)
                continue;

            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                array<string, 3> tri = { faceVerts[0], faceVerts[i], faceVerts[i + 1] };
                for (auto& token : tri) {
                    string a, b, c;
                    stringstream vs(token);
                    getline(vs, a, '/');
                    getline(vs, b, '/');
                    getline(vs, c, '/');

                    vIndex.push_back(parseIndex(a, (int)tempV.size()));
                    vnIndex.push_back(parseIndex(c, (int)tempVN.size()));
                }
            }
        }
    }

    int indexCount = (int)vIndex.size();
    float* vertices = new float[indexCount * 6];
    int idx = 0;

    for (int i = 0; i < indexCount; i++) {
        int vi = vIndex[i];
        if (vi < 0 || vi >= (int)tempV.size())
            continue;

        glm::vec3 position = tempV[vi];
        vertices[idx++] = position.x;
        vertices[idx++] = position.y;
        vertices[idx++] = position.z;

        int ni = vnIndex[i];
        if (ni >= 0 && ni < (int)tempVN.size()) {
            glm::vec3 normal = glm::abs(tempVN[ni]);
            vertices[idx++] = normal.x;
            vertices[idx++] = normal.y;
            vertices[idx++] = normal.z;
        } else {
            vertices[idx++] = 0.8f;
            vertices[idx++] = 0.8f;
            vertices[idx++] = 0.8f;
        }
    }

    return { vertices, indexCount };
}

// ---------- VAO ----------
GLuint createVAO(MeshData mesh)
{
    GLuint VAO, VBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertexCount * 6 * sizeof(float), mesh.vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return VAO;
}

// ---------- INPUT ----------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
        selecionado = (selecionado + 1) % totalObjetos;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        modoAtual = ROTATE;

    if (key == GLFW_KEY_T && action == GLFW_PRESS)
        modoAtual = TRANSLATE;

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        modoAtual = SCALE;

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        perspective = !perspective;
}

// ---------- SHADER ----------
GLuint createShader()
{
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return prog;
}

// ---------- MAIN ----------
int main()
{
    if (!glfwInit()) {
        cout << "Falha ao inicializar GLFW" << endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Final OBJ Viewer", NULL, NULL);
    if (!window) {
        cout << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetKeyCallback(window, key_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_DEPTH_TEST);

    GLuint shader = createShader();
    glUseProgram(shader);

    MeshData m1 = loadOBJ("obj1.obj");
    MeshData m2 = loadOBJ("obj2.obj");
    MeshData m3 = loadOBJ("obj3.obj");

    objetos[0].VAO = createVAO(m1);
    objetos[0].vertexCount = m1.vertexCount;

    objetos[1].VAO = createVAO(m2);
    objetos[1].vertexCount = m2.vertexCount;

    objetos[2].VAO = createVAO(m3);
    objetos[2].vertexCount = m3.vertexCount;

    float spacing = 3.5f;
    objetos[0].pos = glm::vec3(-spacing, 0.0f, 0.0f);
    objetos[1].pos = glm::vec3(0.0f, 0.0f, 0.0f);
    objetos[2].pos = glm::vec3(spacing, 0.0f, 0.0f);
    for (int i = 0; i < totalObjetos; i++) {
        objetos[i].rot = glm::vec3(0);
        objetos[i].scale = glm::vec3(1);
    }

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        glClearColor(0.1f,0.1f,0.1f,1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (glfwGetKey(window, GLFW_KEY_W)) camera.processKeyboard("FORWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S)) camera.processKeyboard("BACKWARD", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A)) camera.processKeyboard("LEFT", deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D)) camera.processKeyboard("RIGHT", deltaTime);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = perspective ?
            glm::perspective(glm::radians(45.0f), 800.f/600.f, 0.1f, 100.f) :
            glm::ortho(-3.f,3.f,-3.f,3.f,0.1f,100.f);

        glUniformMatrix4fv(glGetUniformLocation(shader,"view"),1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader,"projection"),1,GL_FALSE,glm::value_ptr(projection));

        Objeto& obj = objetos[selecionado];
        float speed = 2.0f * deltaTime;

        if (modoAtual == ROTATE) {
            if (glfwGetKey(window, GLFW_KEY_X)) obj.rot.x += speed;
            if (glfwGetKey(window, GLFW_KEY_Y)) obj.rot.y += speed;
            if (glfwGetKey(window, GLFW_KEY_Z)) obj.rot.z += speed;
        }

        if (modoAtual == TRANSLATE) {
            if (glfwGetKey(window, GLFW_KEY_I)) obj.pos.y += speed;
            if (glfwGetKey(window, GLFW_KEY_K)) obj.pos.y -= speed;
            if (glfwGetKey(window, GLFW_KEY_J)) obj.pos.x -= speed;
            if (glfwGetKey(window, GLFW_KEY_L)) obj.pos.x += speed;
            if (glfwGetKey(window, GLFW_KEY_U)) obj.pos.z += speed;
            if (glfwGetKey(window, GLFW_KEY_O)) obj.pos.z -= speed;
        }

        if (modoAtual == SCALE) {
            if (glfwGetKey(window, GLFW_KEY_UP)) obj.scale += glm::vec3(speed);
            if (glfwGetKey(window, GLFW_KEY_DOWN)) obj.scale -= glm::vec3(speed);
        }

        for (int i = 0; i < totalObjetos; i++)
        {
            glm::mat4 model = glm::mat4(1);

            model = glm::translate(model, objetos[i].pos);
            model = glm::rotate(model, objetos[i].rot.x, glm::vec3(1,0,0));
            model = glm::rotate(model, objetos[i].rot.y, glm::vec3(0,1,0));
            model = glm::rotate(model, objetos[i].rot.z, glm::vec3(0,0,1));
            model = glm::scale(model, objetos[i].scale);

            glUniformMatrix4fv(glGetUniformLocation(shader,"model"),1,GL_FALSE,glm::value_ptr(model));

            if (i == selecionado)
                glUniform3f(glGetUniformLocation(shader,"overrideColor"),1,0.3,0.3);
            else
                glUniform3f(glGetUniformLocation(shader,"overrideColor"),1,1,1);

            glBindVertexArray(objetos[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, objetos[i].vertexCount);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}