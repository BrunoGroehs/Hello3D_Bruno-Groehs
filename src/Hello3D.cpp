#include <iostream>
#include <string>

using namespace std;

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int setupGeometry();

const GLuint WIDTH = 1000, HEIGHT = 1000;

const GLchar* vertexShaderSource = "#version 330\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 330\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

bool rotateX = false, rotateY = false, rotateZ = false;

// Variáveis para translação e escala
float transX = 0.0f, transY = 0.0f, transZ = 0.0f;
float scaleObj = 1.0f;

int main()
{
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Bruno Groehs", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);

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
	GLuint VAO = setupGeometry();

	glUseProgram(shaderID);

	glm::mat4 model = glm::mat4(1);
	GLint modelLoc = glGetUniformLocation(shaderID, "model");
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	// Posições dos cubos instanciados
	glm::vec3 cubePositions[] = {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(0.5f,  0.5f, -0.5f),
		glm::vec3(-0.5f, -0.4f,  0.2f)
	};

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		glBindVertexArray(VAO);

		// Loop para desenhar múltiplos cubos
		for (int i = 0; i < 3; i++)
		{
			glm::mat4 model = glm::mat4(1);
			
			// 1. Translação de Input do usuário
			model = glm::translate(model, glm::vec3(transX, transY, transZ));

			// 2. Translação da instância do cubo (para não ficarem todos no mesmo lugar)
			model = glm::translate(model, cubePositions[i]);
			
			// 3. Rotação (do projeto base)
			// Aplica uma rotação inicial para conseguirmos ver que é um objeto 3D de fato (e não 2D olhando de frente)
			model = glm::rotate(model, glm::radians(30.0f), glm::vec3(1.0f, 1.0f, 0.0f));
			
			if (rotateX)
				model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			else if (rotateY)
				model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
			else if (rotateZ)
				model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

			// 4. Escala de Input do usuário
			model = glm::scale(model, glm::vec3(scaleObj));

			// Para eles não ficarem do mesmo tamanho inicial, multiplicamos uma escala extra em alguns
			if (i == 1) model = glm::scale(model, glm::vec3(0.5f));
			if (i == 2) model = glm::scale(model, glm::vec3(0.7f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

			glDrawArrays(GL_TRIANGLES, 0, 36);
			// glDrawArrays(GL_POINTS, 0, 36); // Removido pontos pra não poluir, ou você pode manter
		}
		
		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
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

	// Comandos de translação (WASD para X/Z, IJ para Y)
	float moveSpeed = 0.05f;
	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) transZ -= moveSpeed;
	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) transZ += moveSpeed;
	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT)) transX -= moveSpeed;
	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT)) transX += moveSpeed;
	if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT)) transY += moveSpeed;
	if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT)) transY -= moveSpeed;

	// Comandos de escala ([ e ])
	float scaleSpeed = 0.05f;
	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scaleObj -= scaleSpeed;
	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT)) scaleObj += scaleSpeed;
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

int setupGeometry()
{
	GLfloat vertices[] = {
		// Face Traseira (Vermelho)
		-0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f,
		 0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f,
		 0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 0.0f,
		 0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 0.0f,
		-0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 0.0f,
		-0.25f, -0.25f, -0.25f, 1.0f, 0.0f, 0.0f,

		// Face Frontal (Verde)
		-0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 0.0f,
		 0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 0.0f,
		 0.25f,  0.25f,  0.25f, 0.0f, 1.0f, 0.0f,
		 0.25f,  0.25f,  0.25f, 0.0f, 1.0f, 0.0f,
		-0.25f,  0.25f,  0.25f, 0.0f, 1.0f, 0.0f,
		-0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 0.0f,

		// Face Esquerda (Azul)
		-0.25f,  0.25f,  0.25f, 0.0f, 0.0f, 1.0f,
		-0.25f,  0.25f, -0.25f, 0.0f, 0.0f, 1.0f,
		-0.25f, -0.25f, -0.25f, 0.0f, 0.0f, 1.0f,
		-0.25f, -0.25f, -0.25f, 0.0f, 0.0f, 1.0f,
		-0.25f, -0.25f,  0.25f, 0.0f, 0.0f, 1.0f,
		-0.25f,  0.25f,  0.25f, 0.0f, 0.0f, 1.0f,

		// Face Direita (Amarelo)
		 0.25f,  0.25f,  0.25f, 1.0f, 1.0f, 0.0f,
		 0.25f,  0.25f, -0.25f, 1.0f, 1.0f, 0.0f,
		 0.25f, -0.25f, -0.25f, 1.0f, 1.0f, 0.0f,
		 0.25f, -0.25f, -0.25f, 1.0f, 1.0f, 0.0f,
		 0.25f, -0.25f,  0.25f, 1.0f, 1.0f, 0.0f,
		 0.25f,  0.25f,  0.25f, 1.0f, 1.0f, 0.0f,

		// Face Inferior (Ciano)
		-0.25f, -0.25f, -0.25f, 0.0f, 1.0f, 1.0f,
		 0.25f, -0.25f, -0.25f, 0.0f, 1.0f, 1.0f,
		 0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 1.0f,
		 0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 1.0f,
		-0.25f, -0.25f,  0.25f, 0.0f, 1.0f, 1.0f,
		-0.25f, -0.25f, -0.25f, 0.0f, 1.0f, 1.0f,

		// Face Superior (Magenta)
		-0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 1.0f,
		 0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 1.0f,
		 0.25f,  0.25f,  0.25f, 1.0f, 0.0f, 1.0f,
		 0.25f,  0.25f,  0.25f, 1.0f, 0.0f, 1.0f,
		-0.25f,  0.25f,  0.25f, 1.0f, 0.0f, 1.0f,
		-0.25f,  0.25f, -0.25f, 1.0f, 0.0f, 1.0f
	};

	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}
