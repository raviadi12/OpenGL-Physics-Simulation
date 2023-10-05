#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <GL/glew.h>
#include <shader.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <stb_image.h>
#include <vector>
#include <map>
#include <string>
#include <ft2build.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sys/types.h>
#include <Winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#include <thread>
#include <Windows.h>
#include FT_FREETYPE_H  

std::atomic<bool> runSocketThread(true);

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);
void RenderSineWave(Shader& shader, float amplitude, float freq, float scaley, glm::vec3 color);

// settings
const unsigned int SCR_WIDTH = 1440;
const unsigned int SCR_HEIGHT = 900;
int windowedWidth = 1440, windowedHeight = 900;

float amplitude = 0.5f;
float velocity = 0.0f;
float acceleration = 0.0f;

float received_socket = 0.0f;

float stiffness = 50.0f;
float damping = 2.6f;
float targetAmplitude = 0.5f;

float x_offset = 0.0f;
float y_offset = 0.0f;

float scaley = 1.0f;

float freq = 1.0f;

float force_scale = 105.f;


float time_mod = 1.0f;

bool upKeyPressed = false;
bool downKeyPressed = false;

GLFWmonitor* monitor = nullptr;
bool isFullscreen = false;

bool pKeyPressed = false;
bool tKeyPressed = false;

void SmoothIncrease(float& currentValue, float targetValue, float rate)
{
    currentValue += (targetValue - currentValue) * rate;
}

float targetTimeMod = time_mod;  // Initialize target value

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int VAO, VBO;


void update_target_amplitude() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed";
        return;
    }

    SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET) {
        std::cerr << "Failed to create socket";
        WSACleanup();
        return;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(3000); // port number
    server_address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"); // localhost

    if (connect(sockfd, (struct sockaddr*)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        std::cerr << "Connection failed";
        closesocket(sockfd);
        WSACleanup();
        return;
    }

    char buffer[256];
    while (runSocketThread) { // Keep running to receive updates
        ZeroMemory(buffer, sizeof(buffer));
        int bytesReceived = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Recv error";
            break;
        }

        std::string targetAmplitudeStr(buffer, bytesReceived);
        float targetAmplitude = std::stof(targetAmplitudeStr);
        // Update your targetAmplitude here
        received_socket = targetAmplitude;
    }

    closesocket(sockfd);
    WSACleanup();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    float force = 0.0f;
    float speed = 0.05f;
    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_UP)
        {
            upKeyPressed = true;
            std::cout << "Key Pressed";
        }
        else if (key == GLFW_KEY_DOWN)
        {
            downKeyPressed = true;
        }
        else if (key == GLFW_KEY_LEFT)
        {
            freq -= speed;
        }
        else if (key == GLFW_KEY_RIGHT)
        {
            freq += speed;
        }
        else if (key == GLFW_KEY_A)
        {
            stiffness -= 2.5;
        }
        else if (key == GLFW_KEY_D)
        {
            stiffness += 2.5;
        }
        else if (key == GLFW_KEY_W)
        {
            damping -= 2.5;
        }
        else if (key == GLFW_KEY_S)
        {
            damping += 2.5;
        }
        else if (key == GLFW_KEY_E)
        {
            targetAmplitude -= 0.1;
        }
        else if (key == GLFW_KEY_Q)
        {
            targetAmplitude += 0.1;
        }
        else if (key == GLFW_KEY_T)
        {
            targetTimeMod -= 0.1;
        }
        else if (key == GLFW_KEY_P)
        {
            targetTimeMod += 0.1;
        }
        else if (key == GLFW_KEY_F)
        {
            force_scale -= 10.0;
        }
        else if (key == GLFW_KEY_G)
        {
            force_scale += 10.0;
        }
        else if (key == GLFW_KEY_F11)
        {
            if (!isFullscreen)
            {
                monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
                glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            }
            else
            {
                glfwSetWindowMonitor(window, nullptr, 0, 0, windowedWidth, windowedHeight, 0);
                glfwMakeContextCurrent(window);  // Make sure to update the context
            }
            isFullscreen = !isFullscreen;
        }
    }
    else if (action == GLFW_RELEASE)
    {
        if (key == GLFW_KEY_UP)
        {
            upKeyPressed = false;
        }
        else if (key == GLFW_KEY_DOWN)
        {
            downKeyPressed = false;
        }
    }
}


unsigned int sineWaveVAO, sineWaveVBO;

int main(void);  // Forward declaration of your main function

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return main();
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);




