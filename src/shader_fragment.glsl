#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da cor de cada vértice, definidas em "shader_vertex.glsl" e
// "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Parâmetros de refletância
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;

// Identificacao do obj
uniform int obj_id;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImageK
    vec3 Kd0;
    float alpha = 1.0f;

    switch(obj_id)
    {
        case 0:
            Kd0 = texture(TextureImage0, texcoords).rgb;
            break;

        case 1:
            Kd0 = texture(TextureImage1, texcoords).rgb;
            break;

        case 2:
            Kd0 = texture(TextureImage2, texcoords).rgb;
            break;

        case 3:
            Kd0 = texture(TextureImage3, texcoords).rgb;
            break;

        case 4:
            Kd0 = texture(TextureImage4, texcoords).rgb;
            break;
        
        case 5: // cone
            Kd0 = vec3(255.0f, 30.0f, 30.0f);
            alpha = 0.2f;
            break;

        case 6:
            Kd0 = texture(TextureImage5, texcoords).rgb;
            break;

        case 7:
            Kd0 = texture(TextureImage6, texcoords).rgb;
            break;

        case 8:
            Kd0 = texture(TextureImage7, texcoords).rgb;
            break;

        case 9:
            Kd0 = texture(TextureImage8, texcoords).rgb;
            break;

        case 10:
            Kd0 = texture(TextureImage9, texcoords).rgb;
            break;

        default:
            Kd0 = vec3(0.5f, 0.5f, 0.5f);
            break;
    }

    //Kd0 = texture(TextureImage1, texcoords).rgb;

    // Equação de Iluminação
    float lambert = 0;
    if(obj_id != 5) // Cone não recebe shading
        lambert = max(0, dot(n,l));
    
    Kd0 = Kd0 * (lambert + 0.01);
        
    color = vec4(Kd0.x, Kd0.y, Kd0.z, alpha);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec4(1.0,1.0,1.0,1.0)/2.2);
}
