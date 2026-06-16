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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

int setupShader();
int loadSimpleOBJ(string filePath, int &nVertices, string &mtlFileName);
string loadMTL(string filePath, string &textureFileName, glm::vec3 &ka, glm::vec3 &kd, glm::vec3 &ks, float &ns);
GLuint loadTexture(string filePath);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// Cada vértice: posição (x, y, z), cor (r, g, b) e coordenada de textura (s, t) -> 8 floats.
const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout (location = 2) in vec2 texCoord;\n"
"layout (location = 3) in vec3 normal;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"out vec4 finalColor;\n"
"out vec2 texCoordFrag;\n"
"out vec3 Normal;\n"
"out vec3 FragPos;\n"
"void main()\n"
"{\n"
"    gl_Position = projection * view * model * vec4(position, 1.0);\n"
"    FragPos = vec3(model * vec4(position, 1.0));\n"
"    Normal = mat3(transpose(inverse(model))) * normal;\n"
"    finalColor = vec4(color, 1.0);\n"
"    texCoordFrag = texCoord;\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 330\n"
"in vec4 finalColor;\n"
"in vec2 texCoordFrag;\n"
"in vec3 Normal;\n"
"in vec3 FragPos;\n"
"uniform sampler2D texBuffer;\n"
"uniform int useTexture;\n"
"uniform vec3 lightPos[3];\n"
"uniform vec3 lightColor[3];\n"
"uniform int lightEnable[3];\n"
"uniform vec3 viewPos;\n"
"uniform vec3 ka;\n"
"uniform vec3 kd;\n"
"uniform vec3 ks;\n"
"uniform float ns;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"    vec3 norm = normalize(Normal);\n"
"    vec3 viewDir = normalize(viewPos - FragPos);\n"
"    vec3 totalLighting = vec3(0.0);\n"
"    for(int i = 0; i < 3; i++) {\n"
"        if (lightEnable[i] == 0) continue;\n"
"        float distance = length(lightPos[i] - FragPos);\n"
"        // Fator de atenuação para a luz pontual\n"
"        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance));\n"
"        \n"
"        vec3 ambient = 0.2 * ka * lightColor[i];\n"
"        \n"
"        vec3 lightDir = normalize(lightPos[i] - FragPos);\n"
"        float diff = max(dot(norm, lightDir), 0.0);\n"
"        // Atenuação aplicada na parcela difusa e especular\n"
"        vec3 diffuse = kd * diff * lightColor[i] * attenuation;\n"
"        \n"
"        vec3 reflectDir = reflect(-lightDir, norm);\n"
"        float spec = pow(max(dot(viewDir, reflectDir), 0.0), max(ns, 1.0));\n"
"        vec3 specular = ks * spec * lightColor[i] * attenuation;\n"
"        \n"
"        totalLighting += (ambient + diffuse + specular);\n"
"    }\n"
"    vec3 objColor;\n"
"    if (useTexture == 1) {\n"
"        objColor = texture(texBuffer, texCoordFrag).rgb;\n"
"    } else {\n"
"        objColor = finalColor.rgb;\n"
"    }\n"
"    color = vec4(totalLighting * objColor, 1.0);\n"
"}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

// Toggles das luzes pontuais do sistema de 3 pontos
bool enableKeyLight = true;
bool enableFillLight = true;
bool enableBackLight = true;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

// Variáveis para escala
float scaleObj = 1.0f;

// Trajetoria do objeto
vector<glm::vec3> trajectoryPoints;
int currentSegment = 0;
float segmentT = 0.0f;
float trajectorySpeed = 0.5f;
bool trajectoryActive = true;
glm::vec3 objectPosition(0.0f);
const string trajectoryFilePath = "Modelos3D/trajetoria.txt";

bool loadTrajectory(const string &filePath, vector<glm::vec3> &points)
{
	ifstream arq(filePath.c_str());
	if (!arq.is_open())
	{
		cerr << "Nao foi possivel abrir " << filePath << endl;
		return false;
	}
	points.clear();
	string line;
	while (getline(arq, line))
	{
		size_t pos = line.find_first_not_of(" \t\r\n");
		if (pos == string::npos) continue;
		if (line[pos] == '#') continue;

		istringstream ss(line);
		glm::vec3 p;
		if (ss >> p.x >> p.y >> p.z)
			points.push_back(p);
	}
	arq.close();
	cout << "Trajetoria carregada com " << points.size() << " pontos" << endl;
	return true;
}

