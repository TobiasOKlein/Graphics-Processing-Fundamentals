/**
 * @author: Tobias Klein
 * @date: 2026-09-09
 * @description: 2. Faça o desenho de um círculo na tela, utilizando a
 * equação paramétrica do círculo para gerar os vértices.
 * Depois disso:
 * a) Desenhe um octágono
 * b) Desenhe um pentágono
 * c) Desenhe um pac-man
 * d) Desenhe uma fatia de pizza
 * e) DESAFIO 1: Desenhe uma “estrela”
 * f) DESAFIO 2: Desenhe uma espiral.
 * 
 * @note Copilot was used to assist in Doxygen documentation (as preferred by the author)
 * and writing part of this code. Also was used part of the structure provided by the professor,
 * but the architecture and final implementation were done by the author.
 */

// INCLUDES

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// DEFINES

// GP = Graphics Processing - Para evitar conflito com quaisquer outras definicoes

#define GP_PI 3.14159265358979323846f
#define GP_CIRCLE 360.0f
#define GP_SEMICIRCLE 180.0f

// STRUCTS

// Representa um angulo internamente em radianos.
struct Angle
{
    float value;

    // "Métodos" de uma estrutura que retornam ela mesma precisam ser estáticos, pois:
    // "uma referência a um membro não estático deve ser relativa ao objeto específico"
    static Angle fromDegrees(float degrees)
    {
        return {degrees * GP_PI / GP_SEMICIRCLE};
    }

    static Angle fromRadians(float radians)
    {
        return {radians};
    }

    float toDegrees() const
    {
        return value * GP_SEMICIRCLE / GP_PI;
    }

    float toRadians() const
    {
        return value;
    }

    // Permite usar Angle diretamente em funcoes que esperam radianos.
    operator float() const
    {
        return toRadians();
    }
};

// Converte uma cor 0xRRGGBB em componentes normalizados para OpenGL.
struct Color
{
    float R;
    float G;
    float B;

    static Color fromHex(std::uint32_t hex)
    {
        // Automaticamente normalizando os valores de 0-255 para 0.0-1.0.
        return {
            (float)(((hex >> 16) & 0xFF) / 255.0f),
            (float)(((hex >> 8) & 0xFF) / 255.0f),
            (float)(( hex      & 0xFF) / 255.0f)};
    }
};

// FUNCTION PROTOTYPES

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);

// Protótipos das funções auxiliares para criação de shaders e geometria
// Foi usado GLuint no lugar de int para compatibilidade com o OpenGL
GLuint compileShader(GLenum shaderType, const GLchar *source);
GLuint setupShader();
GLuint setupGeometry(const std::vector<float> &vertices);

void clearScreen(const Color &color);
void appendPoint(std::vector<float> &vertices, float x, float y, const Color &color);
void appendVertices(std::vector<float> &destination, const std::vector<float> &source);

std::vector<float> makeCircleOutline(float centerX, float centerY, float radius,
                                           int pointCount, Angle initialAngle,
                                           Angle endAngle, const Color &color);
std::vector<float> makeFilledSector(float centerX, float centerY, float radius,
                                          int arcPoints, Angle initialAngle,
                                          Angle endAngle, const Color &color);
std::vector<float> makeClosedCircle(float centerX, float centerY, float radius,
                                          int pointCount, Angle initialAngle,
                                          const Color &color);
std::vector<float> makeStar(float centerX, float centerY, float radius,
                                  const Color &color);
std::vector<float> makeSpiral(float centerX, float centerY, int points,
                                    float loops, float radius, Angle rotation,
                                    const Color &color);


// GLOBAL VARIABLES

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 800, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"glsl(
#version 460 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
out vec3 vertexColor;

void main()
{
    gl_Position = vec4(position, 1.0);
    vertexColor = color;
}
)glsl";

// Código fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"glsl(
#version 460 core
in vec3 vertexColor;
out vec4 color;

void main()
{
    color = vec4(vertexColor, 1.0);
}
)glsl";


// MAIN FUNCTION

/**
 * @brief Inicializa a janela, gera e renderiza todas as formas geométricas do exercício 2.
 *
 * @return Código de saída da aplicação.
 */
