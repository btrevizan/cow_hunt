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

// Parâmetros que definem as propriedades espectrais da superfície
vec3 Kd=vec3(0,0,0); // Refletância difusa
vec3 Ks=vec3(0,0,0); // Refletância especular
vec3 Ka=vec3(0,0,0); // Refletância ambiente
float q = 1; // Expoente especular para o modelo de iluminação de Phong

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

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*dot(n, l);

    float alpha = 1.0f;

    switch(obj_id)
    {
        case 0: // Árvore
            q = 32;
            Ks = vec3(0.3500,0.3500,0.3500);
            Kd = texture(TextureImage0, texcoords).rgb;
            break;

        case 1: // Banheiro
            q = 9;
            Ks = vec3(0.4680,0.4680,0.4680);
            Kd = texture(TextureImage1, texcoords).rgb;
            break;

        case 2: // Casa
            q = 20;
            Ks = vec3(0.3,0.3,0.3);
            Kd = texture(TextureImage2, texcoords).rgb;
            break;

        case 3: // Celeiro
            q = 20;
            Ks = vec3(0.3,0.3,0.3);
            Kd = texture(TextureImage3, texcoords).rgb;
            break;

        case 4: // Chão
            q = 5;
            Kd = texture(TextureImage4, texcoords).rgb;
            Ks = vec3(0.1,0.1,0.1);
            break;

        case 5: // cone
            q = 1;
            Ks = vec3(0.0,0.0,0.0);
            Kd = vec3(255.0f, 30.0f, 30.0f);
            alpha = 0.3f;
            break;

        case 6: // disco
            q = 40;
            Kd = texture(TextureImage5, texcoords).rgb;
            Ks = vec3(0.8550,0.8550,0.8550);
            break;

        case 7: // Silo
            q = 27;
            Ks = vec3(0.8280,0.8280,0.8280);
            Kd = texture(TextureImage6, texcoords).rgb;
            break;

        case 8: // Trator
            q = 20;
            Ks = vec3(0.8100,0.8100,0.8100);
            Kd = texture(TextureImage7, texcoords).rgb;
            break;

        case 9: // Turbina
            q = 20;
            Ks = vec3(0.8100,0.8100,0.8100);
            Kd = texture(TextureImage8, texcoords).rgb;
            break;

        case 10:    // Vaca
            q = 1;
            Ks = vec3(0.0,0.0,0.0);
            Kd = texture(TextureImage9, texcoords).rgb;
            break;

        default:
            q = 1;
            Ks = vec3(0.0,0.0,0.0);
            Kd = vec3(255.0f, 0.0f, 255.0f);    // Se não há padrão, recebe magenta para fácil visualização
            break;
    }

    if(Kd.x<0.1 && Kd.y<0.1 && Kd.z<0.1 && obj_id!=6)    // Se está na sombra gerada pela textura, não reflete (a não ser no disco)
        Ks = vec3(0.0,0.0,0.0);

    // Lambert
    float lambert = 0;
    if(obj_id != 5) // Cone não recebe shading
        lambert = max(0, dot(n,l)) + 0.2;

    // Termo especular utilizando o modelo de iluminação de Phong
    float phong_specular_term = pow(max(0.0, dot(r, v)), q);

    // Espectro da fonte de iluminação
    vec3 light_spectrum = vec3(1.0,1.0,1.0); // PREENCH AQUI o espectro da fonte de luz



    vec3 color3 = Kd * (lambert + 0.01) * light_spectrum
                + Ks * light_spectrum * phong_specular_term;

    color = vec4(color3.x, color3.y, color3.z, alpha);

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec4(1.0,1.0,1.0,1.0)/2.2);
}