#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------

    glfwWindowHint(GLFW_SAMPLES, 1.0);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Spring Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glEnable(GL_MULTISAMPLE);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers

    if (glewInit() != GLEW_OK)
        std::cout << "Error!" << std::endl;

    // OpenGL state
    // ------------
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // compile and setup the shader
    // ----------------------------
    Shader shader("text.vs", "text.fs");

    Shader sineWaveShader("sinewaveshader.vs", "sinewaveshader.fs");
    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(SCR_WIDTH), 0.0f, static_cast<float>(SCR_HEIGHT));
    shader.use();
    glUniformMatrix4fv(glGetUniformLocation(shader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

    // load font as face
    FT_Face face;
    if (FT_New_Face(ft, "fonts/arial.ttf", 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glfwSetKeyCallback(window, key_callback);

    double lastTime = glfwGetTime();
    int frameCount = 0;




    // configure VAO/VBO for texture quads
    // -----------------------------------
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glGenVertexArrays(1, &sineWaveVAO);
    glGenBuffers(1, &sineWaveVBO);

    std::thread socketThread(update_target_amplitude);



    // render loop
    // -----------

        // After initializing OpenGL context
    const GLubyte* glVersion = glGetString(GL_VERSION);
    const GLubyte* glRenderer = glGetString(GL_RENDERER);

    std::string versionString(reinterpret_cast<const char*>(glVersion));
    std::string rendererString(reinterpret_cast<const char*>(glRenderer));
    while (!glfwWindowShouldClose(window))
    {
        // FPS COBA-coba
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // Misalkan Scaling di atas 1.0, maka timenya akan terupdate secara parallel
        if (freq > 1.0f) {
            scaley = 1.0f / freq;
        }
        else {
            scaley = 1.0f;
        }


        float force = 0.0f;
        if (upKeyPressed)
        {
            force += force_scale;
            std::cout << "Sedang ditekan";
            std::cout << force << std::endl;
        }

        // Fisika iseng
        acceleration = -stiffness * (amplitude - targetAmplitude) - damping * velocity + force;
        velocity += acceleration * deltaTime;
        amplitude += velocity * deltaTime;
        // input
        // -----

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        RenderSineWave(sineWaveShader, amplitude, freq, scaley, glm::vec3(1.0, 1.0f, 1.0f));

        SmoothIncrease(time_mod, targetTimeMod, 0.01f);

        std::string forceStr = std::to_string(force);
        std::string acc = std::to_string(acceleration);
        std::string amp = std::to_string(amplitude);
        std::string target_amplitude = std::to_string(targetAmplitude);
        std::string damping_str = std::to_string(damping);
        std::string stiffnes_str = std::to_string(stiffness);
    //    std::string freq_str = std::to_string(freq);
        std::string socket_str = std::to_string(received_socket);

        RenderText(shader, "FM-RAVI Software, Using GPU Renderer: " + rendererString, 25.0f, 880.0f, 0.25f, glm::vec3(1.0, 1.0f, 0.0f));

        // Concatenate and render the text
        RenderText(shader, "Spring initial amplitude: " + target_amplitude, 25.0f, 200.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
        RenderText(shader, "Spring damping: " + damping_str, 25.0f, 170.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
        RenderText(shader, "Spring stiffness: " + stiffnes_str, 25.0f, 140.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
        RenderText(shader, "Current force: " + forceStr + "N", 25.0f, 110.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
        RenderText(shader, "Current Amplitude: " + amp, 25.0f, 80.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
        RenderText(shader, "Current Acceleration: " + acc + "m/s^2", 25.0f, 50.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));
  //      RenderText(shader, "Socket: " + socket_str, 25.0f, 25.0f, 0.4f, glm::vec3(1.0f, 1.0f, 0.0f));

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    runSocketThread = false;  // Stop the socket thread
    socketThread.join();

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// render line of text
// -------------------
void RenderText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderSineWave(Shader& shader, float amplitude, float freq, float scaley, glm::vec3 color)
{
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "lineColor"), color.x, color.y, color.z);

    const int num_points = 1000;  // Number of points on the sine wave
    float vertices[num_points * 2];  // Each point has an x and y coordinate

    for (int i = 0; i < num_points; ++i)
    {
        float x = 2.0f * i / (num_points - 1) - 1.0f;  // Map i to [-1, 1]
        float y = amplitude * scaley * sin(2 * 3.14159 * x * freq + glfwGetTime() * time_mod);

        vertices[i * 2] = x;
        vertices[i * 2 + 1] = y;
    }

    glBindVertexArray(sineWaveVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sineWaveVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glDrawArrays(GL_LINE_STRIP, 0, num_points);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