int main()
{
    // INITIALIZATION
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Lista 1 - Exercício 2", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Falha ao inicializar GLAD" << std::endl;
        glfwTerminate();
        return -1;
    }

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    // RENDER SETUP

    // Cada forma e gerada pela equacao parametrica e enviada para um unico VBO.
    const Color circleColor = Color::fromHex(0xCCCCCC);
    const Color octagonColor = Color::fromHex(0xFF4D99);
    const Color pentagonColor = Color::fromHex(0x33B3E6);
    const Color pacmanColor = Color::fromHex(0xFFCC00);
    const Color pizzaColor = Color::fromHex(0xE6801A);
    const Color starColor = Color::fromHex(0xFFE633);
    const Color spiralColor = Color::fromHex(0x33FFB3);

    const std::vector<float> circle = makeCircleOutline(0.0f, 0.0f, 0.18f, 40,
                                                    Angle::fromDegrees(0.0f),
                                                    Angle::fromDegrees(GP_CIRCLE),
                                                    circleColor);
    const std::vector<float> octagon = makeClosedCircle(-0.75f, 0.55f, 0.20f, 8,
                                                    Angle::fromDegrees(22.5f),
                                                    octagonColor);
    const std::vector<float> pentagon = makeClosedCircle(0.0f, 0.55f, 0.20f, 5,
                                                    Angle::fromDegrees(90.0f),
                                                    pentagonColor);
    const std::vector<float> pacman = makeFilledSector(0.75f, 0.55f, 0.20f, 25,
                                                    Angle::fromDegrees(45.0f),
                                                    Angle::fromDegrees(315.0f),
                                                    pacmanColor);
    const std::vector<float> pizza = makeFilledSector(-0.75f, -0.55f, 0.20f, 20,
                                                    Angle::fromDegrees(0.0f),
                                                    Angle::fromDegrees(60.0f),
                                                    pizzaColor);
    const std::vector<float> star = makeStar(0.0f, -0.55f, 0.20f, starColor);
    const std::vector<float> spiral = makeSpiral(0.60f, -0.55f, 120, 3.5f, 0.30f,
                                                    Angle::fromRadians(0.0f),
                                                    spiralColor);

    std::vector<float> allVertices;
    appendVertices(allVertices, circle);
    appendVertices(allVertices, octagon);
    appendVertices(allVertices, pentagon);
    appendVertices(allVertices, pacman);
    appendVertices(allVertices, pizza);
    appendVertices(allVertices, star);
    appendVertices(allVertices, spiral);


    // VAO = Vertex Array Object, encapsula o VBO e os atributos de vértice.
    GLuint VAO = setupGeometry(allVertices);
    GLuint shaderProgram = setupShader();
    glUseProgram(shaderProgram);


    // Indices iniciais e quantidades de vertices.
    const int circleStart = 0;
    const int circleCount = (int)((circle.size() / 6));

    const int octagonStart = circleCount;
    const int octagonCount = (int)((octagon.size() / 6));

    const int pentagonStart = octagonStart + octagonCount;
    const int pentagonCount = (int)((pentagon.size() / 6));

    const int pacmanStart = pentagonStart + pentagonCount;
    const int pacmanCount = (int)((pacman.size() / 6));

    const int pizzaStart = pacmanStart + pacmanCount;
    const int pizzaCount = (int)((pizza.size() / 6));

    const int starStart = pizzaStart + pizzaCount;
    const int starCount = (int)((star.size() / 6));

    const int spiralStart = starStart + starCount;
    const int spiralCount = (int)((spiral.size() / 6));

    // Cor de fundo da tela
    const Color backgroundColor = Color::fromHex(0x0D0D14);
    clearScreen(backgroundColor);


    // RENDER LOOP

    while (!glfwWindowShouldClose(window)) // Enquanto a janela nao fechar
    {
        glfwPollEvents();
        clearScreen(backgroundColor);

        glBindVertexArray(VAO);
        glUseProgram(shaderProgram);

        glDrawArrays(GL_LINE_LOOP, circleStart, circleCount);
        glDrawArrays(GL_LINE_LOOP, octagonStart, octagonCount);
        glDrawArrays(GL_LINE_LOOP, pentagonStart, pentagonCount);
        glDrawArrays(GL_TRIANGLE_FAN, pacmanStart, pacmanCount);
        glDrawArrays(GL_TRIANGLE_FAN, pizzaStart, pizzaCount);
        glDrawArrays(GL_LINE_LOOP, starStart, starCount);
        glDrawArrays(GL_LINE_STRIP, spiralStart, spiralCount);

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO); // Limpeza do VAO para liberar recursos apos o uso
    glDeleteProgram(shaderProgram); // Limpeza do shader para liberar recursos apos o uso
    glfwTerminate();
    return 0;
}


// FUNCTIONS


