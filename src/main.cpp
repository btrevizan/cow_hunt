//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
// INF01047 Fundamentos de Computação Gráfica 2017/1
//               Bernardo Trevizan (285638)
//               Vinicius Meireles (385637)
//
//                   TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <time.h>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;

        char filepath[100];
        strcpy(filepath, filename);
        strcat(filepath, ".obj");

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filepath, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("\nErro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model, int* k, const char* filename); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);
void TextRendering_ShowCowCount(GLFWwindow* window);
void TextRendering_ShowTimer(GLFWwindow* window);
void TextRendering_ShowGameOver(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    int          id;
    bool         hasAnm;                        // se o obj tem animacao
    bool         hasRandAngle;                  // se o obj tem angulos aleatorios
    std::string  name;                          // Nome do objeto
    void*        first_index;                   // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices;                   // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode;                // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id;        // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min;                      // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
    glm::vec3    ambient;                       // refletancia ambiente
    glm::vec3    diffuse;                       // refletancia difusa
    glm::vec3    specular;                      // refletancia especular
    std::vector<glm::vec3> animation;           // coordenadas x,y,z para translacao
    std::vector<glm::vec3> angles;              // angulos de x,y,z para rotacao
    std::vector<glm::vec3> pos;                 // posiçoes das instancia do modelo
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.5209f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 6.0f; // Distância da câmera para a origem

// Variáveis que controlam o movimento da nave
bool wKeyPressed = false;
bool sKeyPressed = false;
bool aKeyPressed = false;
bool dKeyPressed = false;
bool wKeyReleased = true;
bool aKeyReleased = true;
bool sKeyReleased = true;
bool dKeyReleased = true;
bool spaceKeyPressed = false;

// Variável que controla o tamanho do nível (até onde as vacas podem andar)
float tamanhoNivel = 68.0f;

// Variavel que controla se a vaca esta subindo
bool cowUp = false;

// Variavel que controla o número de vacas abduzidas
unsigned int cowCount = 0;

// Variaveis que controlam o tempo da partida
clock_t init_clock;
double timer;

// Camera
glm::vec4 camera_position_c  = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Ponto "c", centro da câmera
glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up"

// Matriz do modelo, utilizada na detecção de colisão
glm::mat4 model;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint obj_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;
GLint ka_uniform;
GLint kd_uniform;
GLint ks_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

#define M_PI 3.14159265358979323846
#define TIME_LIMIT 60

bool isIntersecting(SceneObject* obj1, glm::vec3* coord, glm::vec3* ang, SceneObject* obj2); // detecta se um objeto intersecta o outro
glm::vec3 getAnimation(glm::vec3* anm, glm::vec3* angles);
void LoadObjAttr(const char* filename, SceneObject* obj);
float rad(float d); // converte graus para radianos
float randAngle();  // gera um valor aleatorio entre 0 e PI
void reset();

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com a OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Pedimos apra utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas da OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional
    GLFWwindow* window;
    window = glfwCreateWindow(1280, 720, "Cow Hunt", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas pela OpenGL 3.3, utilizando a
    // biblioteca GLAD. Veja
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 1280, 720); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slide 216 do documento
    // "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    std::vector<const char*> objNames = {"arvore","banheiro","casa","celeiro","chao","cone","disco","silo","trator","turbina","vaca"};
    std::vector<const char*>::iterator it;

    int k = 0;
    const char* basepath = "../../data/";
    for(it = objNames.begin(); it != objNames.end(); it++)
    {
        char filepath[100];
        strcpy(filepath, basepath);
        strcat(filepath, *it);

        ObjModel objmodel(filepath, basepath);
        ComputeNormals(&objmodel);
        BuildTrianglesAndAddToVirtualScene(&objmodel, &k, filepath);

        // Carregamos textura do objeto
        if(k != 6) // não carrega quando for o cone
            LoadTextureImage(filepath); // TextureImageK
    }

    if(argc > 1)
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model, &k, argv[1]);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 46 à 66 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22 à 33 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    // Velocidade do movimento
    float delta = 0.15f;

    // Marca o tempo de inicio
    init_clock = clock();

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Rasterização.
        //
        //           R     G     B     A
        glClearColor(0.3f, 0.6f, 1.0f, 0.5f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos)
        glUseProgram(program_id);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(-g_CameraPhi);
        float z = r*cos(-g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(-g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as variáveis que efetivamente definem a câmera virtual.
        glm::vec4 camera_view_vector = glm::vec4(x,y,z,0.0f); // Vetor "view", sentido para onde a câmera está virada

        // Movimentos possíveis da camera
        // W: para frente
        // S: para trás
        // A: para a esquerda
        // D: para a direita
        glm::vec4 w = -camera_view_vector;
        glm::vec4 u = crossproduct(camera_up_vector, w);

        // Normalizamos os vetores u e w
        w = w / norm(w);
        u = u / norm(u);

        if(wKeyPressed) // frente
            camera_position_c += delta * (-w);

        if(sKeyPressed) // ré
            camera_position_c += delta * w;

        if(aKeyPressed) // esquerda
            camera_position_c += delta * (-u);

        if(dKeyPressed) // direita
            camera_position_c += delta * u;

        // Desenhamos o disco voador em relação a camera
        model = Matrix_Identity(); // Transformação identidade de modelagem

        glm::vec4 l = camera_position_c + camera_view_vector;
        model = Matrix_Translate(l.x, 3.0f, l.z);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawVirtualObject("Disco");

        g_VirtualScene["Disco"].pos.pop_back();
        g_VirtualScene["Disco"].pos.push_back(glm::vec3(l.x, 3.0f, l.z));

        if(spaceKeyPressed)
        {
            // Desenhamos um cone embaixo do disco quando aperta-se a barra de espaco
            model = Matrix_Translate(l.x, -0.504f, l.z);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            DrawVirtualObject("Cone");

            g_VirtualScene["Cone"].pos.pop_back();
            g_VirtualScene["Cone"].pos.push_back(glm::vec3(l.x, -0.504f, l.z));
        }

        camera_position_c.y = 6.5f; // matem a altura da camera constante

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 162 do
        // documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 180-183 do documento
        // "Aula_09_Projecoes.pdf".
        float nearplane = -0.6f;  // Posição do "near plane"
        float farplane  = -45.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 199 do
            // documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // veja slide 206 do documento "Aula_09_Projecoes.pdf".
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Desenhamos os modelos
        std::vector<glm::vec3>::iterator itpos;   // iterator para as posicoes
        std::vector<glm::vec3>::iterator itanm;   // iterator para as animacoes
        std::vector<glm::vec3>::iterator itangle; // iterator para os angulos

        for(const auto& dict : g_VirtualScene)
        {
            SceneObject* obj = &g_VirtualScene[dict.second.name];
            if(obj->id != 5 && obj->id != 6) // se não for o disco ou o cone
            {
                itanm   = obj->animation.begin();
                itangle = obj->angles.begin();

                for(itpos = obj->pos.begin(); itpos != obj->pos.end(); itpos++)
                {
                    unsigned int qtyObj = obj->pos.size();

                    glm::vec3 anm = *itanm;
                    glm::vec3 angles = *itangle;

                    if(obj->id == 10) // se o obj e uma vaca
                    {
                        if(spaceKeyPressed) // se a nave esta em trabalho de abducao
                        {
                            glm::vec3 pos = *itpos;

                            // Verifica intersecao com o disco (a vaca chegou na nave)
                            if(cowUp && isIntersecting(obj, &pos, &angles, &g_VirtualScene["Disco"]))
                            {
                                printf("Vc abduziu uma vaca.\n");

                                // a vaca some
                                obj->pos.erase(itpos);
                                obj->angles.erase(itangle);
                                obj->animation.erase(itanm);

                                cowUp = false;
                                cowCount++;
                            }
                            // Verifica interseccao com o cone
                            else if(isIntersecting(obj, &pos, &angles, &g_VirtualScene["Cone"]))
                            {
                                cowUp = true;
                                anm = glm::vec3(0.0f, delta, 0.0f); // a vaca sobe
                            }
                        }
                        else if((*itpos).y > 0.184919f)
                        {
                                // parou de apertar espaco antes da vaca chegar na nave
                                (*itpos).y -= delta; // a vaca cai... e vive
                                cowUp = false;
                        }
                    }

                    // Atualizamos a posicao do obj
                    if(qtyObj == obj->pos.size())
                    {
                        (*itpos).x += anm.x;
                        if((*itpos).x > tamanhoNivel || (*itpos).x < -tamanhoNivel)   // Se chegou à borda do mapa, inverte movimento
                        {
                            (*itanm).x = -(*itanm).x;
                            (*itanm).z = -(*itanm).z;
                            (*itangle).y += rad(180);
                        }

                        (*itpos).y += anm.y;

                        (*itpos).z += anm.z;
                        if((*itpos).z > tamanhoNivel || (*itpos).z < -tamanhoNivel)   // Se chegou à borda do mapa, inverte movimento
                        {
                            (*itanm).x = -(*itanm).x;
                            (*itanm).z = -(*itanm).z;
                            (*itangle).y += rad(180);
                        }

                        model = Matrix_Translate((*itpos).x, (*itpos).y, (*itpos).z)
                              * Matrix_Rotate_Z(angles.z)
                              * Matrix_Rotate_X(angles.x)
                              * Matrix_Rotate_Y(angles.y);

                        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                        DrawVirtualObject(obj->name.c_str());
                    }

                    if(obj->hasRandAngle) itangle++;
                    if(obj->hasAnm) itanm++;
                }
            }
        }

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos quantas vacas foram abduzidas
        TextRendering_ShowCowCount(window);

        // Atualiza o tempo da partida
        timer = TIME_LIMIT - ((clock() - init_clock) / (double) CLOCKS_PER_SEC);

        // Mostra o tempo da partida
        TextRendering_ShowTimer(window);

        if(timer <= 0)
        {
            g_CameraPhi = rad(-90);
            TextRendering_ShowGameOver(window);
        }

        // O framebuffer onde a OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        glfwSwapBuffers(window);

        // Verifica com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos os recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    char filepath[100];
    strcpy(filepath, filename);
    strcat(filepath, ".png");

    printf("Carregando imagem \"%s\"... ", filepath);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filepath, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "\nERROR: Cannot open image file \"%s\".\n\n", filepath);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 160 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Setamos as variáveis de refletância do fragment shader
    glm::vec3 ambient  = g_VirtualScene[object_name].ambient;
    glm::vec3 diffuse  = g_VirtualScene[object_name].diffuse;
    glm::vec3 specular = g_VirtualScene[object_name].specular;
    glUniform3f(ka_uniform, ambient.x, ambient.y, ambient.z);
    glUniform3f(kd_uniform, diffuse.x, diffuse.y, diffuse.z);
    glUniform3f(ks_uniform, specular.x, specular.y, specular.z);

    int obj_id = g_VirtualScene[object_name].id;
    glUniform1i(obj_id_uniform, obj_id);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slide 216 do documento
// "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");
    ka_uniform              = glGetUniformLocation(program_id, "ka");
    kd_uniform              = glGetUniformLocation(program_id, "kd");
    ks_uniform              = glGetUniformLocation(program_id, "ks");
    obj_id_uniform          = glGetUniformLocation(program_id, "obj_id");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model, int* k, const char* filename)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];

                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if ( model->attrib.normals.size() >= (size_t)3*idx.normal_index )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( model->attrib.texcoords.size() >= (size_t)2*idx.texcoord_index )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.id             = *k;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        // Refletancias
        theobject.ambient  = glm::vec3(model->materials[shape].ambient[0], model->materials[shape].ambient[1], model->materials[shape].ambient[2]);
        theobject.diffuse  = glm::vec3(model->materials[shape].diffuse[0], model->materials[shape].diffuse[1], model->materials[shape].diffuse[2]);
        theobject.specular = glm::vec3(model->materials[shape].specular[0], model->materials[shape].specular[1], model->materials[shape].specular[2]);

        // Carregar posicoes, angulos e animacoes
        LoadObjAttr(filename, &theobject);

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 2)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    (*k) += 1;

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!

    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatóriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    return program_id;
}

