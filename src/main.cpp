#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <shader/shader.h>
#include <camera.h>

#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <experimental/filesystem>

#include "stb_image/stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// forward declarations
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow *window);

bool load_image(std::vector<unsigned char> &image, const std::string &filename, int &x, int &y, int &N);
void returnColorValues(int &x, int &y, int &width, const size_t &RGBA, std::vector<unsigned char> &image);
bool write_char_array(std::ostream &os, const char *string);
float f(float x, float y);
void generate_grid(int N, int M, std::vector<glm::vec3> &vertices, std::vector<glm::uvec3> &indices);
GLuint generate_vao(const std::vector<glm::vec3> &vertices, const std::vector<glm::uvec3> &indices);
void draw_vao(GLuint vao, GLsizei n);

// settings
const uint16_t SCR_WIDTH = 800;
const uint16_t SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(5.0f, 5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float cameraMovementSpeed = 4.0f;
float cameraBoostSpeed = 8.0f;

bool escapePressed = false;
bool vsyncOn = true;
bool wireFrameOn = true;
bool shiftKeyPressed = false;

// timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

const size_t RGBA = 4;
int image_width, image_height;
static std::vector<unsigned char> image;
int x = image_width;
int y = image_height;

static char filepath[128] = {0};
static char currentFilename[128] = "-";
bool imageLoaded = false;
float heightScaling = 10.0f;
glm::vec3 heightmapColor{1.0f, 1.0f, 1.0f};

static int N = 40; // image width
static int M = 20; // image height

// filesystem namespace
namespace fs = std::experimental::filesystem;

int main()
{
   fs::current_path("../");

   // glfw: initialize and configure
   glfwInit();
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

   // glfw window creation
   GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Heightmap Visualizer", nullptr, nullptr);
   if (window == nullptr)
   {
      std::cout << "Failed to create GLFW window" << std::endl;
      glfwTerminate();
      return -1;
   }
   glfwMakeContextCurrent(window);
   glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
   glfwSetCursorPosCallback(window, mouse_callback);
   glfwSetScrollCallback(window, scroll_callback);
   glfwSetKeyCallback(window, keyboard_callback);

   // tell GLFW to capture our mouse
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   // tell stb_image.h to flip loaded textures on the y-axis (before loading the model)
   stbi_set_flip_vertically_on_load(true);

   // glad: load all OpenGL function pointers
   if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
   {
      std::cout << "Failed to initialize GLAD" << std::endl;
      return -1;
   }

   // configure global opengl state
   // -------------------------------------------
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   // imgui init
   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   ImGuiIO &io = ImGui::GetIO();
   (void)io;

   ImGui::StyleColorsDark();

   ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 330");

   // build and compile our shader program
   // ---------------------------------------
   Shader modelShader("shaders/vertex.vs", "shaders/fragment.fs");

   // set up vertex data (and buffer(s)) and configure vertex attributes
   // ------------------------------------------------------------------
   /*float vertices[] = {
       // positions          // colors           // texture coords
       0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,   // top right
       0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,  // bottom right
       -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom left
       -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f   // top left
   };
   unsigned int indices[] = {
       0, 1, 3, // first triangle
       1, 2, 3  // second triangle
   };
   unsigned int VBO, VAO, EBO;
   glGenVertexArrays(1, &VAO);
   glGenBuffers(1, &VBO);
   glGenBuffers(1, &EBO);

   glBindVertexArray(VAO);

   glBindBuffer(GL_ARRAY_BUFFER, VBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

   // position attribute
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
   glEnableVertexAttribArray(0);
   // color attribute
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
   glEnableVertexAttribArray(1);
   // texture coord attribute
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
   glEnableVertexAttribArray(2);

   // load and create a texture
   // -------------------------
   unsigned int texture;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
   // set the texture wrapping parameters
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // set texture wrapping to GL_REPEAT (default wrapping method)
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   // set texture filtering parameters
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   // load image, create texture and generate mipmaps
   int width, height, nrChannels;
   // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
   unsigned char *data = stbi_load("../ahuha.png", &width, &height, &nrChannels, 0);
   if (data)
   {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
      glGenerateMipmap(GL_TEXTURE_2D);
   }
   else
   {
      std::cout << "Failed to load texture" << std::endl;
   }
   stbi_image_free(data);
*/
   std::vector<glm::vec3> vertices;
   std::vector<glm::uvec3> indices;

   generate_grid(N, M, vertices, indices);
   GLuint vao = generate_vao(vertices, indices);

   modelShader.use();
   modelShader.setVec3("heightmapColor", heightmapColor);

   // render loop
   //----------------------------------------
   while (!glfwWindowShouldClose(window))
   {
      float currentFrame = glfwGetTime();
      deltaTime = currentFrame - lastFrame;
      lastFrame = currentFrame;

      if (vsyncOn)
      {
         glfwSwapInterval(1);
      }
      else
      {
         glfwSwapInterval(0);
      }

      if (shiftKeyPressed)
         camera.boostMovementSpeed(cameraBoostSpeed);
      if (!shiftKeyPressed)
         camera.resetMovementSpeed(cameraMovementSpeed);

      // input
      processInput(window);

      // render
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      modelShader.use();
      glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
      glm::mat4 view = camera.GetViewMatrix();
      modelShader.setMat4("projection", projection);
      modelShader.setMat4("view", view);

      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
      model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1, 0, 0));
      model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0, 0, 1));
      model = glm::scale(model, glm::vec3(10.0f, 10.0f * M / N, 10.0f));
      modelShader.setMat4("model", model);

      // ImGui Setup
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Basic GUI for loading images
      ImGui::Begin("Heightmap");
      ImGui::PushItemWidth(300);
      ImGui::InputText("File Path", filepath, IM_ARRAYSIZE(filepath));
      /*ImGui::DragInt("X", &x);
      if (x >= image_width)
         x = image_width;
      if (x <= 0)
         x = 0;
      ImGui::SameLine();
      ImGui::DragInt("Y", &y);
      if (y >= image_height)
         y = image_height;
      if (y <= 0)
         y = 0;
      bool showColorValue = ImGui::Button("RGBA Value at given X/Y");
      if (showColorValue)
      {
         returnColorValues(x, y, image_width, RGBA, image);
      }
      ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);*/
      /*bool showImageSize = ImGui::Button("Image Size");
      if (showImageSize)
      {
         if (!image.empty())
         {
            std::cout << "Width: " << image_width << " Height: " << image_height << '\n';
         }
         else
         {
            std::cout << "No image loaded.\n";
         }
      }*/
      //ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
      bool loadFile = ImGui::Button("Load File");
      if (loadFile)
      {
         bool success = load_image(image, filepath, image_width, image_height, N);
         if (success)
            strcpy(currentFilename, filepath);
      }

      ImGui::SameLine();

      bool generateGrid = ImGui::Button("Generate Grid");
      if (generateGrid)
      {
         modelShader.setVec3("heightmapColor", heightmapColor);
         std::cout << "Generating grid" << '\n';
         vertices.clear();
         indices.clear();
         generate_grid(N, M, vertices, indices);
         vao = generate_vao(vertices, indices);
      }
      ImGui::SameLine();
      ImGui::PushItemWidth(114);
      ImGui::DragFloat("Z Scale Factor", &heightScaling);

      //ImGui::InputInt("Size of N", &N);
      ImGui::Text("Width: %i", N);
      ImGui::SameLine();

      //ImGui::InputInt("Size of M", &M);
      ImGui::Text("Height: %i", M);
      ImGui::SameLine();
      ImGui::Text("Loaded file: %s", currentFilename);

      ImGui::Separator();

      ImGui::ColorEdit3("Heightmap Color", (float *)&heightmapColor);
      ImGui::Checkbox("Wireframe On", &wireFrameOn);
      if (wireFrameOn)
      {
         glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      }
      else
      {
         glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      }

      ImGui::End();

      //-------------------------------------------------------------------------------

      ImGui::Begin("Performance");
      ImGui::Text("VSync");
      ImGui::Checkbox("VSync", &vsyncOn);

      ImGui::Text("Frametime: %.3f ms (FPS %.1f)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
      //bind textures on corresponding texture units
      /*glBindTexture(GL_TEXTURE_2D, texture);

      ourShader.use();

      glBindVertexArray(VAO);

      glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
*/
      draw_vao(vao, (GLsizei)indices.size() * 3);
      glFrontFace(GL_CW);
      draw_vao(vao, (GLsizei)indices.size() * 3);
      glFrontFace(GL_CCW);

      // Imgui Rendering
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
      //------------------------------------------
      glfwSwapBuffers(window);
      glfwPollEvents();
   }
   /*
   glDeleteVertexArrays(1, &VAO);
   glDeleteBuffers(1, &VBO);
   glDeleteBuffers(1, &EBO);
*/
   // glfw: terminate, clearing all previously allocated GLFW resources
   //---------------------------------------------------
   glfwTerminate();

   return 0;
}