/**
 * @brief Adiciona um vértice posicionado com sua cor ao vetor de geometria.
 *
 * @param[in,out] vertices Vetor que receberá os valores do novo ponto.
 * @param[in] x Coordenada x do ponto.
 * @param[in] y Coordenada y do ponto.
 * @param[in] color Cor do ponto em formato RGB normalizado.
 */
void appendPoint(std::vector<float> &vertices, float x, float y, const Color &color)
{
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(0.0f);
    vertices.push_back(color.R);
    vertices.push_back(color.G);
    vertices.push_back(color.B);
}


/**
 * @brief Acrescenta todos os vértices de uma forma ao vetor principal de renderização (VBO).
 *
 * @param[in,out] destination Vetor de destino que receberá os vértices.
 * @param[in] source Vetor contendo os vértices a serem concatenados.
 */
void appendVertices(std::vector<float> &destination, const std::vector<float> &source)
{
    destination.insert(destination.end(), source.begin(), source.end());
}


/**
 * @brief Gera os vértices de um contorno circular entre dois ângulos.
 *
 * @param[in] centerX Coordenada x do centro do círculo.
 * @param[in] centerY Coordenada y do centro do círculo.
 * @param[in] radius Raio do círculo.
 * @param[in] pointCount Quantidade de pontos usados para aproximar o arco.
 * @param[in] initialAngle Ângulo inicial do arco em radianos.
 * @param[in] endAngle Ângulo final do arco em radianos.
 * @param[in] color Cor dos vértices gerados.
 *
 * @return Vetor com os pontos do contorno em formato de vértices OpenGL.
 */
std::vector<float> makeCircleOutline(float centerX, float centerY, float radius,
                                           int pointCount, Angle initialAngle,
                                           Angle endAngle, const Color &color)
{
    std::vector<float> vertices;
    if (pointCount <= 1)
        return vertices;

    const float start = initialAngle.toRadians();
    const float end = endAngle.toRadians();
    const float step = (end - start) / (float)((pointCount - 1));

    for (int i = 0; i < pointCount; ++i)
    {
        const float angle = start + step * (float)((i));
        const float x = centerX + radius * std::cos(angle);
        const float y = centerY + radius * std::sin(angle);
        appendPoint(vertices, x, y, color);
    }

    return vertices;
}


/**
 * @brief Cria um setor circular preenchido a partir do centro, do raio e do arco.
 *
 * @param[in] centerX Coordenada x do centro do setor.
 * @param[in] centerY Coordenada y do centro do setor.
 * @param[in] radius Raio do setor.
 * @param[in] arcPoints Número de pontos do arco para aproximar a fatia.
 * @param[in] initialAngle Ângulo inicial do setor em radianos.
 * @param[in] endAngle Ângulo final do setor em radianos.
 * @param[in] color Cor da forma gerada.
 *
 * @return Vetor com os vértices do setor preenchido.
 */
std::vector<float> makeFilledSector(float centerX, float centerY, float radius,
                                          int arcPoints, Angle initialAngle,
                                          Angle endAngle, const Color &color)
{
    // O GL_TRIANGLE_FAN transforma esses pontos em uma fatia circular.

    std::vector<float> vertices;
    appendPoint(vertices, centerX, centerY, color);

    const float start = initialAngle.toRadians();
    const float end = endAngle.toRadians();
    const float step = (end - start) / (float)((arcPoints - 1));

    for (int i = 0; i < arcPoints; ++i)
    {
        const float angle = start + step * (float)((i));
        appendPoint(vertices,
                centerX + radius * std::cos(angle),
                centerY + radius * std::sin(angle),
                color);
    }

    return vertices;
}


/**
 * @brief Gera um polígono fechado regular centrado em um ponto.
 *
 * @param[in] centerX Coordenada x do centro do polígono.
 * @param[in] centerY Coordenada y do centro do polígono.
 * @param[in] radius Raio do polígono regular.
 * @param[in] pointCount Quantidade de lados ou pontos do polígono.
 * @param[in] initialAngle Ângulo inicial de rotação do polígono.
 * @param[in] color Cor do polígono.
 *
 * @return Vetor com os vértices do polígono fechado.
 */
std::vector<float> makeClosedCircle(float centerX, float centerY, float radius,
                                          int pointCount, Angle initialAngle,
                                          const Color &color)
{
    std::vector<float> vertices;
    for (int i = 0; i < pointCount; ++i)
    {
        const float angle = initialAngle.toRadians() + Angle::fromDegrees((GP_CIRCLE / pointCount) * i).toRadians();
        const float x = centerX + radius * std::cos(angle);
        const float y = centerY + radius * std::sin(angle);
        appendPoint(vertices, x, y, color);
    }
    return vertices;
}


