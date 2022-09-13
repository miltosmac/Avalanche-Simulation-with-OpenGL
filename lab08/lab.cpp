// Include C++ headers
#include <iostream>
#include <string>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Shader loading utilities and other
#include <common/shader.h>
#include <common/util.h>
#include <common/camera.h>
#include <model.h>
#include <texture.h>
#include "FountainEmitter.h"
#include "MarchingCubes.h"
#include "RayCasting.h"

//TODO delete the includes afterwards
#include <chrono>
using namespace std::chrono;
//

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using namespace std;
using namespace glm;

// Function prototypes
void initialize();
void createContext();
void mainLoop();
void free();

#define W_WIDTH 1024
#define W_HEIGHT 768
#define TITLE "Lab 08"

// Global variables
GLFWwindow* window;
Camera* camera;
GLuint particleShaderProgram, normalShaderProgram, cubesShaderProgram;
GLuint projectionMatrixLocation, viewMatrixLocation, modelMatrixLocation, CubeModelMatrixLocation, projectionAndViewMatrix, CubeProjectionAndViewMatrix;
GLuint translationMatrixLocation, rotationMatrixLocation, scaleMatrixLocation;
GLuint SnowTexture, diffuceColorSampler, snowflake_Texture, TerrainTexture, TerrainColorSampler, SnowColourSampler, HeightMapSampler;

glm::vec3 slider_emitter_pos(0.0f, 20.0f, 0.0f);
int particles_slider = 10000;
void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods);

bool game_paused = false;

bool use_sorting = false;
bool use_rotations = true;

float height_threshold = 100.0f;

glm::vec4 background_color = glm::vec4(0.5f, 0.5f, 0.5f, 0.0f);