// called every loop to check whether ESC is pressed. If that's the case the window closes
void processInput(GLFWwindow *window)
{
   if (!escapePressed)
   {
      const float cameraSpeed = 2.5f * deltaTime;
      if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
         camera.ProcessKeyboard(FORWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
         camera.ProcessKeyboard(BACKWARD, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
         camera.ProcessKeyboard(LEFT, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
         camera.ProcessKeyboard(RIGHT, deltaTime);
      if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
         shiftKeyPressed = true;
      if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
         shiftKeyPressed = false;
   }
}

// checks whether the window has changed size to adjust the viewport too
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
   glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{

   if (firstMouse)
   {
      lastX = xpos;
      lastY = ypos;
      firstMouse = false;
   }

   float xoffset = xpos - lastX;
   float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top

   lastX = xpos;
   lastY = ypos;
   if (!escapePressed)
      camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
   camera.ProcessMouseScroll(yoffset);
}

void keyboard_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
   if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
   {
      //glfwSetWindowShouldClose(window, true);
      if (!escapePressed)
      {
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
         escapePressed = true;
      }
      else
      {
         glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
         escapePressed = false;
      }
   }
}
// function to load images by pressing the load image button if a filepath is given
// x and y resemble the image_width and image_height in this case
// error handling is done through printing related strings into the console for now
// ----------------------------------------------------------------------------------------------
bool load_image(std::vector<unsigned char> &image, const std::string &filename, int &x, int &y, int &N)
{
   write_char_array(std::cout, filepath);
   std::cout << std::endl;
   int n;
   unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, 4);
   if (data != nullptr)
   {
      image = std::vector<unsigned char>(data, data + x * y * 4);
      std::cout << "Image loaded successfully\n";
      imageLoaded = true;
      std::cout << "Image width: " << image_width << ", Image height: " << image_height << '\n';
      N = image_width;
      M = image_height;
   }
   else
   {
      std::cout << "Failed to load image\n";
   }
   stbi_image_free(data);
   return (data != nullptr);
}

// function to return the color of a pixel at a given position on an image
// done mainly through image vector's data
// ---------------------------------------------------------------------------
void returnColorValues(int &x, int &y, int &image_width, const size_t &RGBA, std::vector<unsigned char> &image)
{
   if (!image.empty())
   {
      size_t index = RGBA * (y * image_width + x);
      std::cout << "RGBA Value at x = " << x << " y = " << y << ":\n"
                << "R: " << static_cast<int>(image[index + 0]) << '\n'
                << "G: " << static_cast<int>(image[index + 1]) << '\n'
                << "B: " << static_cast<int>(image[index + 2]) << '\n'
                << "A: " << static_cast<int>(image[index + 3]) << '\n';
   }
   else
   {
      std::cout << "No image loaded.\n";
   }
}

// helper function to print the current Filepath that is used when loading an image
// the result is posted inside of the console
//----------------------------------------------------------------------------------
bool write_char_array(std::ostream &os, const char *string)
{
   if (strlen(string) > 0)
   {
      std::cout << "Filepath: ";
      os.write(string, strlen(string));
      return true;
   }
   std::cout << "No Filepath given";
   return false;
}

// helper function to first draw a heightmap based on sin if no image has been loaded yet
// ---------------------------------------------------------------------------------------
float f(float x, float y)
{
   return sin(x * 2.0f * M_PI) * sin(y * 2.0f * M_PI) * 0.1f;
}

// generates the actual grid by filling the vertices and indices vector
// ---------------------------------------------------------------------
void generate_grid(int N, int M, std::vector<glm::vec3> &vertices, std::vector<glm::uvec3> &indices)
{
   for (int i = 0; i <= N; ++i) // i = row
   {
      for (int j = 0; j <= M; ++j) // y = column
      {
         float x = (float)j / (float)N; // at position j/N is x
         float y = (float)i / (float)M; // at position i/N is y
         float z = f(x, y);
         //std::cout << "Row: " << i << " Column: " << j << " Value: " << z << '\n';
         if (imageLoaded)
         {
            size_t index = RGBA * (j * image_width + i);
            float red = static_cast<float>(image[index + 0]);
            float green = static_cast<float>(image[index + 1]);
            float blue = static_cast<float>(image[index + 2]);

            //std::cout << red << ' ' << green << ' ' << blue << '\n';

            z = ((red * green * blue) / (255 * 255 * 255)) * heightScaling / 100;
            //std::cout << z << '\n';
         }
         vertices.push_back(glm::vec3(x, y, z));

         // NORMALS
      }
   }

   for (int j = 0; j < N; ++j)
   {
      for (int i = 0; i < M; ++i)
      {
         int row1 = j * (M + 1);
         int row2 = (j + 1) * (M + 1);

         // triangle 1
         indices.push_back(glm::uvec3(row1 + i, row1 + i + 1, row2 + i + 1));

         // triangle 2
         indices.push_back(glm::uvec3(row1 + i, row2 + i + 1, row2 + i));
      }
   }
}

// Generates the VAOs, VBOs and IBOs of the heightmap
// -------------------------------------------------
GLuint generate_vao(const std::vector<glm::vec3> &vertices, const std::vector<glm::uvec3> &indices)
{
   GLuint vao;
   glGenVertexArrays(1, &vao);
   glBindVertexArray(vao);

   GLuint vbo;
   glGenBuffers(1, &vbo);
   glBindBuffer(GL_ARRAY_BUFFER, vbo);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), glm::value_ptr(vertices[0]), GL_STATIC_DRAW);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

   GLuint ibo;
   glGenBuffers(1, &ibo);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(glm::uvec3), glm::value_ptr(indices[0]), GL_STATIC_DRAW);

   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   return vao;
}

// Draws the actual heightmap based on a given VAO
// ---------------------------------------
void draw_vao(GLuint vao, GLsizei n)
{
   glBindVertexArray(vao);
   glDrawElements(GL_TRIANGLES, (GLsizei)n, GL_UNSIGNED_INT, NULL);
   glBindVertexArray(0);
}