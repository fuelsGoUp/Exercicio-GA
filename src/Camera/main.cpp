/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 * e https://antongerdelan.net/opengl/
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 03/03/2026
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <fstream>
#include <sstream>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Camera
#include "Camera.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();
int setupGeometry();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar* vertexShaderSource = R"glsl(#version 450
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec3 ovverrideColor;
out vec4 finalColor;
void main()
{
gl_Position = projection * view * model * vec4(position, 1.0);
finalColor = vec4(color * ovverrideColor, 1.0);
}
)glsl";

// Código fonte do Fragment Shader (em GLSL)
const GLchar* fragmentShaderSource = R"glsl(#version 450
in vec4 finalColor;
out vec4 color;

void main()
{
    color = finalColor;
}
)glsl";

bool rotateX=false, rotateY=false, rotateZ=false;
bool perspective = true; //começa com projeção perspectiva

//Instanciação da Camera
Camera camera(glm::vec3(0.0, 0.0, -3.0), glm::vec3(0.0,1.0,0.0),90.0,0.0);
float deltaTime = 0.0;
float lastFrame = 0.0; 

// constroi um objeto
struct Objeto {
	GLuint VAO;
	int vertexCount;
	glm::vec3 pos;
};

// cria uma lista de objetos
Objeto objetos[4];
int totalObjetos = 3;
int selecionado = 0;

struct MeshData {
	float* vertices;
	int vertexCount;
};

MeshData loadObj(const char* path)
{
	std::ifstream file(path);
	if(!file.is_open()){
		std::cout << "Erro ao abrir o arquivo.\n";
	}

	float tempVertices[10000][3];
	int tempFaces[10000][3];

	int vCount = 0;
	int fCount = 0;

	std::string line;

	while(getline(file, line)){
		std::stringstream ss(line);
		std::string type;
		ss >> type;

		if (type == "v") {
			ss >> tempVertices[vCount][0]
			   >> tempVertices[vCount][1]
			   >> tempVertices[vCount][2];
			vCount++;
		}

		else if (type == "f") {
			ss >> tempFaces[fCount][0]
			   >> tempFaces[fCount][1]
			   >> tempFaces[fCount][2];
			fCount++;
		}
	}

	int totalVertices = fCount * 3;

	float* vertices = new float[totalVertices * 6];

	int index = 0;

	for(int i = 0; i < fCount; i++){
		for(int j = 0; j < 3; j++){
			int vi = tempFaces[i][j] - 1; // OBJ indices start at 1

			vertices[index++] = tempVertices[vi][0];
			vertices[index++] = tempVertices[vi][1];
			vertices[index++] = tempVertices[vi][2];

			vertices[index++] = 1.0f; // R
			vertices[index++] = 1.0f; // G
			vertices[index++] = 1.0f; // B
		}
	}

	MeshData mesh;
	
	// mais adicionar

	return {vertices, totalVertices};

}

GLuint createVAOFromOBJ(MeshData mesh)
{
	Gluint VBO, VAO;

	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, mesh.vertexCount * 6 * sizeof(float), mesh.vertices, GL_STATIC_DRAW);

	// posição
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