bool saveTrajectory(const string &filePath, const vector<glm::vec3> &points)
{
	ofstream arq(filePath.c_str());
	if (!arq.is_open())
	{
		cerr << "Erro ao salvar trajetoria" << endl;
		return false;
	}
	arq << "# Pontos da trajetoria\n";
	for (size_t i = 0; i < points.size(); i++)
	{
		arq << points[i].x << " " << points[i].y << " " << points[i].z << "\n";
	}
	arq.close();
	cout << "Trajetoria salva (" << points.size() << " pontos)" << endl;
	return true;
}

int main()
{
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D Texturizado -- Bruno Groehs", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// Traz o mouse capture para uso com a câmera
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return -1;
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	GLuint shaderID = setupShader();

	// Carrega o OBJ (com vt/vn) e o MTL referenciado, e a textura difusa (map_Kd) do MTL.
	string objPath = "Modelos3D/Suzanne.obj";
	string modelDir = "Modelos3D/";

	int nVertices = 0;
	string mtlFileName;
	GLuint VAO = loadSimpleOBJ(objPath, nVertices, mtlFileName);
	if ((int)VAO == -1)
	{
		cout << "Falha ao carregar OBJ" << endl;
		return -1;
	}

	GLuint texID = 0;
	int useTexture = 0;
	glm::vec3 ka(0.2f), kd(0.8f), ks(0.5f);
	float ns = 32.0f;

	if (!mtlFileName.empty())
	{
		string textureFileName;
		loadMTL(modelDir + mtlFileName, textureFileName, ka, kd, ks, ns);
		if (!textureFileName.empty())
		{
			texID = loadTexture(modelDir + textureFileName);
			if (texID != 0) useTexture = 1;
		}
	}

	glUseProgram(shaderID);

	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	GLint viewLoc = glGetUniformLocation(shaderID, "view");
	GLint projLoc = glGetUniformLocation(shaderID, "projection");
	GLint texLoc = glGetUniformLocation(shaderID, "texBuffer");
	GLint useTexLoc = glGetUniformLocation(shaderID, "useTexture");
	
	// Phong uniforms
	// Array de posições das luzes com base no objeto principal (0,0,0)
	glm::vec3 lightPositions[] = {
		glm::vec3(1.5f, 1.5f, 2.0f),   // Key light: Direita, cima, frente -> Luz Principal Intensa
		glm::vec3(-1.5f, 0.5f, 2.0f),  // Fill light: Esquerda, frente (mais baixa) -> Suavizar
		glm::vec3(0.0f, 2.0f, -2.5f)   // Back light: Atrás, cima -> Profundidade/Fundo
	};
	// Intensidades de cores p/ cada luz
	glm::vec3 lightColors[] = {
		glm::vec3(1.0f, 1.0f, 1.0f),   // Key: Mais forte
		glm::vec3(0.4f, 0.4f, 0.4f),   // Fill: Preenchimento Suave
		glm::vec3(0.8f, 0.8f, 0.8f)    // Back: Destacar bordas/fundo
	};

	glUniform3fv(glGetUniformLocation(shaderID, "lightPos"), 3, glm::value_ptr(lightPositions[0]));
	glUniform3fv(glGetUniformLocation(shaderID, "lightColor"), 3, glm::value_ptr(lightColors[0]));
	glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.Position));
	glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(ka));
	glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(kd));
	glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(ks));
	glUniform1f(glGetUniformLocation(shaderID, "ns"), ns);

	glUniform1i(texLoc, 0);
	glUniform1i(useTexLoc, useTexture);

	glEnable(GL_DEPTH_TEST);

	loadTrajectory(trajectoryFilePath, trajectoryPoints);
	if (!trajectoryPoints.empty())
		objectPosition = trajectoryPoints[0];

	cout << "Trajetoria: T=on/off | P=add ponto | O=salvar | L=recarregar | C=limpar | ,/.=velocidade" << endl;

	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// input
		processInput(window);

		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		// pass projection matrix to shader (note that in this case it could change every frame)
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		// camera/view transformation
		glm::mat4 view = camera.GetViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		// atualizar a posição da câmera para o Phong
		glUniform3fv(glGetUniformLocation(shaderID, "viewPos"), 1, glm::value_ptr(camera.Position));

		// Atualiza o estado das luzes pro Shader via uniform
		int lightEnables[] = {enableKeyLight ? 1 : 0, enableFillLight ? 1 : 0, enableBackLight ? 1 : 0};
		glUniform1iv(glGetUniformLocation(shaderID, "lightEnable"), 3, lightEnables);

		if (useTexture == 1)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texID);
		}

		glBindVertexArray(VAO);

		// Atualiza posicao do objeto pela trajetoria (lerp entre pontos, ciclica)
		if (trajectoryActive && trajectoryPoints.size() >= 2)
		{
			int n = (int)trajectoryPoints.size();
			glm::vec3 a = trajectoryPoints[currentSegment];
			glm::vec3 b = trajectoryPoints[(currentSegment + 1) % n];
			float dist = glm::length(b - a);
			if (dist < 0.0001f)
			{
				currentSegment = (currentSegment + 1) % n;
				segmentT = 0.0f;
			}
			else
			{
				segmentT += (trajectorySpeed * deltaTime) / dist;
				while (segmentT >= 1.0f)
				{
					segmentT -= 1.0f;
					currentSegment = (currentSegment + 1) % n;
					a = trajectoryPoints[currentSegment];
					b = trajectoryPoints[(currentSegment + 1) % n];
					dist = glm::length(b - a);
					if (dist < 0.0001f) { segmentT = 0.0f; break; }
				}
				objectPosition = a + (b - a) * segmentT;
			}
		}
		else if (!trajectoryPoints.empty())
		{
			objectPosition = trajectoryPoints[currentSegment % trajectoryPoints.size()];
		}

		// Desenhar a Suzanne (apenas um objeto principal)
		glm::mat4 model = glm::mat4(1);

		// 1. Translacao pela trajetoria
		model = glm::translate(model, objectPosition);

		// 2. Rotação do objeto
		if (rotateX)
			model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
		else if (rotateY)
			model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		else if (rotateZ)
			model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

		// 3. Escala de input do usuário
		model = glm::scale(model, glm::vec3(scaleObj));

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		glDrawArrays(GL_TRIANGLES, 0, nVertices);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	if (texID) glDeleteTextures(1, &texID);
	glfwTerminate();
	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	// Comandos de ativação das luzes do sistema 3 Points
	if (key == GLFW_KEY_1 && action == GLFW_PRESS) enableKeyLight = !enableKeyLight;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS) enableFillLight = !enableFillLight;
	if (key == GLFW_KEY_3 && action == GLFW_PRESS) enableBackLight = !enableBackLight;

	// Comandos de escala ([ e ])
	float scaleSpeed = 0.05f;
	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scaleObj -= scaleSpeed;
	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scaleObj += scaleSpeed;

	// Trajetoria
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		trajectoryActive = !trajectoryActive;
		cout << "Trajetoria " << (trajectoryActive ? "ON" : "OFF") << endl;
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		// adiciona um ponto na frente da camera (2 unidades pra frente)
		glm::vec3 p = camera.Position + camera.Front * 2.0f;
		trajectoryPoints.push_back(p);
		cout << "Ponto adicionado: (" << p.x << ", " << p.y << ", " << p.z << ") total=" << trajectoryPoints.size() << endl;
		if (trajectoryPoints.size() == 1) objectPosition = p;
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		saveTrajectory(trajectoryFilePath, trajectoryPoints);
	}
	if (key == GLFW_KEY_L && action == GLFW_PRESS)
	{
		loadTrajectory(trajectoryFilePath, trajectoryPoints);
		currentSegment = 0;
		segmentT = 0.0f;
		if (!trajectoryPoints.empty()) objectPosition = trajectoryPoints[0];
	}
	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		trajectoryPoints.clear();
		currentSegment = 0;
		segmentT = 0.0f;
		objectPosition = glm::vec3(0.0f);
		cout << "Trajetoria limpa" << endl;
	}
	if (key == GLFW_KEY_COMMA && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		trajectorySpeed = glm::max(0.05f, trajectorySpeed - 0.1f);
		cout << "Velocidade trajetoria: " << trajectorySpeed << endl;
	}
	if (key == GLFW_KEY_PERIOD && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		trajectorySpeed += 0.1f;
		cout << "Velocidade trajetoria: " << trajectorySpeed << endl;
	}
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the mouse moves, this callback is called
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
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
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
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Carrega um arquivo .OBJ. Lê v, vt, vn e f, montando o vBuffer com 8 floats por vértice:
// posição (x, y, z), cor (r, g, b) e coordenada de textura (s, t).
// Recupera também o nome do arquivo .MTL referenciado em "mtllib".
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
		cerr << "Erro ao tentar ler o arquivo " << filePath << endl;
		return -1;
	}

	string line;
	while (getline(arqEntrada, line))
	{
		istringstream ssline(line);
		string word;
		ssline >> word;

		if (word == "mtllib")
		{
			ssline >> mtlFileName;
		}
		else if (word == "v")
		{
			glm::vec3 vertice;
			ssline >> vertice.x >> vertice.y >> vertice.z;
			vertices.push_back(vertice);
		}
		else if (word == "vt")
		{
			glm::vec2 vt;
			ssline >> vt.s >> vt.t;
			texCoords.push_back(vt);
		}
		else if (word == "vn")
		{
			glm::vec3 normal;
			ssline >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
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
				if (getline(ss, index)) ni = !index.empty() ? stoi(index) - 1 : -1;

				// Posição
				vBuffer.push_back(vertices[vi].x);
				vBuffer.push_back(vertices[vi].y);
				vBuffer.push_back(vertices[vi].z);

				// Cor
				vBuffer.push_back(color.r);
				vBuffer.push_back(color.g);
				vBuffer.push_back(color.b);

				// Coordenada de textura
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

				// Normal
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

	cout << "Gerando o buffer de geometria..." << endl;
	GLuint VBO, VAO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Posição (location = 0): 3 floats
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Cor (location = 1): 3 floats
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// Coordenadas de textura (location = 2): 2 floats
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Normais (location = 3): 3 floats
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(8 * sizeof(GLfloat)));
	glEnableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	nVertices = vBuffer.size() / 11;

	return VAO;
}

