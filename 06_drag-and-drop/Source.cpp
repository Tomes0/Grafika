#include <array>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLM/glm.hpp>
#include <iostream>
#include <math.h>
#include <string>
#include <vector>

using namespace std;
static std::vector<glm::vec3>	myPoints = {
	glm::vec3(-0.5f, -0.5f, 0.0f),
	glm::vec3(0.5f,  -0.5f, 0.0f),
	glm::vec3(0.5f,  0.5f, 0.0f),
	glm::vec3(-0.5f,  0.5f, 0.0f),
};
std::vector<glm::vec3> bezierPoints = {};

#define		numVBOs			2
#define		numVAOs			3
GLuint		VBO[numVBOs];
GLuint		VAO[numVAOs];
GLuint		vaoIndex;

int			window_width = 600;
int			window_height = 600;
char		window_title[] = "Drag and Drop";
GLboolean	keyboard[512] = { GL_FALSE };
GLFWwindow* window = nullptr;
GLuint		renderingProgram;
GLint		dragged = -1;
GLboolean	isLeftMouseButtonPressed = false;

bool checkOpenGLError() {
	bool	foundError = false;
	int		glErr = glGetError();
	while (glErr != GL_NO_ERROR) {
		cout << "glError: " << glErr << endl;

		foundError = true;
		glErr = glGetError();
	}
	return foundError;
}

void printShaderLog(GLuint shader) {
	int		length = 0;
	int		charsWritten = 0;
	char* log = nullptr;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

	if (length > 0) {
		log = (char*)malloc(length);
		glGetShaderInfoLog(shader, length, &charsWritten, log);
		cout << "Shader Info Log: " << log << endl;
		free(log);
	}
}

void printProgramLog(int prog) {
	int		length = 0;
	int		charsWritten = 0;
	char* log = nullptr;
	glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);

	if (length > 0) {
		log = (char*)malloc(length);
		glGetProgramInfoLog(prog, length, &charsWritten, log);
		cout << "Program Info Log: " << log << endl;
		free(log);
	}
}

string readShaderSource(const char* filePath) {
	ifstream	fileStream(filePath, ios::in);
	string		content;
	string		line;
	while (!fileStream.eof()) {
		getline(fileStream, line);
		content.append(line + "\n");
	}
	fileStream.close();
	return content;
}

GLuint createShaderProgram() {
	GLint		vertCompiled;
	GLint		fragCompiled;
	GLint		linked;
	string		vertShaderStr = readShaderSource("vertexShader.glsl");
	string		fragShaderStr = readShaderSource("fragmentShader.glsl");
	GLuint		vShader = glCreateShader(GL_VERTEX_SHADER);
	GLuint		fShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* vertShaderSrc = vertShaderStr.c_str();
	const char* fragShaderSrc = fragShaderStr.c_str();
	glShaderSource(vShader, 1, &vertShaderSrc, NULL);
	glShaderSource(fShader, 1, &fragShaderSrc, NULL);
	glCompileShader(vShader);
	checkOpenGLError();
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &vertCompiled);
	if (vertCompiled != 1) {
		cout << "Vertex compilation failed." << endl;
		printShaderLog(vShader);
	}
	glCompileShader(fShader);
	checkOpenGLError();
	glGetShaderiv(fShader, GL_COMPILE_STATUS, &fragCompiled);
	if (fragCompiled != 1) {
		cout << "Fragment compilation failed." << endl;
		printShaderLog(fShader);
	}
	GLuint		vfProgram = glCreateProgram();
	glAttachShader(vfProgram, vShader);
	glAttachShader(vfProgram, fShader);
	glLinkProgram(vfProgram);
	checkOpenGLError();
	glGetProgramiv(vfProgram, GL_LINK_STATUS, &linked);
	if (linked != 1) {
		cout << "Shader linking failed." << endl;
		printProgramLog(vfProgram);
	}
	glDeleteShader(vShader);
	glDeleteShader(fShader);
	return vfProgram;
}

GLfloat dist2(glm::vec3 P1, glm::vec3 P2) {
	GLfloat		dx = P1.x - P2.x;
	GLfloat		dy = P1.y - P2.y;
	return dx * dx + dy * dy;
}

GLint getActivePoint(vector<glm::vec3> p, GLfloat sensitivity, GLfloat x, GLfloat y) {
	GLfloat		s = sensitivity * sensitivity;
	GLint		size = p.size();
	GLfloat		xNorm = x / ((GLfloat)window_width / 2.0) - 1.0f;
	GLfloat		yNorm = y / ((GLfloat)window_height / 2.0) - 1.0f;
	glm::vec3	mousePos = glm::vec3(xNorm, yNorm, 0.0f);
	for (GLint i = 0; i < size; i++)
		if (dist2(p[i], mousePos) < s)
			return i;
	return -1;
}