// Função MAIN
int main()
{
	// Inicialização da GLFW
	glfwInit();

	//Muita atenção aqui: alguns ambientes não aceitam essas configurações
	//Você deve adaptar para a versão do OpenGL suportada por sua placa
	//Sugestão: comente essas linhas de código para desobrir a versão e
	//depois atualize (por exemplo: 4.5 com 4 e 5)
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//Essencial para computadores da Apple
//#ifdef __APPLE__
//	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//#endif

	// Criação da janela GLFW
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Olá Camera Sintética!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	// GLAD: carrega todos os ponteiros d funções da OpenGL
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;

	}

	// Obtendo as informações de versão
	const GLubyte* renderer = glGetString(GL_RENDERER); /* get renderer string */
	const GLubyte* version = glGetString(GL_VERSION); /* version as a string */
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	// Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);


	// Compilando e buildando o programa de shader
	GLuint shaderID = setupShader();

	// Criando os buffers de vertices dos objetos e armazenando o identificador do VAO retornado por setupGeometry() na estrutura de cada objeto
	objetos[0].VAO = setupGeometry();
	objetos[0].vertexCount = 18;

	objetos[1].VAO = setupGeometry();
	objetos[1].vertexCount = 18;

	objetos[2].VAO = setupGeometry();
	objetos[2].vertexCount = 18;

	// posição dos objetos
	for (int i = 0; i < totalObjetos; i++){
		objetos[i].pos.x = (i % 2) * 2.0f - 1.0f;
		objetos[i].pos.y = (i / 2) * -2.0f + 1.0f;
		objetos[i].pos.z = 0.0f;
	}

	glUseProgram(shaderID);

    // Matriz de modelo - Transformações nos objetos
	glm::mat4 model = glm::mat4(1); //matriz identidade;
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
    //-----------------
    // Matriz de projeção
    //glm::mat4 projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
    //glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	
    glm::mat4 projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/(float)HEIGHT,0.1f,100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	
    
    // Matriz de view
    glm::mat4 view = glm::lookAt(glm::vec3(0,0,-3), glm::vec3(0,0,0), glm::vec3(0,1,0));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    glEnable(GL_DEPTH_TEST);
    

	// Loop da aplicação - "game loop"
	while (!glfwWindowShouldClose(window))
	{
		// Controle do tempo entre frames
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Checa se houveram eventos de input (key pressed, mouse moved etc.) e chama as funções de callback correspondentes
		glfwPollEvents();

		// Limpa o buffer de cor
		glClearColor(0.2f, 0.2f, 0.2f, 1.0f); //cor de fundo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Essas funções estão depreciadas, só vão funcionar se usarmos o 
        // OpenGL em modo "Compability" -- eu as deixo pra facilitar a visualização
		glLineWidth(10);
		glPointSize(10);

        // ----------
         
         
         if (perspective)
         {
            projection = glm::perspective(glm::radians(45.0f),(float)WIDTH/(float)HEIGHT,0.1f,100.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

         }
         else // Troca para projeção paralela ortográfica
         {
            projection = glm::ortho(-3.0, 3.0, -3.0, 3.0, 0.1, 100.0);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
         }

		 if(glfwGetKey(window,GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard("FORWARD",deltaTime);
		 if(glfwGetKey(window,GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard("BACKWARD",deltaTime);
		 if(glfwGetKey(window,GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard("LEFT",deltaTime);
		 if(glfwGetKey(window,GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard("RIGHT",deltaTime);
        
        if (glfwGetKey(window, GLFW_KEY_1)==GLFW_PRESS)
        {
            //Visualização de frente
            view = glm::lookAt(glm::vec3(0,0,-3), glm::vec3(0,0,0), glm::vec3(0,1,0));
        }
        else if (glfwGetKey(window, GLFW_KEY_2)==GLFW_PRESS)
        {
            //Visualização de costas
            view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0));
        }
        else if (glfwGetKey(window, GLFW_KEY_3)==GLFW_PRESS)
        {
            //Visualização da esquerda
            view = glm::lookAt(glm::vec3(-3,0,0), glm::vec3(0,0,0), glm::vec3(0,1,0));
        }
        else if (glfwGetKey(window, GLFW_KEY_4)==GLFW_PRESS)
        {
            //Visualização da direita
            view = glm::lookAt(glm::vec3(3,0,0), glm::vec3(0,0,0), glm::vec3(0,1,0));
        }
        else if (glfwGetKey(window, GLFW_KEY_5)==GLFW_PRESS)
        {
            //Visualização de cima
            view = glm::lookAt(glm::vec3(0,3,0), glm::vec3(0,0,0), glm::vec3(0,0,1));
        }
        else if (glfwGetKey(window, GLFW_KEY_6)==GLFW_PRESS)
        {
            //Visualização debaixo
            view = glm::lookAt(glm::vec3(0,-3,0), glm::vec3(0,0,0), glm::vec3(0,0,1));
        }
        
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
        // ----------

		float angle = (GLfloat)glfwGetTime();
		model = glm::mat4(1); 
		if (rotateX)
		{
			model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
			
		}
		else if (rotateY)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

		}
		else if (rotateZ)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

		}

		glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
		
		// Atualização da matriz de view de acordo com as mudanças que ela sofreu via input
		// de mouse e/ou teclado
		view = camera.getViewMatrix();
		glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// renderiza os objetos
		for (int i = 0; i < totalObjetos; i++){
			glm::mat4 model = glm::mat4(1);

			model = glm::translate(model, objetos[i].pos);

			// rotação do selecionado
			if(i == selecionado){
				float angle = glfwGetTime();
				model = glm::rotate(model, angle, glm::vec3(0,1,0));
			}

			glUniformMatrix4fv(glGetUniformLocation(shaderID, "moddel"), 1, GL_FALSE, glm::value_ptr(model));

			// cores diferentes ao selecionar
			if(i == selecionado){
				glUniform3f(glGetUniformLocation(shaderID, "ovverrideColor"), 1.0f, 0.3f, 0.3f);
			}
			else{
				glUniform3f(glGetUniformLocation(shaderID, "ovverrideColor"), 1.0f, 1.0f, 1.0f);
			}
			
			// renderização
			glBindVertexArray(objetos[i].VAO);
			glDrawArrays(GL_TRIANGLES, 0, objetos[i].vertexCount);

		}

		// Troca os buffers da tela
		glfwSwapBuffers(window);
	}
	// Pede pra OpenGL desalocar os buffers
	glDeleteVertexArrays(1, &VAO);
	// Finaliza a execução da GLFW, limpando os recursos alocados por ela
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_TAB && action == GLFW_PRESS){
		selecionado = (selecionado + 1) % totalObjetos;
	}
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

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
         {
            perspective = !perspective;
         }



}

//Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

// Esta função está bastante harcoded - objetivo é criar os buffers que armazenam a 
// geometria de um triângulo
// Apenas atributo coordenada nos vértices
// 1 VBO com as coordenadas, VAO com apenas 1 ponteiro para atributo
// A função retorna o identificador do VAO
int setupGeometry()
{
	// Aqui setamos as coordenadas x, y e z do triângulo e as armazenamos de forma
	// sequencial, já visando mandar para o VBO (Vertex Buffer Objects)
	// Cada atributo do vértice (coordenada, cores, coordenadas de textura, normal, etc)
	// Pode ser arazenado em um VBO único ou em VBOs separados
	GLfloat vertices[] = {

        // Base da pirâmide: 2 triângulos (multicor)
        // x     y     z      r    g    b
        -0.5, -0.5, -0.5,   1.0, 1.0, 0.0,
        -0.5, -0.5,  0.5,   0.0, 1.0, 1.0,
         0.5, -0.5, -0.5,   1.0, 0.0, 1.0,

        -0.5, -0.5,  0.5,   1.0, 1.0, 0.0,
         0.5, -0.5,  0.5,   0.0, 1.0, 1.0,
         0.5, -0.5, -0.5,   1.0, 0.0, 1.0,

        // Frente (z = -0.5) -> Amarelo
        -0.5, -0.5, -0.5,   1.0, 1.0, 0.0,
         0.0,  0.5,  0.0,   1.0, 1.0, 0.0,
         0.5, -0.5, -0.5,   1.0, 1.0, 0.0,

        // Esquerda (x = -0.5) -> Ciano
        -0.5, -0.5, -0.5,   0.0, 1.0, 1.0,
         0.0,  0.5,  0.0,   0.0, 1.0, 1.0,
        -0.5, -0.5,  0.5,   0.0, 1.0, 1.0,

        // Trás (z = 0.5) -> Verde
        -0.5, -0.5,  0.5,   0.0, 1.0, 0.0,
         0.0,  0.5,  0.0,   0.0, 1.0, 0.0,
         0.5, -0.5,  0.5,   0.0, 1.0, 0.0,

        // Direita (x = 0.5) -> Magenta
         0.5, -0.5,  0.5,   1.0, 0.0, 1.0,
         0.0,  0.5,  0.0,   1.0, 0.0, 1.0,
         0.5, -0.5, -0.5,   1.0, 0.0, 1.0,
    };


	GLuint VBO, VAO;

	//Geração do identificador do VBO
	glGenBuffers(1, &VBO);

	//Faz a conexão (vincula) do buffer como um buffer de array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	//Envia os dados do array de floats para o buffer da OpenGl
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//Geração do identificador do VAO (Vertex Array Object)
	glGenVertexArrays(1, &VAO);

	// Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
	// e os ponteiros para os atributos 
	glBindVertexArray(VAO);
	
	//Para cada atributo do vertice, criamos um "AttribPointer" (ponteiro para o atributo), indicando: 
	// Localização no shader * (a localização dos atributos devem ser correspondentes no layout especificado no vertex shader)
	// Numero de valores que o atributo tem (por ex, 3 coordenadas xyz) 
	// Tipo do dado
	// Se está normalizado (entre zero e um)
	// Tamanho em bytes 
	// Deslocamento a partir do byte zero 
	
	//Atributo posição (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Atributo cor (r, g, b)
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1);


	// Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice 
	// atualmente vinculado - para que depois possamos desvincular com segurança
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
	glBindVertexArray(0);

	return VAO;
}

