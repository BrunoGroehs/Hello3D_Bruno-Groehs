#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Camera.h>

struct SceneObject {
    GLuint VAO;
    int nVertices;
    GLuint texID;
    int useTexture;
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float ns;
    glm::vec3 position;
    glm::vec3 rotation;
    float scale;

    string trajectoryFile;
    vector<glm::vec3> trajectoryPoints;
    float trajectoryT;
    float trajectorySpeed;
    bool trajectoryActive;
};

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

int setupShader();
int loadSimpleOBJ(string filePath, int &nVertices, string &mtlFileName);
string loadMTL(string filePath, string &textureFileName, glm::vec3 &ka, glm::vec3 &kd, glm::vec3 &ks, float &ns);
GLuint loadTexture(string filePath);
bool loadScene(const string &filePath, vector<SceneObject> &objects);
bool loadTrajectory(const string &filePath, vector<glm::vec3> &points);
bool saveTrajectory(const string &filePath, const vector<glm::vec3> &points);
glm::vec3 bezierPoint(const vector<glm::vec3> &pts, int base, float t);

const GLuint WIDTH = 1000, HEIGHT = 800;

const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout (location = 2) in vec2 texCoord;\n"
"layout (location = 3) in vec3 normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec2 texCoordFrag;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(position, 1.0);\n"
"    FragPos = vec3(model * vec4(position, 1.0));\n"
"    Normal = mat3(transpose(inverse(model))) * normal;\n"
"    texCoordFrag = texCoord;\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 330\n"
"in vec2 texCoordFrag;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"uniform sampler2D texBuffer;\n"
"uniform int useTexture;\n"
"uniform vec3 lightPos[3];\n"
"uniform vec3 lightColor[3];\n"
"uniform int lightEnable[3];\n"
"uniform float lightIntensity;\n"
"uniform vec3 viewPos;\n"
"uniform vec3 ka;\n"
"uniform vec3 kd;\n"
"uniform vec3 ks;\n"
"uniform float ns;\n"
"uniform int isSelected;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    vec3 ambient = ka * 0.3;\n"
"    vec3 directLighting = vec3(0.0);\n"
"    for(int i = 0; i < 3; i++) {\n"
"        if (lightEnable[i] == 0) continue;\n"
"        vec3 lightDir = normalize(lightPos[i] - FragPos);\n"
"        float diff = max(dot(norm, lightDir), 0.0);\n"
"        vec3 diffuse = kd * diff * lightColor[i] * lightIntensity;\n"
"        vec3 reflectDir = reflect(-lightDir, norm);\n"
"        float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(ns, 8.0));\n"
"        vec3 specular = ks * spec * lightColor[i] * lightIntensity;\n"
"        directLighting += diffuse + specular;\n"
"    }\n"
"    vec3 objColor;\n"
"    if (useTexture == 1) {\n"
"        objColor = texture(texBuffer, texCoordFrag).rgb;\n"
"    } else {\n"
"        objColor = vec3(1.0);\n"
"    }\n"
"    vec3 result = (ambient + directLighting) * objColor;\n"
"    if (isSelected == 1) {\n"
"        result = mix(result, vec3(1.0, 0.6, 0.1), 0.4);\n"
"    }\n"
"    color = vec4(result, 1.0);\n"
"}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

bool enableKeyLight = true;
bool enableFillLight = true;
bool enableBackLight = true;
float lightIntensity = 1.0f;

bool showTexture = true;