// Lê um arquivo .MTL recuperando, por enquanto, apenas o nome da textura difusa (map_Kd).
string loadMTL(string filePath, string &textureFileName, glm::vec3 &ka, glm::vec3 &kd, glm::vec3 &ks, float &ns)
{
	string materialName;
	ifstream arq(filePath.c_str());
	if (!arq.is_open())
	{
		cerr << "Erro ao tentar ler o MTL " << filePath << endl;
		return materialName;
	}

	string line;
	while (getline(arq, line))
	{
		istringstream ss(line);
		string word;
		ss >> word;

		if (word == "newmtl")
		{
			ss >> materialName;
		}
		else if (word == "map_Kd")
		{
			ss >> textureFileName;
		}
		else if (word == "Ka")
		{
			ss >> ka.r >> ka.g >> ka.b;
		}
		else if (word == "Kd")
		{
			ss >> kd.r >> kd.g >> kd.b;
		}
		else if (word == "Ks")
		{
			ss >> ks.r >> ks.g >> ks.b;
		}
		else if (word == "Ns")
		{
			ss >> ns;
		}
	}
	arq.close();

	cout << "Material lido: " << materialName << " | textura: " << textureFileName << endl;
	return materialName;
}

// Carrega uma textura usando a stb_image. Retorna o ID OpenGL ou 0 em caso de falha.
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
		cout << "Textura carregada: " << filePath << " (" << width << "x" << height << ", " << nrChannels << " canais)" << endl;
		stbi_image_free(data);
	}
	else
	{
		cerr << "Falha ao carregar textura: " << filePath << endl;
		glDeleteTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, 0);
		return 0;
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	return texID;
}