void renderHelpingWindow() {
    static int counter = 0;

    ImGui::Begin("Helper Window");                          // Create a window called "Hello, world!" and append into it.

    ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

    ImGui::ColorEdit3("Background", &background_color[0]);

    ImGui::SliderFloat("x position", &slider_emitter_pos[0], -30.0f, 30.0f);
    ImGui::SliderFloat("y position", &slider_emitter_pos[1], -30.0f, 30.0f);
    ImGui::SliderFloat("z position", &slider_emitter_pos[2], -30.0f, 30.0f);
    ImGui::SliderFloat("height", &height_threshold, 0, 200);

    ImGui::SliderInt("particles", &particles_slider, 0, 40000);


    if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
    ImGui::SameLine();
    ImGui::Text("counter = %d", counter);

    ImGui::Checkbox("Use sorting", &use_sorting);
    ImGui::Checkbox("Use rotations", &use_rotations);

    ImGui::Text("Performance %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
 
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void createContext() {
    particleShaderProgram = loadShaders(
        "ParticleShader.vertexshader",
        "ParticleShader.fragmentshader");


    normalShaderProgram = loadShaders(
        "StandardShading.vertexshader",
        "StandardShading.fragmentshader");

    cubesShaderProgram = loadShaders(
        "CubeShader.vertexshader",
        "CubeShader.fragmentshader"
    );

    projectionAndViewMatrix = glGetUniformLocation(particleShaderProgram, "PV");

    translationMatrixLocation = glGetUniformLocation(normalShaderProgram, "T");
    rotationMatrixLocation = glGetUniformLocation(normalShaderProgram, "R");
    scaleMatrixLocation = glGetUniformLocation(normalShaderProgram, "S");

    modelMatrixLocation = glGetUniformLocation(normalShaderProgram, "M");
    
    viewMatrixLocation = glGetUniformLocation(normalShaderProgram, "V");
    projectionMatrixLocation = glGetUniformLocation(normalShaderProgram, "P");

    diffuceColorSampler = glGetUniformLocation(particleShaderProgram, "texture0");
    TerrainColorSampler = glGetUniformLocation(normalShaderProgram, "terrain_texture");

    HeightMapSampler = glGetUniformLocation(normalShaderProgram, "height_map");

    CubeProjectionAndViewMatrix = glGetUniformLocation(cubesShaderProgram, "PV");
    CubeModelMatrixLocation = glGetUniformLocation(cubesShaderProgram, "M");
    SnowColourSampler = glGetUniformLocation(cubesShaderProgram, "snow_texture");
    
    SnowTexture = loadSOIL("snow.jpg");
    snowflake_Texture = loadSOIL("snowflake_1.png");
    TerrainTexture = loadSOIL("Color.png");
    glfwSetKeyCallback(window, pollKeyboard);
}

void free() {
    glDeleteProgram(particleShaderProgram);
    glfwTerminate();
}



void ReadBMP(char* filename, float **height_map, float& max_length)
{
    int i;
    FILE* f = fopen(filename, "rb");

    if (f == NULL)
        throw "Argument Exception";

    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

    // extract image height and width from header
    int width = *(int*)&info[18];
    int height = *(int*)&info[22];
    
    cout << endl;
    cout << "  Name: " << filename << endl;
    cout << " Width: " << width << endl;
    cout << "Height: " << height << endl;

    int row_padded = (width * 3 +3) & (~3);
    unsigned char* data = new unsigned char[row_padded];
    unsigned char tmp;

    max_length = 0.0f; 

    for (int i = 0; i < height; i++)//Starting from bottom left of the BMP
    {
        fread(data, sizeof(unsigned char), row_padded, f);//Reads the one 'row' of bits every time
        for (int j = 0; j < width * 3; j += 3)
        {
            // Convert (B, G, R) to (R, G, B)
            tmp = data[j];
            data[j] = data[j + 2];
            data[j + 2] = tmp;

            vec3 colour = vec3((int)data[j], (int)data[j + 1], (int)data[j + 2]);
            //cout << "R: " << (int)data[j] << " G: " << (int)data[j + 1] << " B: " << (int)data[j + 2] << endl;
            height_map[i][j / 3] = length(colour);
            if (length(colour) > max_length) {
                max_length = length(colour);
            }
        }
    }
    /* We created a 2D Array of the Heights.
    Next we find the max of the colour length
    Finally we "normalize" the colour lengths*/
    for (int i = 0; i < height; i++) { 
        for (int j = 0; j < width; j++) {
            height_map[i][j] = height_map[i][j]/max_length; 
        }
    }
    fclose(f);
    return ;
};

void ReadNormalBMP(char* filename, glm::vec3** NormalMap) {
    int i;
    FILE* f = fopen(filename, "rb");

    if (f == NULL)
        throw "Argument Exception";

    unsigned char info[54];
    fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

    // extract image height and width from header
    int width = *(int*)&info[18];
    int height = *(int*)&info[22];
    int row_padded = (width * 3 + 3) & (~3);
    unsigned char* data = new unsigned char[row_padded];
    unsigned char tmp;

    for (int i = 0; i < height; i++)//Starting from bottom left of the BMP
    {
        fread(data, sizeof(unsigned char), row_padded, f);//Reads the one 'row' of bits every time
        for (int j = 0; j < width * 3; j += 3)
        {
            // Convert (B, G, R) to (R, G, B)
            tmp = data[j];
            data[j] = data[j + 2];
            data[j + 2] = tmp;

            NormalMap[i][j/3] = vec3(data[j], data[j + 1], data[j + 2]);
            }
        }

    fclose(f);
    return;
};



void GetVertices(Drawable* model, float& max_height, vec2& Max_Vector, vec2& Min_Vector) {
    std::vector<vec3> Vertices;
    //GLint width, height;
    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, HeightMap);
    //glUniform1i(HeightMapSampler, 1);
    Vertices.clear();
    //glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
    //glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    max_height = model->vertices[1].y;
    Max_Vector = vec2(model->vertices[1].x, model->vertices[1].z);
    Min_Vector = vec2(model->vertices[1].x, model->vertices[1].z);
    for (int i = 0; i <= (model->vertices.size()) - 1; i++) {
        Vertices.push_back(model->vertices[i]);
        if (Vertices[i].y > max_height) {
            max_height = Vertices[i].y;
        }
        if (Vertices[i].x > Max_Vector.x) {
            Max_Vector.x = Vertices[i].x;
        }
        if (Vertices[i].z > Max_Vector.y) {
            Max_Vector.y = Vertices[i].z;
        }
        if (Vertices[i].x < Min_Vector.x) {
            Min_Vector.x = Vertices[i].x;
        }
        if (Vertices[i].z < Min_Vector.y) {
            Min_Vector.y = Vertices[i].z;
        }
    }
};

void Update_MCubes_Triangles(vector<vec3> & Vertices) {
    if (Vertices.size() != 0) {
        GLuint VBO, VAO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * Vertices.size(), &Vertices[0], GL_STATIC_DRAW);

        //Vertices
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);

        glDrawArrays(GL_TRIANGLES, 0, Vertices.size());

        //glDrawElements(GL_TRIANGLES, Vertices.size(), GL_UNSIGNED_INT, NULL);
    }
};