Camera camera(glm::vec3(0.0f, 0.0f, 5.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

vector<SceneObject> sceneObjects;
int selectedObject = 0;

const string sceneFilePath = "Modelos3D/scene.txt";

bool loadTrajectory(const string &filePath, vector<glm::vec3> &points)
{
    ifstream arq(filePath.c_str());
    if (!arq.is_open()) return false;
    vector<glm::vec3> tmp;
    string line;
    while (getline(arq, line))
    {
        size_t pos = line.find_first_not_of(" \t\r\n");
        if (pos == string::npos) continue;
        if (line[pos] == '#') continue;
        istringstream ss(line);
        glm::vec3 p;
        if (ss >> p.x >> p.y >> p.z)
            tmp.push_back(p);
    }
    arq.close();
    points = tmp;
    return true;
}

bool saveTrajectory(const string &filePath, const vector<glm::vec3> &points)
{
    ofstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        cerr << "Erro ao salvar em " << filePath << endl;
        return false;
    }
    arq << "# Pontos da trajetoria (Bezier cubica)\n";
    for (size_t i = 0; i < points.size(); i++)
        arq << points[i].x << " " << points[i].y << " " << points[i].z << "\n";
    arq.close();
    cout << "Salvo: " << filePath << " (" << points.size() << " pontos)" << endl;
    return true;
}

glm::vec3 bezierPoint(const vector<glm::vec3> &pts, int base, float t)
{
    int n = (int)pts.size();
    glm::vec3 p0 = pts[base % n];
    glm::vec3 p1 = pts[(base + 1) % n];
    glm::vec3 p2 = pts[(base + 2) % n];
    glm::vec3 p3 = pts[(base + 3) % n];
    float u = 1.0f - t;
    return u*u*u*p0 + 3.0f*u*u*t*p1 + 3.0f*u*t*t*p2 + t*t*t*p3;
}

bool loadScene(const string &filePath, vector<SceneObject> &objects)
{
    ifstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        cerr << "Nao foi possivel abrir: " << filePath << endl;
        return false;
    }
    objects.clear();
    string line;
    int idx = 0;
    while (getline(arq, line))
    {
        size_t pos = line.find_first_not_of(" \t\r\n");
        if (pos == string::npos) continue;
        if (line[pos] == '#') continue;

        istringstream ss(line);
        string objPath;
        glm::vec3 t, r;
        float s;
        string trajFile = "";
        if (!(ss >> objPath >> t.x >> t.y >> t.z >> r.x >> r.y >> r.z >> s))
            continue;
        ss >> trajFile;

        SceneObject o;
        int nVertices = 0;
        string mtlName;
        o.VAO = loadSimpleOBJ(objPath, nVertices, mtlName);
        if ((int)o.VAO == -1) { idx++; continue; }
        o.nVertices = nVertices;
        o.texID = 0;
        o.useTexture = 0;
        o.ka = glm::vec3(0.3f);
        o.kd = glm::vec3(0.8f);
        o.ks = glm::vec3(0.5f);
        o.ns = 32.0f;

        string modelDir = objPath.substr(0, objPath.find_last_of('/') + 1);
        if (!mtlName.empty())
        {
            string textureFileName;
            loadMTL(modelDir + mtlName, textureFileName, o.ka, o.kd, o.ks, o.ns);
            if (!textureFileName.empty())
            {
                o.texID = loadTexture(modelDir + textureFileName);
                if (o.texID != 0) o.useTexture = 1;
            }
        }

        o.position = t;
        o.rotation = r;
        o.scale = s;
        o.trajectoryT = 0.0f;
        o.trajectorySpeed = 0.3f;
        o.trajectoryActive = true;

        if (trajFile.empty() || trajFile == "-")
            o.trajectoryFile = "Modelos3D/traj_" + to_string(idx) + ".txt";
        else
            o.trajectoryFile = trajFile;

        loadTrajectory(o.trajectoryFile, o.trajectoryPoints);

        cout << "[" << idx << "] " << objPath
             << " pos=(" << t.x << "," << t.y << "," << t.z << ")"
             << " trajetoria=" << o.trajectoryFile
             << " (" << o.trajectoryPoints.size() << " pts)" << endl;

        objects.push_back(o);
        idx++;
    }
    arq.close();
    cout << "Cena: " << objects.size() << " objetos" << endl;
    return true;
}