// Definição da função que será chamada sempre que a janela for redimensionada,
// por consequência alterando o tamanho do "framebuffer" (região de memória
// onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slide 183 do
    // documento "Aula_03_Rendering_Pipeline_Grafico.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 199
    // do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // ser divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = (M_PI/2.0f) - 0.01f;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    if (g_CameraDistance < 1.0f)
        g_CameraDistance = 1.0f;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    // Ativa ou não as variáveis de movimento
    if (key == GLFW_KEY_W && action == GLFW_PRESS && !spaceKeyPressed)
    {
        wKeyPressed = true;
        wKeyReleased = false;
    }

    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
        wKeyPressed = false;
        wKeyReleased = true;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS && !spaceKeyPressed)
    {
        sKeyPressed = true;
        sKeyReleased = false;
    }

    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
        sKeyPressed = false;
        sKeyReleased = true;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS && !spaceKeyPressed)
    {
        aKeyPressed = true;
        aKeyReleased = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        aKeyPressed = false;
        aKeyReleased = true;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS && !spaceKeyPressed)
    {
        dKeyPressed = true;
        dKeyReleased = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        dKeyPressed = false;
        dKeyReleased = true;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        spaceKeyPressed = true;

        // Se expaço é precionado, o disco para
        wKeyPressed = false;
        sKeyPressed = false;
        aKeyPressed = false;
        dKeyPressed = false;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
    {
        spaceKeyPressed = false;

        // Recupera informação de movimento
        if(!wKeyReleased)
            wKeyPressed = true;
        if(!aKeyReleased)
            aKeyPressed = true;
        if(!sKeyReleased)
            sKeyPressed = true;
        if(!dKeyReleased)
            dKeyPressed = true;
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
    {
        if(timer <= 0)
            reset(); // Reiniciar o jogo
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Escrevemos na tela quantas vacas já foram abduzidas
void TextRendering_ShowCowCount(GLFWwindow* window)
{
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    char buffer[23];
    snprintf(buffer, 23, "VACAS ABDUZIDAS: %d\n", cowCount);

    TextRendering_PrintString(window, buffer, -1.0f+charwidth, 0.98f-lineheight, 1.5f);
}

// Escrevemos na tela o tempo da partida
void TextRendering_ShowTimer(GLFWwindow* window)
{
    if(timer <= 0)
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    char buffer[15];
    snprintf(buffer, 15, "TEMPO: %ds\n", (int)timer);

    TextRendering_PrintString(window, buffer, 1.0f-16*charwidth, 0.98f-lineheight, 1.5f);
}

// Escrevemos na tela game over
void TextRendering_ShowGameOver(GLFWwindow* window)
{
    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    char buffer[10];
    snprintf(buffer, 10, "GAME OVER\n");
    TextRendering_PrintString(window, buffer, -0.06f-(35*charwidth)/2, lineheight, 5.0f);

    char subbuffer[35];
    snprintf(subbuffer, 35, "Pressione ENTER para reiniciar\n");
    TextRendering_PrintString(window, subbuffer, -0.06f-(35*charwidth)/2, -lineheight-0.01f, 1.5f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void LoadObjAttr(const char* filename, SceneObject* obj)
{
    // Carregar coeficientes de animacao
    printf("Carregando animações do modelo... ");

    char anmpath[100];
    strcpy(anmpath, filename);
    strcat(anmpath, ".anm");

    glm::vec3 animation = glm::vec3(0.0f, 0.0f, 0.0f);
    std::ifstream anm;
    anm.open(anmpath);

    if(!anm)
    {
        obj->hasAnm = false;
        obj->animation.push_back(animation);
        printf("Erro ao ler os coeficientes de animação do modelo.\n");
    }
    else
    {
        obj->hasAnm = true;
        std::string line;
        float x, y, z;

        std::getline(anm, line);
        std::istringstream coord(line);

        if (!(coord >> x >> y >> z))
            printf("\nOcorreu um erro durante a leitura dos coeficientes de animação do modelo.\n");

        animation = glm::vec3(x, y, z);
        anm.close();

        printf("OK\n");
    }

    // Carregar posicoes e alinhar angulos e animacoes
    printf("Carregando as posições do modelo... ");

    char pospath[100];
    strcpy(pospath, filename);
    strcat(pospath, ".pos");

    std::ifstream posfile;
    posfile.open(pospath);

    if(!posfile)
        printf("Erro ao ler as posições do modelo.\n");
    else
    {
        std::string line;
        float x, y, z;

        // Primeira linha do arquivo define a rotacao do obj
        std::getline(posfile, line);

        std::istringstream rot(line);
        rot >> x >> y >> z;
        // Verifica se os angulos de rotacao são aleatorios
        obj->hasRandAngle = false;
        glm::vec3 angles = glm::vec3(rad(x), rad(y), rad(z));

        if(x == 360.0f && y == 360.0f && z == 360.0f)
            obj->hasRandAngle = true;
        else
            obj->angles.push_back(angles);

        // Posicoes
        while(std::getline(posfile, line))
        {
            std::istringstream coord(line);

            if (!(coord >> x >> y >> z))
            {
                printf("\nOcorreu um erro durante a leitura das posições do modelo.\n");
                break;
            }

            glm::vec3 one_pos = glm::vec3(x, y, z);
            obj->pos.push_back(one_pos);

            // Para cada posicao há uma rotacao
            if(obj->hasRandAngle)
            {
                angles = glm::vec3(0.0f, randAngle(), 0.0f);
                obj->angles.push_back(angles);
            }

            if(obj->hasAnm)
                obj->animation.push_back(getAnimation(&animation, &angles));
        }

        posfile.close();
        printf("OK\n");
    }
}

glm::vec3 getAnimation(glm::vec3* anm, glm::vec3* angles)
{
    glm::vec3 animation;

    animation.x = (*anm).x * cos((*angles).y);
    animation.z = (*anm).z * -sin((*angles).y);
    animation.y = 0.0f;

    return animation;
}

float randAngle()
{
    float radians = (float) (rand() % 360);
    return rad(radians);
}

float rad(float d)
{
    return (d * M_PI) / 180;
}

bool isIntersecting(SceneObject* obj1, glm::vec3* coord, glm::vec3* ang, SceneObject* obj2)
{
    glm::vec3 bbmax1 = obj1->bbox_max;
    glm::vec3 bbmin1 = obj1->bbox_min;

    glm::vec3 bbmax2 = obj2->bbox_max;
    glm::vec3 bbmin2 = obj2->bbox_min;

    glm::mat4 model;
    std::vector<glm::vec3>::iterator itpos2;
    std::vector<glm::vec3>::iterator itang2;

    glm::vec3 pos = *coord;
    glm::vec3 angles = *ang;

    model = Matrix_Translate(pos.x, pos.y, pos.z)
          * Matrix_Rotate_Z(angles.z)
          * Matrix_Rotate_X(angles.x)
          * Matrix_Rotate_Y(angles.y);

    // Pontos max e min da bounding box já transformados pela matriz do modelo
    glm::vec4 max1 = model * glm::vec4(bbmax1.x, bbmax1.y, bbmax1.z, 1.0f);
    glm::vec4 min1 = model * glm::vec4(bbmin1.x, bbmin1.y, bbmin1.z, 1.0f);

    itang2 = obj2->angles.begin();
    for(itpos2 = obj2->pos.begin(); itpos2 != obj2->pos.end(); itpos2++)
    {
        pos = *itpos2;
        model = Matrix_Translate(pos.x, pos.y, pos.z)
              * Matrix_Rotate_Z((*itang2).z)
              * Matrix_Rotate_X((*itang2).x)
              * Matrix_Rotate_Y((*itang2).y);

        glm::vec4 max2 = model * glm::vec4(bbmax2.x, bbmax2.y, bbmax2.z, 1.0f);
        glm::vec4 min2 = model * glm::vec4(bbmin2.x, bbmin2.y, bbmin2.z, 1.0f);

        // Testa se obj1 tem um canto inferior sobre do obj2
        if (min1.x >= min2.x && min1.x <= max2.x &&
            min1.y >= min2.y && min1.y <= max2.y &&
            min1.z >= min2.z && min1.z <= max2.z)
            return true;

        // Testa se obj1 tem um canto superior sobre do obj2
        if (max1.x >= min2.x && max1.x <= max2.x &&
            max1.y >= min2.y && max1.y <= max2.y &&
            max1.z >= min2.z && max1.z <= max2.z)
            return true;

        // Testa se obj1 está "dentro" do obj2
        if (min1.x >= min2.x && min1.x <= max2.x &&
            max1.y >= min2.y && max1.y <= max2.y &&
            min1.z >= min2.z && min1.z <= max2.z)
            return true;

        // Testa se obj1 está "dentro" do obj2
        if (max1.x >= min2.x && max1.x <= max2.x &&
            min1.y >= min2.y && min1.y <= max2.y &&
            max1.z >= min2.z && max1.z <= max2.z)
            return true;

        if(obj2->hasRandAngle) itang2++;
    }

    return false;
}

void reset()
{
    // Carregamos as vacas novamente
    int k = 10;
    const char* basepath = "../../data/";
    const char* filepath = "../../data/vaca";

    ObjModel objmodel(filepath, basepath);
    ComputeNormals(&objmodel);
    BuildTrianglesAndAddToVirtualScene(&objmodel, &k, filepath);

    g_CameraPhi = 0.5209f; // colocamos a  camera olhando para a nave novamente
    init_clock = clock();  // zeramos o cronometro
    cowCount = 0;          // zeramos a contagem de vacas abduzidas
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