void init(GLFWwindow* window) {
	renderingProgram = createShaderProgram();
	glGenBuffers(numVBOs, VBO);
	glGenVertexArrays(numVAOs, VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, myPoints.size() * sizeof(glm::vec3), myPoints.data(), GL_STATIC_DRAW);
	glBindVertexArray(VAO[0]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBufferData(GL_ARRAY_BUFFER, myPoints.size() * sizeof(glm::vec3), myPoints.data(), GL_STATIC_DRAW);
	glBindVertexArray(VAO[1]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, bezierPoints.size() * sizeof(glm::vec3), bezierPoints.data(), GL_STATIC_DRAW);
	glBindVertexArray(VAO[2]);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(renderingProgram);

	vaoIndex = glGetUniformLocation(renderingProgram, "vaoIndex");

	glPointSize(10.0f);
	glLineWidth(5.0f);
	glClearColor(0.0, 0.0, 0.0, 1.0);
}

void display(GLFWwindow* window, double currentTime) {
	glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(VAO[0]);
	glProgramUniform1i(renderingProgram, vaoIndex, 0);
	glDrawArrays(GL_LINE_LOOP, 0, myPoints.size());

	glBindVertexArray(VAO[1]);
	glProgramUniform1i(renderingProgram, vaoIndex, 1);
	glDrawArrays(GL_POINTS, 0, myPoints.size());

	glBindVertexArray(VAO[2]);
	glProgramUniform1i(renderingProgram, vaoIndex, 2);
	glDrawArrays(GL_LINE_STRIP, 0, bezierPoints.size());

	glBindVertexArray(0);
}
void cleanUpScene() {
	glDeleteVertexArrays(numVAOs, VAO);
	glDeleteBuffers(numVBOs, VBO);
	glDeleteProgram(renderingProgram);
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
	window_width = width;
	window_height = height;
	glViewport(0, 0, width, height);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if ((action == GLFW_PRESS) && (key == GLFW_KEY_ESCAPE))
		cleanUpScene();
	if (action == GLFW_PRESS)
		keyboard[key] = GL_TRUE;
	else if (action == GLFW_RELEASE)
		keyboard[key] = GL_FALSE;
}

void  drawSimpleLines() {
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, myPoints.size() * sizeof(glm::vec3), myPoints.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

long double NCR(int n, int r) {
	/* Binomiális együttható. */
	/* Binomial coefficient. */
	if (r == 0) return 1;

	/*
	 Extra computation saving for large R, using property:
	 N choose R = N choose (N - R)
	*/
	if (r > n / 2) return NCR(n, n - r);

	long double result = 1;

	for (int k = 1; k <= r; ++k) {
		result *= n - k + 1;
		result /= k;
	}

	return result;
}

/*
It will be the Bernstein basis polynomial of degree n.
*/
GLfloat blending(GLint n, GLint i, GLfloat t) {
	return NCR(n, i) * pow(t, i) * pow(1.0f - t, n - i);
}


void drawBezierCurve(std::vector<glm::vec3> controlPoints) {

	bezierPoints.clear();

	glm::vec3	nextPoint;
	GLfloat		t = 0.0f;// , B;
	GLfloat		increment = 1.0f / 100.0f; /* hány darab szakaszból rakjuk össze a görbénket? */

	while (t <= 1.0f) {
		nextPoint = glm::vec3(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < controlPoints.size(); i++) {
			nextPoint += blending(controlPoints.size() - 1, i, t) * controlPoints[i];
		}

		bezierPoints.push_back(nextPoint);
		t += increment;
	}

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, bezierPoints.size() * sizeof(glm::vec3), bezierPoints.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 1);
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {

	if (dragged >= 0 && isLeftMouseButtonPressed) {
		GLfloat	xNorm = xPos / ((GLfloat)window_width / 2.0f) - 1.0f;
		GLfloat	yNorm = ((GLfloat)window_height - yPos) / ((GLfloat)window_height / 2.0f) - 1.0f;
		myPoints.at(dragged).x = xNorm;
		myPoints[dragged].x = xNorm;
		myPoints.at(dragged).y = yNorm;

		drawSimpleLines();
		drawBezierCurve(myPoints);
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double	x, y;
		glfwGetCursorPos(window, &x, &y);

		isLeftMouseButtonPressed = true;
		dragged = getActivePoint(myPoints, 0.1f, x, window_height - y);


		if (dragged == -1 && myPoints.size() <= 8) {
			GLfloat	xNorm = x / ((GLfloat)window_width / 2.0f) - 1.0f;
			GLfloat	yNorm = ((GLfloat)window_height - y) / ((GLfloat)window_height / 2.0f) - 1.0f;

			myPoints.push_back(glm::vec3(xNorm, yNorm, 0));

			drawSimpleLines();
			drawBezierCurve(myPoints);
		}
	}


	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		double	x, y;
		glfwGetCursorPos(window, &x, &y);
		dragged = getActivePoint(myPoints, 0.1f, x, window_height - y);

		if (dragged >= 0 && myPoints.size() >= 5) {
			myPoints.erase(myPoints.begin() + dragged);

			drawSimpleLines();
			drawBezierCurve(myPoints);
		}
	}

	if ((button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) && action == GLFW_RELEASE) {
		dragged = -1;
		isLeftMouseButtonPressed = false;
	}
}


int main(void) {
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	window = glfwCreateWindow(window_width, window_height, window_title, nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetCursorPosCallback(window, cursorPosCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	if (glewInit() != GLEW_OK)
		exit(EXIT_FAILURE);
	drawBezierCurve(myPoints);
	glfwSwapInterval(1);
	glfwSetWindowSizeLimits(window, 400, 400, 800, 800);
	glfwSetWindowAspectRatio(window, 1, 1);
	init(window);
	while (!glfwWindowShouldClose(window)) {
		display(window, glfwGetTime());
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	cleanUpScene();
	return EXIT_SUCCESS;
}