int main()
{
    glfwInit();

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Diorama3D - Bruno Groehs", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    GLuint shaderID = setupShader();

    if (!loadScene(sceneFilePath, sceneObjects))
        return -1;

    glUseProgram(shaderID);

    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc  = glGetUniformLocation(shaderID, "view");
    GLint projLoc  = glGetUniformLocation(shaderID, "projection");
    GLint texLoc   = glGetUniformLocation(shaderID, "texBuffer");
    GLint useTexLoc = glGetUniformLocation(shaderID, "useTexture");
    GLint kaLoc = glGetUniformLocation(shaderID, "ka");
    GLint kdLoc = glGetUniformLocation(shaderID, "kd");
    GLint ksLoc = glGetUniformLocation(shaderID, "ks");
    GLint nsLoc = glGetUniformLocation(shaderID, "ns");
    GLint selLoc = glGetUniformLocation(shaderID, "isSelected");

    glm::vec3 lightPositions[] = {
        glm::vec3( 3.0f,  3.0f,  3.0f),
        glm::vec3(-3.0f,  2.0f,  2.0f),
        glm::vec3( 0.0f,  3.0f, -3.0f)
    };
    glm::vec3 lightColors[] = {
        glm::vec3(1.0f, 0.95f, 0.85f),
        glm::vec3(0.6f, 0.65f, 0.8f),
        glm::vec3(0.5f, 0.7f, 1.0f)
    };

    glUniform3fv(glGetUniformLocation(shaderID, "lightPos"),   3, glm::value_ptr(lightPositions[0]));
    glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 3, glm::value_ptr(lightColors[0]));
    glUniform1i(texLoc, 0);

    glEnable(GL_DEPTH_TEST);

    cout << "\n=== Diorama3D ===" << endl;
    cout << "Camera:    WASD + mouse | scroll: zoom" << endl;
    cout << "Selecao:   TAB" << endl;
    cout << "Mover:     setas (XY) | PgUp/PgDown ou I/K (Z) | [ ]: escala" << endl;
    cout << "Rotacao:   X / Y / Z (toggle)" << endl;
    cout << "Materiais: M (textura on/off)" << endl;
    cout << "Luzes:     1/2/3 (toggle) | + / - (intensidade)" << endl;
    cout << "Trajeto:   T anima | P add ponto | O salva | L recarrega | C limpa | , .: velocidade" << endl;
    cout << "ESC: sair\n" << endl;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glfwPollEvents();

        glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float angle = (GLfloat)glfwGetTime();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        glm::mat4 view = camera.GetViewMatrix();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.Position));

        int lightEnables[] = {enableKeyLight ? 1 : 0, enableFillLight ? 1 : 0, enableBackLight ? 1 : 0};
        glUniform1iv(glGetUniformLocation(shaderID, "lightEnable"), 3, lightEnables);
        glUniform1f(glGetUniformLocation(shaderID, "lightIntensity"), lightIntensity);

        for (int i = 0; i < (int)sceneObjects.size(); i++)
        {
            SceneObject &o = sceneObjects[i];

            glm::vec3 trajPos(0.0f);
            if (o.trajectoryPoints.size() >= 4)
            {
                int n = (int)o.trajectoryPoints.size();
                if (o.trajectoryActive)
                {
                    o.trajectoryT += o.trajectorySpeed * deltaTime;
                    while (o.trajectoryT >= (float)n) o.trajectoryT -= (float)n;
                }
                int base = (int)o.trajectoryT;
                float t = o.trajectoryT - (float)base;
                trajPos = bezierPoint(o.trajectoryPoints, base, t);
            }

            int useTex = (showTexture && o.useTexture) ? 1 : 0;
            glUniform1i(useTexLoc, useTex);
            glUniform3fv(kaLoc, 1, glm::value_ptr(o.ka));
            glUniform3fv(kdLoc, 1, glm::value_ptr(o.kd));
            glUniform3fv(ksLoc, 1, glm::value_ptr(o.ks));
            glUniform1f(nsLoc, o.ns);
            glUniform1i(selLoc, (i == selectedObject) ? 1 : 0);

            if (useTex == 1)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, o.texID);
            }

            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, o.position + trajPos);

            if (i == selectedObject)
            {
                if (rotateX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
                else if (rotateY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
                else if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
            }

            model = glm::rotate(model, glm::radians(o.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(o.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(o.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(o.scale));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(o.VAO);
            glDrawArrays(GL_TRIANGLES, 0, o.nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    for (auto &o : sceneObjects)
    {
        glDeleteVertexArrays(1, &o.VAO);
        if (o.texID) glDeleteTextures(1, &o.texID);
    }
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && !sceneObjects.empty())
    {
        selectedObject = (selectedObject + 1) % (int)sceneObjects.size();
        cout << "Objeto selecionado: " << selectedObject << endl;
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS) { rotateX = !rotateX; rotateY = false; rotateZ = false; }
    if (key == GLFW_KEY_Y && action == GLFW_PRESS) { rotateX = false; rotateY = !rotateY; rotateZ = false; }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { rotateX = false; rotateY = false; rotateZ = !rotateZ; }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) { enableKeyLight  = !enableKeyLight;  cout << "Key Light: "  << (enableKeyLight  ? "ON" : "OFF") << endl; }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) { enableFillLight = !enableFillLight; cout << "Fill Light: " << (enableFillLight ? "ON" : "OFF") << endl; }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) { enableBackLight = !enableBackLight; cout << "Back Light: " << (enableBackLight ? "ON" : "OFF") << endl; }

    if ((key == GLFW_KEY_KP_ADD || key == GLFW_KEY_EQUAL) && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        lightIntensity = glm::min(3.0f, lightIntensity + 0.1f);
        cout << "Intensidade: " << lightIntensity << endl;
    }
    if ((key == GLFW_KEY_KP_SUBTRACT || key == GLFW_KEY_MINUS) && (action == GLFW_PRESS || action == GLFW_REPEAT))
    {
        lightIntensity = glm::max(0.0f, lightIntensity - 0.1f);
        cout << "Intensidade: " << lightIntensity << endl;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        showTexture = !showTexture;
        cout << "Textura: " << (showTexture ? "ON" : "OFF") << endl;
    }

    if (selectedObject >= 0 && selectedObject < (int)sceneObjects.size())
    {
        SceneObject &o = sceneObjects[selectedObject];

        if (key == GLFW_KEY_LEFT_BRACKET  && (action == GLFW_PRESS || action == GLFW_REPEAT)) o.scale = glm::max(0.05f, o.scale - 0.05f);
        if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) o.scale += 0.05f;

        if (key == GLFW_KEY_T && action == GLFW_PRESS)
        {
            o.trajectoryActive = !o.trajectoryActive;
            cout << "Trajetoria obj " << selectedObject << ": " << (o.trajectoryActive ? "ON" : "OFF") << endl;
        }

        if (key == GLFW_KEY_P && action == GLFW_PRESS)
        {
            glm::vec3 worldPoint = camera.Position + camera.Front * 2.5f;
            glm::vec3 localPoint = worldPoint - o.position;
            o.trajectoryPoints.push_back(localPoint);
            cout << "Ponto adicionado obj " << selectedObject
                 << " local=(" << localPoint.x << "," << localPoint.y << "," << localPoint.z << ")"
                 << " total=" << o.trajectoryPoints.size() << endl;
        }
        if (key == GLFW_KEY_O && action == GLFW_PRESS)
        {
            saveTrajectory(o.trajectoryFile, o.trajectoryPoints);
            o.trajectoryT = 0.0f;
        }
        if (key == GLFW_KEY_L && action == GLFW_PRESS)
        {
            loadTrajectory(o.trajectoryFile, o.trajectoryPoints);
            o.trajectoryT = 0.0f;
            cout << "Recarregado " << o.trajectoryFile << " (" << o.trajectoryPoints.size() << " pts)" << endl;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            o.trajectoryPoints.clear();
            o.trajectoryT = 0.0f;
            cout << "Trajetoria obj " << selectedObject << " limpa" << endl;
        }
        if (key == GLFW_KEY_COMMA && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            o.trajectorySpeed = glm::max(0.05f, o.trajectorySpeed - 0.05f);
            cout << "Velocidade obj " << selectedObject << ": " << o.trajectorySpeed << endl;
        }
        if (key == GLFW_KEY_PERIOD && (action == GLFW_PRESS || action == GLFW_REPEAT))
        {
            o.trajectorySpeed += 0.05f;
            cout << "Velocidade obj " << selectedObject << ": " << o.trajectorySpeed << endl;
        }
    }
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    if (selectedObject >= 0 && selectedObject < (int)sceneObjects.size())
    {
        float step = 2.0f * deltaTime;
        SceneObject &o = sceneObjects[selectedObject];
        if (glfwGetKey(window, GLFW_KEY_RIGHT)     == GLFW_PRESS) o.position.x += step;
        if (glfwGetKey(window, GLFW_KEY_LEFT)      == GLFW_PRESS) o.position.x -= step;
        if (glfwGetKey(window, GLFW_KEY_UP)        == GLFW_PRESS) o.position.y += step;
        if (glfwGetKey(window, GLFW_KEY_DOWN)      == GLFW_PRESS) o.position.y -= step;
        if (glfwGetKey(window, GLFW_KEY_PAGE_UP)   == GLFW_PRESS) o.position.z -= step;
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) o.position.z += step;
        if (glfwGetKey(window, GLFW_KEY_I)         == GLFW_PRESS) o.position.z -= step;
        if (glfwGetKey(window, GLFW_KEY_K)         == GLFW_PRESS) o.position.z += step;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[1024];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 1024, NULL, infoLog);
        cout << "ERROR::VERTEX::" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 1024, NULL, infoLog);
        cout << "ERROR::FRAGMENT::" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 1024, NULL, infoLog);
        cout << "ERROR::LINK::" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