void mainLoop() {

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    camera->position = vec3(-11.7f, 10.6f, -33.0f);
    auto* monkey = new Drawable("suzanne.obj");
    auto* quad = new Drawable("quad.obj");
    auto* Mountain = new Drawable("Mountain.obj");


    FountainEmitter f_emitter = FountainEmitter(quad, particles_slider);
    //OrbitEmitter o_emitter = OrbitEmitter(quad, particles_slider, 10, 80);

    mat4 Terrain_Model_M = mat4(1);
    std::vector<vec3> Triangle_Vertices;
    Triangle_Vertices.clear();
    float max_h,max_length;
    glm::vec2 max_v, min_v;
    GetVertices(Mountain, max_h, max_v, min_v);

    //std::vector<vec3> height_map;
    float** HeightMap;
    HeightMap = new float* [2048];
    for (int i = 0; i < 2048; i++) {
        HeightMap[i] = new float[2048];
    }

    vec3** NormalMap;
    NormalMap = new vec3* [2048];
    for (int i = 0; i < 2048; i++) {
        NormalMap[i] = new vec3[2048];
    }

    float x_div = 500.0f;
    float z_div = 500.0f;
    float y_div = (14.8f / 80.0f) * x_div;
    //Let's Create the Grid for the Marching Cubes Algorithm
    bool*** SquareGrid;
    SquareGrid = new bool** [x_div];
    for (int i = 0; i < x_div; i++) {
        SquareGrid[i] = new bool* [z_div];
        for (int j = 0; j < z_div; j++) {
            SquareGrid[i][j] = new bool[(int)floor(y_div+1)];
            for (int k = 0; k < (int)floor(y_div + 1); k++) {
                SquareGrid[i][j][k] = false;
            }
        }
    }//80 meters of length (& width) are divided in 100 (& 100 respectively) so 14.8 meters of height should be 12 
    


    float Normal_max_length;
    ReadBMP("Height_Map_Try_6.bmp", HeightMap,max_length);
    ReadNormalBMP("Normal_Try_2.bmp", NormalMap);
    

    float t = glfwGetTime();
    do {
        f_emitter.changeParticleNumber(particles_slider);
        f_emitter.emitter_pos = slider_emitter_pos;
        f_emitter.use_rotations = use_rotations;
        f_emitter.use_sorting = use_sorting;
        f_emitter.height_threshold = height_threshold;

        float currentTime = glfwGetTime();
        float dt = currentTime - t;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        glClearColor(background_color[0], background_color[1], background_color[2], background_color[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(particleShaderProgram); // Use shader programm for the particles

        // camera
        camera->update();
        mat4 projectionMatrix = camera->projectionMatrix;
        mat4 viewMatrix = camera->viewMatrix;

        auto PV = projectionMatrix * viewMatrix;
        glUniformMatrix4fv(projectionAndViewMatrix, 1, GL_FALSE, &PV[0][0]);

        //*/ Use particle based drawing
        glActiveTexture(GL_TEXTURE0);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, snowflake_Texture);
        glBindTexture(GL_TEXTURE_2D, snowflake_Texture);
        glUniform1i(diffuceColorSampler, 0);
        if(!game_paused) {
            f_emitter.updateParticles(currentTime, dt, HeightMap,NormalMap, SquareGrid, max_h, max_v, min_v, Triangle_Vertices, camera->position);
        }
        f_emitter.renderParticles();
        if (!game_paused && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
            glm::vec3 ray_origin=camera->position;
            glm::vec3 ray_direction;
            double xpos, ypos;
            //getting cursor position
            glfwGetCursorPos(window, &xpos, &ypos);
            ScreenPosToWorldRay(xpos, ypos, W_WIDTH, W_HEIGHT, viewMatrix, projectionMatrix, ray_direction);
            f_emitter.StartAvalanche(currentTime, dt,ray_origin,ray_direction);
        }
        //*/

        // Use shader program for the terrain

        glUseProgram(normalShaderProgram);
        
        glUniformMatrix4fv(projectionAndViewMatrix, 1, GL_FALSE, &PV[0][0]);
   
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, TerrainTexture);
        glUniform1i(TerrainColorSampler, 0);

        Mountain->bind();
        Mountain->draw();

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &Terrain_Model_M[0][0]);

        //Use Shader program for the Marching Cubes

        glUseProgram(cubesShaderProgram);

        glUniformMatrix4fv(projectionAndViewMatrix, 1, GL_FALSE, &PV[0][0]);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, SnowTexture);
        glUniform1i(SnowColourSampler, 0);

        glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, &Terrain_Model_M[0][0]);
        Update_MCubes_Triangles(Triangle_Vertices);
        
        renderHelpingWindow();
        glfwPollEvents();
        glfwSwapBuffers(window);
        t = currentTime;
        
    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);
}

void initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        throw runtime_error("Failed to initialize GLFW\n");
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(W_WIDTH, W_HEIGHT, TITLE, NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        throw runtime_error(string(string("Failed to open GLFW window.") +
                            " If you have an Intel GPU, they are not 3.3 compatible." +
                            "Try the 2.1 version.\n"));
    }

    glfwMakeContextCurrent(window);

    // Start GLEW extension handler
    glewExperimental = GL_TRUE;

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        glfwTerminate();
        throw runtime_error("Failed to initialize GLEW\n");
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Hide the mouse and enable unlimited movement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, W_WIDTH / 2, W_HEIGHT / 2);

    // Gray background color
    glClearColor(0.5f, 0.5f, 0.5f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    // glEnable(GL_CULL_FACE);
    // glFrontFace(GL_CW);
    // glFrontFace(GL_CCW);

    // enable point size when drawing points
    glEnable(GL_PROGRAM_POINT_SIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Log
    logGLParameters();

    // Create camera
    camera = new Camera(window);

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double xpos, double ypos) {
                                 camera->onMouseMove(xpos, ypos);
                             }
    );
}
void pollKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Task 2.1:
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
        game_paused = !game_paused;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        if (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            camera->active = true;
        }
        else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            camera->active = false;
        }

    }
}

int main(void) {
    try {
        initialize();
        createContext();
        mainLoop();
        free();
    } catch (exception& ex) {
        cout << ex.what() << endl;
        getchar();
        free();
        return -1;
    }

    return 0;
}