/**
 * @brief Cria uma estrela geométrica com alternância de raios interno e externo.
 *
 * @param[in] centerX Coordenada x do centro da estrela.
 * @param[in] centerY Coordenada y do centro da estrela.
 * @param[in] radius Raio externo da estrela.
 * @param[in] color Cor da estrela.
 *
 * @return Vetor com os vértices da estrela.
 */
std::vector<float> makeStar(float centerX, float centerY, float radius, const Color &color)
{
    // A estrela possui dez vertices: cinco pontas externas e cinco internas.
    // Alternar os raios cria o recuo entre cada par de pontas.
    const int points = 10;
    std::vector<float> vertices;

    for (int i = 0; i < points; ++i)
    {
        // A separacao angular uniforme distribui os vertices a cada 36 graus.
        // O deslocamento de 90 graus faz a primeira ponta ficar para cima.
        const float t = Angle::fromDegrees((GP_CIRCLE / points) * i + 90.0f);
        const float factor = (i % 2 == 0) ? 1.0f : 0.45f;
        const float x = centerX + radius * factor * std::cos(t);
        const float y = centerY + radius * factor * std::sin(t);
        appendPoint(vertices, x, y, color);
    }
    return vertices;
}


/**
 * @brief Gera uma espiral com crescimento radial ao longo de várias voltas.
 *
 * @param[in] centerX Coordenada x do centro da espiral.
 * @param[in] centerY Coordenada y do centro da espiral.
 * @param[in] points Quantidade de pontos usados para aproximar a espiral.
 * @param[in] loops Número de voltas da espiral.
 * @param[in] radius Raio máximo da espiral.
 * @param[in] rotation Rotação inicial da espiral em radianos.
 * @param[in] color Cor da espiral.
 *
 * @return Vetor com os vértices da espiral.
 */
std::vector<float> makeSpiral(float centerX, float centerY, int points,
                                    float loops, float radius, Angle rotation,
                                    const Color &color)
{
    std::vector<float> vertices;
    // O angulo total determina quantas voltas a espiral realiza.
    const float totalAngle = loops * 2.0f * GP_PI;
    const float startAngle = rotation.toRadians();

    for (int i = 0; i < points; ++i)
    {
        // t varia de 0 a 1. Assim, a espiral comeca no centro e cresce
        // gradualmente ate atingir o raio informado.
        const float t = (float)((i) / (float)((points - 1)));
        const float angle = startAngle + totalAngle * t;
        const float currentRadius = radius * t;
        const float x = centerX + currentRadius * std::cos(angle);
        const float y = centerY + currentRadius * std::sin(angle);
        appendPoint(vertices, x, y, color);
    }
    return vertices;
}


/**
 * @brief Fecha a janela quando a tecla Escape é pressionada.
 *
 * @param[in] window Janela GLFW associada ao evento.
 * @param[in] key Código da tecla pressionada.
 * @param[in] scancode Código do scancode da tecla.
 * @param[in] action Ação do evento de teclado.
 * @param[in] mode Modificadores ativos no evento.
 */
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
}


/**
 * @brief Compila um shader individual a partir do código fonte GLSL.
 *
 * @param[in] shaderType Tipo do shader a ser compilado.
 * @param[in] source Código fonte GLSL do shader.
 *
 * @return Identificador do shader compilado.
 */
GLuint compileShader(GLenum shaderType, const GLchar *source)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    // Checando erros de compilação (exibição via log no terminal)
    GLint success = 0;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Erro de compilacao do shader:\n" << infoLog << std::endl;
    }
    return shader;
}


/**
 * @brief Compila e vincula os shaders de vértice e fragmento do programa principal.
 *
 * @return Identificador do programa de shader linkado.
 */
GLuint setupShader()
{
    // Linkando os shaders e criando o identificador do programa de shader

    // Vertex Shader
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);

    // Fragment Shader
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Checando por erros de linkagem
    GLint success = 0;
    GLchar infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Erro de linkagem do shader program:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}


/**
 * @brief Cria o VAO e o VBO com os vértices fornecidos pela geometria composta.
 *
 * @param[in] vertices Vetor com os dados de vértice em formato intercalado XYZRGB.
 *
 * @return Identificador do VAO gerado.
 */
GLuint setupGeometry(const std::vector<float> &vertices)
{
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (GLvoid *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}


/**
 * @brief Limpa o buffer de cor com a cor especificada.
 *
 * @param[in] color Cor de fundo em RGB normalizado.
 */
void clearScreen(const Color &color)
{
    glClearColor(color.R, color.G, color.B, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}