int loadSimpleOBJ(string filePath, int &nVertices, string &mtlFileName)
{
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<glm::vec3> normals;
    vector<GLfloat> vBuffer;
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

    ifstream arqEntrada(filePath.c_str());
    if (!arqEntrada.is_open())
    {
        cerr << "Erro ao ler " << filePath << endl;
        return -1;
    }

    string line;
    while (getline(arqEntrada, line))
    {
        istringstream ssline(line);
        string word;
        ssline >> word;

        if (word == "mtllib") ssline >> mtlFileName;
        else if (word == "v")
        {
            glm::vec3 v;
            ssline >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (word == "vt")
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        }
        else if (word == "vn")
        {
            glm::vec3 vn;
            ssline >> vn.x >> vn.y >> vn.z;
            normals.push_back(vn);
        }
        else if (word == "f")
        {
            while (ssline >> word)
            {
                int vi = 0, ti = -1, ni = -1;
                istringstream ss(word);
                string index;

                if (getline(ss, index, '/')) vi = !index.empty() ? stoi(index) - 1 : 0;
                if (getline(ss, index, '/')) ti = !index.empty() ? stoi(index) - 1 : -1;
                if (getline(ss, index))      ni = !index.empty() ? stoi(index) - 1 : -1;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);

                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);

                if (ti >= 0 && ti < (int)texCoords.size())
                {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                }
                else
                {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }

                if (ni >= 0 && ni < (int)normals.size())
                {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                }
                else
                {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(1.0f);
                }
            }
        }
    }

    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 11;
    return VAO;
}

string loadMTL(string filePath, string &textureFileName, glm::vec3 &ka, glm::vec3 &kd, glm::vec3 &ks, float &ns)
{
    string materialName;
    ifstream arq(filePath.c_str());
    if (!arq.is_open())
    {
        cerr << "Erro ao ler MTL " << filePath << endl;
        return materialName;
    }

    string line;
    while (getline(arq, line))
    {
        istringstream ss(line);
        string word;
        ss >> word;

        if (word == "newmtl") ss >> materialName;
        else if (word == "map_Kd") ss >> textureFileName;
        else if (word == "Ka") ss >> ka.r >> ka.g >> ka.b;
        else if (word == "Kd") ss >> kd.r >> kd.g >> kd.b;
        else if (word == "Ks") ss >> ks.r >> ks.g >> ks.b;
        else if (word == "Ns") ss >> ns;
    }
    arq.close();
    return materialName;
}

GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }
    else
    {
        cerr << "Falha ao carregar textura: " << filePath << endl;
        glDeleteTextures(1, &texID);
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}
