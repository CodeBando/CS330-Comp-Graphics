/*
* CS330 - SNHU Comp Graphics and Visualization
* Project Milestones
* Author: Matthew Bandyk
* Version: 5.0
* Last updated: 8/10/2023
* ----UPDATES---- 
* 7/22: Corrected the Cylinder creation issue - indices were incorrectly created for top/bottom circles
*       Updated render function to allow rotation of cylinders and torus as a single object around a single point
*       Created and added Camera functionality, see Camera.h file
*       Created the CreatePlane function to create a plane for the scene
* 7/30: Updated the Cylinder, Torus, and Plane mesh creation functions to capture texture coords
*       Created texture loading function to load textures, and place texture ID's into a map to utilize in Render
*       Updated Render function to render textures onto the objects
*       Updated to include functionality to switch between Ortho and Perspective views
*       Update to utilize Structs for the objects rather than single VAO's, VBO's, and EBO's
* 8/6:  Created new functions to handle light cube square mesh creation
*       Updated Shaders to handle normals of the objects, as well as lighting utilizing Phong Lighting concepts
*       Updated object mesh creation functions to capture objects normals
*       Created two point lights to light the scene
* 8/10  Created function to create and store cube mesh data
*       Created function to create and store sphere mesh data
*       Added additional light to help create a realistic scene based on image
*       Added additional textures to utilize on new objects
*       Added material properties to the shader and objects to create more realistic lighting
*/

// Libraries to include
#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>

// Include inline camera class to handle camera build and movements 
#include "Camera.h"

using namespace std;

// Shader programs macro
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif // !GLSL

const char* const SCR_TITLE = "Project Milestone - Matt Bandyk";
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Create camera
Camera camera(glm::vec3(0.0f, 6.0f, 7.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// delcare variables for timing
float deltaTime = 0.0f; // time between current frame and last frame
float lastFrame = 0.0f;

struct Material {
    float shininess;
    glm::vec3 specularColor;
};

// Struct declarations
// Individual VAO's/VBO's/EBO's and Textures to allow for individual textures per surface of cylinders
struct Cylinder {
    GLuint cylinderTopVAO;
    GLuint cylinderBottomVAO;
    GLuint cylinderSidesVAO;
    GLuint cylinderTopTexture;
    GLuint cylinderBottomTexture;
    GLuint cylinderSideTexture;
    GLuint cylinderTopVBO;
    GLuint cylinderBottomVBO;
    GLuint cylinderSideVBO;
    GLuint cylinderTopNormalsVBO;
    GLuint cylinderBottomNormalsVBO;
    GLuint cylinderSideNormalsVBO;
    GLuint cylinderTopEBO;
    GLuint cylinderBottomEBO;
    GLuint cylinderSidesEBO;
    unsigned int cylinderTopIndices;
    unsigned int cylinderBottomIndices;
    unsigned int cylinderSidesIndices;

    GLuint topBottomTextureID; // Texture ID for the top and bottom circles
    GLuint sideTextureID;      // Texture ID for the sides

    Material cylMaterial;

    vector<glm::vec3> verticesTop;
    vector<glm::vec3> verticesBottom;
    vector<glm::vec3> verticesSides;
    vector<glm::mat4> CylinderMatrices;
    vector<glm::vec3> normalsTop;
    vector<glm::vec3> normalsBottom;
    vector<glm::vec3> normalsSides;
};

// Struct to hold torus data
struct Torus {
    GLuint torusVAO;
    GLuint torusTextureVBO;
    GLuint torusNormalVBO;
    GLuint torusVBO;
    GLuint torusEBO;
    unsigned int torusIndices;
    Material torusMaterial;

    GLuint torusTextureID;         // Texture ID for the torus
    vector<glm::mat4> torusMatrices;
    vector<glm::vec3> normals;
};

// Struct to hold plane data
struct Plane {
    GLuint planeVAO;
    GLuint planeTextureVBO;
    GLuint planeVBO;
    GLuint planeNormalVBO;
    GLuint planeEBO;
    unsigned int planeIndices;
    Material planeMaterial;

    GLuint planeTextureID;        // Texture ID for the plane
    vector<glm::mat4> planeMatrices;
    vector<glm::vec3> normals;
};

// Struct to hold the Cube data
struct Cube {
    GLuint cubeVAO;
    GLuint cubeVBO;
    GLuint cubeEBO;
    glm::mat4 translation;
    glm::mat4 rotation;
    GLuint textures[6];
    Material cubeMaterial;
};

// Struct to hold the sphere data
struct Sphere {
    GLuint sphereVAO;
    GLuint sphereVBO;
    GLuint sphereEBO;
    glm::mat4 translation;
    GLuint texture;
    Material sphereMaterial;
    unsigned int sphereIndices;
};

// Struct to hold light cube data
struct LightCube {
    GLuint lCubeVAO;
    GLuint lCubeVBO;
    GLuint lCubeEBO;
};

// Struct to hold Point Lights data
struct PointLight {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float ambientStrength;
    float specularIntensity;
    float highlightSize;
};

PointLight pointLights[3];

Torus torus;
Plane plane;
Sphere sphere;

// This map stores texture paths and their IDs
map<std::string, GLuint> textures; 

// Texture
GLuint textureId;

glm::vec2 uvScale(1.0f, 1.0f); 

GLint gTexWrapMode = GL_REPEAT;

// shader programs
GLuint objectProgramId;
GLuint lightProgramId;

GLFWwindow* window = nullptr;

// Declare functions
void flipImageVertically(unsigned char* image, int width, int height, int channels);
bool Initialize(int, char* [], GLFWwindow** window);
void ResizeWindow(GLFWwindow* window, int width, int height);
void ProcessInput(GLFWwindow* window);
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void CreateCylinderMesh(float radius, float height, int sectors, int stacks, GLuint topBottomCircleTexture, GLuint sideTexture, float shininess, const glm::vec3& specularColor, const glm::vec3& translation, vector<Cylinder>& cylinders);
void CreateTorusMesh(float innerRadius, float outerRadius, int sides, int rings, GLuint torusTexture, float shininess, const glm::vec3& specularColor, const glm::vec3& translation);
void CreatePlane(float width, float height, GLuint planeTexture, float shininess, const glm::vec3& specularColor, const glm::vec3& translation);
void CreateCubeMesh(float width, float height, float depth, GLuint leftSideTexture, GLuint rightSideTexture, GLuint frontTexture, GLuint backTexture, GLuint topTexture, GLuint bottomTexture,
    float shininess, const glm::vec3& specularColor, const glm::vec3& translation, float rotation, vector<Cube>& cubes);
void CreateSphereMesh(float radius, GLuint sphereTextureID, float shininess, const glm::vec3& specularColor, const glm::vec3& translation);
GLuint LoadTexture(const std::string& texturePath);
void DestroyTexture(GLuint textureId);
void Render(const vector<Cylinder>& cylinders, const vector<Cube>& cubes, const vector<LightCube>& lCubes);
bool CreateShaders(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void DestroyShaders(GLuint programId);

// Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec2 textureCoordinate;

    out vec3 FragPos;        
    out vec3 Normal;      
    out vec2 vertexTextureCoordinate;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        FragPos = vec3(model * vec4(position, 1.0));
        Normal = mat3(transpose(inverse(model))) * normal;
        vertexTextureCoordinate = textureCoordinate;
        gl_Position = projection * view * model * vec4(position, 1.0);
    }
);

// Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
    in vec3 FragPos;
    in vec3 Normal;
    in vec2 vertexTextureCoordinate;

    out vec4 fragmentColor;

    uniform sampler2D uTexture;
    uniform vec2 uvScale;
    uniform vec3 viewPosition;

    // Point light properties
    struct PointLight {
        vec3 position;
        vec3 color;
        float intensity;
        float ambientStrength;
        float specularIntensity;
        float highlightSize;
    };

    struct Material {
        float shininess; // How shiny the material is, higher values give tighter, smaller highlights
        vec3 specularColor; // Color of the specular reflection
    };

    // Define an array of point lights
    const int NUM_POINT_LIGHTS = 3;
    uniform PointLight pointLights[NUM_POINT_LIGHTS];
    uniform Material material;


    void main() {
        // Initialize the components to 0
        vec3 ambient = vec3(0.0f);
        vec3 diffuse = vec3(0.0f);
        vec3 specular = vec3(0.0f);

        vec2 scaledTextureCoordinate = vertexTextureCoordinate * uvScale;

        // Iterate over the lights
        for (int i = 0; i < NUM_POINT_LIGHTS; i++) {

            /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

            //Calculate Ambient lighting
            ambient += pointLights[i].ambientStrength * pointLights[i].color * pointLights[i].intensity; // Generate ambient light color

            //Calculate Diffuse lighting
            vec3 norm = normalize(Normal); // Normalize vectors to 1 unit
            vec3 lightDirection = normalize(pointLights[i].position - FragPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
            float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
            diffuse += impact * pointLights[i].color * pointLights[i].intensity; // Generate diffuse light color

            //Calculate Specular lighting
            vec3 viewDir = normalize(viewPosition - FragPos); // Calculate view direction
            vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
            //Calculate specular component
            float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), pointLights[i].highlightSize);
            vec3 spec = pointLights[i].color * material.specularColor * pointLights[i].intensity;
            specular += spec * pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
        }

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, scaledTextureCoordinate);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
    }
);

// vertex shader code for light cubes
const GLchar* lightVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {
        gl_Position = projection * view * model * vec4(position, 1.0);
    }
);

// fragment shader code for light cubes
const GLchar* lightFragmentShaderSource = GLSL(440,
    out vec4 fragmentColor;

    void main() {
        fragmentColor = vec4(1.0); // Set the color of the light representation (e.g., white)
    }
);

// Flips loaded texture images to assign Y-axis going down instead of up
void flipImageVertically(unsigned char* image, int width, int height, int channels) {
    for (int j = 0; j < height / 2; ++j) {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i) {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

/*
* Initialize the GLFW library and create a window
* Returns true if initialization is successful, false otherwise
*/
bool Initialize(int, char* [], GLFWwindow** window) {
    // Initialize GLFW
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return false;
    }

    // Configure GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a GLFW window
    *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, SCR_TITLE, nullptr, nullptr);
    if (!(*window)) {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return false;
    }

    // Make the current OpenGL context the window's context
    glfwMakeContextCurrent(*window);

    // glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glfwSetFramebufferSizeCallback(*window, ResizeWindow);

    glfwSetCursorPosCallback(*window, MousePositionCallback);
    glfwSetScrollCallback(*window, MouseScrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Load OpenGL function pointers with GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        cout << "Failed to initialize GLAD" << endl;
        glfwTerminate();
        return false;
    }

    return true;
}

// Callback function to resize the window
void ResizeWindow(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

/*
* Function to process keyboard inputs
* ESC to terminate the program
* WSAD to navigate forward, backward, left and right
* QE to navigate up and down
* P to switch between Ortho and Perspective
*/
// Processes the keyboard inputs
void ProcessInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera.ProcessKeyboard(UPWARDS, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWNWARDS, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
        camera.ToggleViewMode();
}

// callback function when mouse moves
void MousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// callback function when mouse scroll-wheel moves
void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

/*
* Method to create the mesh for a cylinder
* Creates vertices for the sides and top/bottom circles separately 
* Creates indices for sides and top/bottom circles separately
* Creates VAO's, VBO's, and EBO's objects and binds them
* Store the VAO's, VBO's, index count, Texture ID's and model matrix for later use
* @params radius: radius of the cylinder
*         height: height of the cylinder
*         sectors: number of sectors (subdivisions) around the circumference
*         stacks: number of stacks (subdivisions) along the height
*         texture ID's: Texture ID for sides and texture ID for top/bottom
*         translation: translation vector to position the cylinder
*         cylinders: vector to hold the multiple cylinder objects
*/
void CreateCylinderMesh(float radius, float height, int sectors, int stacks, GLuint topBottomCircleTexture, GLuint sideTexture, float shininess, const glm::vec3& specularColor, const glm::vec3& translation, vector<Cylinder>& cylinders) {
    // Vectors to hold data
    vector<glm::vec3> verticesTop;
    vector<glm::vec3> verticesBottom;
    vector<glm::vec3> verticesSides;
    vector<glm::vec2> texCoordsTop;
    vector<glm::vec2> texCoordsBottom;
    vector<glm::vec2> texCoordsSides;
    vector<glm::vec3> normalsTop;
    vector<glm::vec3> normalsBottom;
    vector<glm::vec3> normalsSides;
    vector<unsigned int> cylinderTopIndices;
    vector<unsigned int> cylinderBottomIndices;
    vector<unsigned int> cylinderSidesIndices;

    Cylinder cylinder;

    // Variables to hold data
    float sectorStep = 2 * glm::pi<float>() / sectors;
    float stackStep = height / stacks;
    float sectorAngle, x, y, z;

    // Create vertices for the top circle of the cylinder
    for (int j = 0; j <= sectors; ++j) {
        sectorAngle = j * sectorStep;
        x = radius * glm::cos(sectorAngle);
        y = height / 2.0f;
        z = radius * glm::sin(sectorAngle);
        verticesTop.push_back(glm::vec3(x, y, z));

        // Calculate the texture coordinates based on normalized polar coordinates
        float u = (glm::cos(sectorAngle) + 1.0f) * 0.5f; // Range: [0, 1]
        float v = (glm::sin(sectorAngle) + 1.0f) * 0.5f; // Range: [0, 1]
        texCoordsTop.push_back(glm::vec2(u, v));
    }

    // Create normals for the top circle of the cylinder
    for (int j = 0; j <= sectors; ++j) {
        // All normals for the top circle point straight up (0, 1, 0)
        normalsTop.push_back(glm::vec3(0, 1, 0));
    }

    // Create vertices for the bottom circle of the cylinder
    for (int j = 0; j <= sectors; ++j) {
        sectorAngle = j * sectorStep;
        x = radius * glm::cos(sectorAngle);
        y = -height / 2.0f;
        z = radius * glm::sin(sectorAngle);
        verticesBottom.push_back(glm::vec3(x, y, z));

        // Calculate the texture coordinates based on normalized polar coordinates
        float u = (glm::cos(sectorAngle) + 1.0f) * 0.5f; // Range: [0, 1]
        float v = (-glm::sin(sectorAngle) + 1.0f) * 0.5f; // Range: [0, 1]
        texCoordsBottom.push_back(glm::vec2(u, v));
    }

    // Create normals for the bottom circle of the cylinder
    for (int j = 0; j <= sectors; ++j) {
        normalsBottom.push_back(glm::vec3(0, -1, 0));
    }

    // Create vertices for the side surfaces of the cylinder and texture coordinates
    for (int i = 0; i <= stacks; ++i) {
        float stackHeight = -height / 2.0f + i * stackStep;

        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;
            x = radius * glm::cos(sectorAngle);
            y = stackHeight;
            z = radius * glm::sin(sectorAngle);

            verticesSides.push_back(glm::vec3(x, y, z));

            // Calculate the texture coordinates for sides based on normalized polar coordinates
            float u = static_cast<float>(j) / sectors;
            float v = (y + height / 2.0f) / height;
            texCoordsSides.push_back(glm::vec2(u, v));
        }
    }

    // Create normals for the side surfaces of the cylinder
    for (int i = 0; i <= stacks; ++i) {
        for (int j = 0; j <= sectors; ++j) {
            sectorAngle = j * sectorStep;
            x = radius * glm::cos(sectorAngle);
            y = 0;
            z = radius * glm::sin(sectorAngle);

            // Normalizing the vector to get the normal
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
            normalsSides.push_back(normal);
        }
    }

    // Create indices for the bottom circle
    int bottomCenterIndex = verticesBottom.size() - 1; // Index of the center of the bottom circle
    int bottomOffset = bottomCenterIndex - sectors; // Offset for the bottom circle vertices

    for (int j = 0; j < sectors; ++j) {
        cylinderBottomIndices.push_back(bottomCenterIndex);
        cylinderBottomIndices.push_back(bottomOffset + j);
        cylinderBottomIndices.push_back(bottomOffset + (j + 1) % sectors);
    }

    // Create indices for the top circle
    int topCenterIndex = verticesTop.size() - 1; // Index of the center of the top circle
    int topOffset = topCenterIndex - sectors; // Offset for the top circle vertices

    for (int j = 0; j < sectors; ++j) {
        cylinderTopIndices.push_back(topCenterIndex);
        cylinderTopIndices.push_back(topOffset + (j + 1) % sectors);
        cylinderTopIndices.push_back(topOffset + j);
    }

    // Create indices for the side surface
    int k1, k2;
    for (int i = 0; i < stacks; ++i) {
        k1 = i * (sectors + 1);
        k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            cylinderSidesIndices.push_back(k1);
            cylinderSidesIndices.push_back(k1 + 1);
            cylinderSidesIndices.push_back(k2);

            cylinderSidesIndices.push_back(k2);
            cylinderSidesIndices.push_back(k1 + 1);
            cylinderSidesIndices.push_back(k2 + 1);
        }
    }

    // Create and bind vertex array object for the sides (VAO)
    glGenVertexArrays(1, &cylinder.cylinderSidesVAO);
    glBindVertexArray(cylinder.cylinderSidesVAO);

    // Create vertex buffer object (VBO) for sides vertices
    glGenBuffers(1, &cylinder.cylinderSideVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderSideVBO);
    glBufferData(GL_ARRAY_BUFFER, verticesSides.size() * sizeof(glm::vec3), verticesSides.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for side normals
    glGenBuffers(1, &cylinder.cylinderSideNormalsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderSideNormalsVBO);
    glBufferData(GL_ARRAY_BUFFER, normalsSides.size() * sizeof(glm::vec3), normalsSides.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for texture coordinates of the sides
    glGenBuffers(1, &cylinder.cylinderSideTexture);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderSideTexture);
    glBufferData(GL_ARRAY_BUFFER, texCoordsSides.size() * sizeof(glm::vec2), texCoordsSides.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create element buffer object (EBO) for indices of sides
    glGenBuffers(1, &cylinder.cylinderSidesEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinder.cylinderSidesEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinderSidesIndices.size() * sizeof(unsigned int), cylinderSidesIndices.data(), GL_STATIC_DRAW);

    // Create and bind vertex array object for the top circle (VAO)
    glGenVertexArrays(1, &cylinder.cylinderTopVAO);
    glBindVertexArray(cylinder.cylinderTopVAO);

    // Create vertex buffer object (VBO) for top vertices
    glGenBuffers(1, &cylinder.cylinderTopVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderTopVBO);
    glBufferData(GL_ARRAY_BUFFER, verticesTop.size() * sizeof(glm::vec3), verticesTop.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for top normals
    glGenBuffers(1, &cylinder.cylinderTopNormalsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderTopNormalsVBO);
    glBufferData(GL_ARRAY_BUFFER, normalsTop.size() * sizeof(glm::vec3), normalsTop.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for texture coordinates of the top circle
    glGenBuffers(1, &cylinder.cylinderTopTexture);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderTopTexture);
    glBufferData(GL_ARRAY_BUFFER, texCoordsTop.size() * sizeof(glm::vec2), texCoordsTop.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create element buffer object (EBO) for indices of the top circle
    glGenBuffers(1, &cylinder.cylinderTopEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinder.cylinderTopEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinderTopIndices.size() * sizeof(unsigned int), cylinderTopIndices.data(), GL_STATIC_DRAW);

    // Create and bind vertex array object for the bottom circle (VAO)
    glGenVertexArrays(1, &cylinder.cylinderBottomVAO);
    glBindVertexArray(cylinder.cylinderBottomVAO);

    // Create vertex buffer object (VBO) for bottom vertices
    glGenBuffers(1, &cylinder.cylinderBottomVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderBottomVBO);
    glBufferData(GL_ARRAY_BUFFER, verticesBottom.size() * sizeof(glm::vec3), verticesBottom.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for bottom normals
    glGenBuffers(1, &cylinder.cylinderBottomNormalsVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderBottomNormalsVBO);
    glBufferData(GL_ARRAY_BUFFER, normalsBottom.size() * sizeof(glm::vec3), normalsBottom.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for texture coordinates of the bottom circle
    glGenBuffers(1, &cylinder.cylinderBottomTexture);
    glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderBottomTexture);
    glBufferData(GL_ARRAY_BUFFER, texCoordsBottom.size() * sizeof(glm::vec2), texCoordsBottom.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create element buffer object (EBO) for indices of the bottom circle
    glGenBuffers(1, &cylinder.cylinderBottomEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cylinder.cylinderBottomEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cylinderBottomIndices.size() * sizeof(unsigned int), cylinderBottomIndices.data(), GL_STATIC_DRAW);

    // Unbind VAO
    glBindVertexArray(0);

    // Store the model matrix for transformation
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translation);
    cylinder.CylinderMatrices.push_back(modelMatrix);

    // Store all coords and push to cylinders Struct Object for later use
    cylinder.verticesTop = verticesTop;
    cylinder.verticesBottom = verticesBottom;
    cylinder.verticesSides = verticesSides;
    cylinder.cylinderTopIndices = cylinderTopIndices.size();
    cylinder.cylinderBottomIndices = cylinderBottomIndices.size();
    cylinder.cylinderSidesIndices = cylinderSidesIndices.size();
    cylinder.sideTextureID = sideTexture;
    cylinder.topBottomTextureID = topBottomCircleTexture;
    cylinder.normalsTop = normalsTop;
    cylinder.normalsBottom = normalsBottom;
    cylinder.normalsSides = normalsSides;
    cylinder.cylMaterial.shininess = shininess;
    cylinder.cylMaterial.specularColor = specularColor;

    cylinders.push_back(cylinder);
}

/*
* Method to create the mesh for a torus
* Creates vertices, normals, and UV coords
* Calculates the vertex
* Creates indices
* Creates VAO, VBO, and EBO objects and binds them
* Store the VAO, VBOs, index count, and model matrix for later use
* @params innerRadius: the inner radius of the torus
          outerRadius: the outer radius of the torus
          sides: number of sides to utilize
          rings: number of rings to utilize
          texture ID: the Texture ID associated with the object
          transformation: the translation that should be applied to the object
*/
void CreateTorusMesh(float innerRadius, float outerRadius, int sides, int rings, GLuint torusTextureID, float shininess, const glm::vec3& specularColor, const glm::vec3& translation) {
    // Calculate the necessary values    
    float thetaStep = 2.0f * glm::pi<float>() / rings;
    float phiStep = 2.0f * glm::pi<float>() / sides;

    // Data containers for vertices and indices
    vector<glm::vec3> vertices;
    vector<glm::vec2> texCoords;
    vector<unsigned int> indices;
    vector<glm::vec3> normals;

    // Create vertices, normals, and texture coordinates
    for (int i = 0; i <= rings; ++i) {
        float theta = i * thetaStep;

        for (int j = 0; j <= sides; ++j) {
            float phi = j * phiStep;

            // Calculate the coordinates of each vertex
            float x = (outerRadius + innerRadius * glm::cos(phi)) * glm::cos(theta);
            float y = innerRadius * glm::sin(phi);
            float z = (outerRadius + innerRadius * glm::cos(phi)) * glm::sin(theta);

            vertices.push_back(glm::vec3(x, y, z));

            // Calculate the normals
            glm::vec3 tubeCenter = glm::vec3(outerRadius * glm::cos(theta), 0, outerRadius * glm::sin(theta));
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z) - tubeCenter);
            normals.push_back(normal);

            // Calculate the texture coordinates
            float u = static_cast<float>(i) / rings;
            float v = static_cast<float>(j) / sides;
            texCoords.push_back(glm::vec2(u, v));
        }
    }

    // Create indices
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < sides; ++j) {
            int first = i * (sides + 1) + j;
            int second = first + sides + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    // Create and bind vertex array object (VAO)
    glGenVertexArrays(1, &torus.torusVAO);
    glBindVertexArray(torus.torusVAO);

    // Create vertex buffer object (VBO) for vertices
    glGenBuffers(1, &torus.torusVBO);
    glBindBuffer(GL_ARRAY_BUFFER, torus.torusVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for normals
    glGenBuffers(1, &torus.torusNormalVBO); 
    glBindBuffer(GL_ARRAY_BUFFER, torus.torusNormalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); 
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create vertex buffer object (VBO) for texture coordinates
    glGenBuffers(1, &torus.torusTextureVBO);
    glBindBuffer(GL_ARRAY_BUFFER, torus.torusTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    // Create element buffer object (EBO) for indices
    glGenBuffers(1, &torus.torusEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, torus.torusEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Unbind VAO
    glBindVertexArray(0);

    // Store the index count for later use
    torus.torusIndices = indices.size();

    // Store the model matrix for transformation
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translation);
    torus.torusMatrices.push_back(modelMatrix);

    // Store the data for later use in torus object
    torus.torusIndices = indices.size();
    torus.torusIndices = indices.size();
    torus.torusTextureID = torusTextureID;
    torus.normals = normals;
    torus.torusMaterial.shininess = shininess;
    torus.torusMaterial.specularColor = specularColor;
}


/*
* Method to create the mesh for a plane
* Creates vertices, normals, and indices
* Creates VAO, VBO, and EBO objects and binds them
* Store the VAO, VBOs, index count, and model matrix for later use
* @params width: the width of the plane
          height: the height of the plane
          planeTextureID: the texture to use on plane object
          translation: the translation that should be applied to the plane
*/
void CreatePlane(float width, float height, GLuint planeTextureID, float shininess, const glm::vec3& specularColor, const glm::vec3& translation) {

    // Calculate the necessary values
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    float vertices[] = {
        // Position (x, y, z)            // Normals (x, y, z)       // Texture Coordinates (s, t)
        -halfWidth, 0.0f, halfHeight,    0.0f, 1.0f, 0.0f,        0.0f, 0.0f,
        halfWidth, 0.0f, halfHeight,     0.0f, 1.0f, 0.0f,        1.0f, 0.0f,
        halfWidth, 0.0f, -halfHeight,    0.0f, 1.0f, 0.0f,        1.0f, 1.0f,
        -halfWidth, 0.0f, -halfHeight,   0.0f, 1.0f, 0.0f,        0.0f, 1.0f
    };

    // Indices for a quad (two triangles)
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    // Create and bind Vertex Array Object (VAO)
    glGenVertexArrays(1, &plane.planeVAO);
    glBindVertexArray(plane.planeVAO);

    // Create and bind Vertex Buffer Object (VBO) for vertices, normals, and textures
    glGenBuffers(1, &plane.planeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, plane.planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Set the vertex attribute pointers (position, normals, textures)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    // Create and bind Element Buffer Object (EBO)
    glGenBuffers(1, &plane.planeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, plane.planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Unbind VBO and VAO (do NOT unbind the EBO while VAO is bound)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), translation);
    plane.planeMatrices.push_back(modelMatrix);

    plane.planeIndices = sizeof(indices);
    plane.planeTextureID = planeTextureID;
    plane.planeMaterial.shininess = shininess;
    plane.planeMaterial.specularColor = specularColor;
}

/*
* Method to create the mesh for a cubes
* Creates vertices, which include normals and texture coords, and also creates indices
* Creates VAO, VBO, and EBO objects and binds them
* Store the VAO, VBOs, index count, and model matrix
* push cube object into cubes vector for rendering later
* @params width: the width of the cube
*         height: the height of the cube
*         depth: the depth of the cube
*         textureIDs: the texture ids to be used on each side of the cube
*         translation: the translation to be applied to the object
*         rotation: the rotation to be applied to the object
*         vector: vector to store all the cube objects
*/
void CreateCubeMesh(float width, float height, float depth, GLuint leftSideTexture, GLuint rightSideTexture, GLuint frontTexture, GLuint backTexture, GLuint topTexture, GLuint bottomTexture,
    float shininess, const glm::vec3& specularColor, const glm::vec3& translation, float rotation, vector<Cube>& cubes) {

    Cube newCube;

    // width, height, and depth are centered around the origin
    GLfloat halfW = width * 0.5f;
    GLfloat halfH = height * 0.5f;
    GLfloat halfD = depth * 0.5f;

    GLfloat vertices[] = {
        // Positions          // Normals                // Texture Coords
        // Front Face
         halfW,  halfH, halfD,  0.0f,  0.0f,  1.0f,     1.0f, 1.0f,
        -halfW,  halfH, halfD,  0.0f,  0.0f,  1.0f,     0.0f, 1.0f,
        -halfW, -halfH, halfD,  0.0f,  0.0f,  1.0f,     0.0f, 0.0f,
         halfW, -halfH, halfD,  0.0f,  0.0f,  1.0f,     1.0f, 0.0f,

        // Back Face
        -halfW, -halfH, -halfD,  0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
         halfW, -halfH, -halfD,  0.0f,  0.0f, -1.0f,    0.0f, 0.0f,
         halfW,  halfH, -halfD,  0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
        -halfW,  halfH, -halfD,  0.0f,  0.0f, -1.0f,    1.0f, 1.0f,

        // Left Face
        -halfW, -halfH,  halfD, -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
        -halfW, -halfH, -halfD, -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
        -halfW,  halfH, -halfD, -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
        -halfW,  halfH,  halfD, -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,

        // Right Face
         halfW, -halfH,  halfD,  1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
         halfW, -halfH, -halfD,  1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
         halfW,  halfH, -halfD,  1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
         halfW,  halfH,  halfD,  1.0f,  0.0f,  0.0f,    0.0f, 1.0f,

         // Bottom Face
         -halfW, -halfH,  halfD,  0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
          halfW, -halfH,  halfD,  0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
          halfW, -halfH, -halfD,  0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
         -halfW, -halfH, -halfD,  0.0f, -1.0f,  0.0f,   1.0f, 1.0f,

         // Top Face
         -halfW,  halfH,  halfD,  0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
          halfW,  halfH,  halfD,  0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
          halfW,  halfH, -halfD,  0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
         -halfW,  halfH, -halfD,  0.0f,  1.0f,  0.0f,   0.0f, 1.0f
    };

    // indices for all the faces
    GLuint indices[] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Bottom face
        16, 17, 18, 18, 19, 16,
        // Top face
        20, 21, 22, 22, 23, 20
    };

    // Create VAO
    glGenVertexArrays(1, &newCube.cubeVAO);
    glBindVertexArray(newCube.cubeVAO);

    // Generate the VBO
    glGenBuffers(1, &newCube.cubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, newCube.cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 3 floats for position, 3 floats for normals, and 2 floats for texture coordinates
    GLint posAttrib = 0, normAttrib = 1, texAttrib = 2;

    // set vertex attribute pointers
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(posAttrib);

    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(normAttrib);

    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(texAttrib);

    // Generate EBO
    glGenBuffers(1, &newCube.cubeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newCube.cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    float rotationRadians = glm::radians(rotation);

    // Store the transform
    newCube.translation = glm::translate(glm::mat4(1.0f), translation);
    newCube.rotation = glm::rotate(glm::mat4(1.0f), rotationRadians, glm::vec3(0.0f, 1.0f, 0.0f));

    // Store texture IDs for each face
    newCube.textures[0] = frontTexture;
    newCube.textures[1] = backTexture;
    newCube.textures[2] = leftSideTexture;
    newCube.textures[3] = rightSideTexture;
    newCube.textures[4] = bottomTexture;
    newCube.textures[5] = topTexture;
    newCube.cubeMaterial.shininess = shininess;
    newCube.cubeMaterial.specularColor = specularColor;

    cubes.push_back(newCube);
}

/*
* Method to create the mesh for a sphere
* Creates vertices, which include normals and texture coords, and also creates indices
* Creates VAO, VBO, and EBO objects and binds them
* Store the VAO, VBOs, index count, and model matrix
* push cube object into cubes vector for rendering later
* @params radius: radius of the sphere
*         textureID: Texture id that should be used on the object
*         translation: the translation that should be applied to the object
*/
void CreateSphereMesh(float radius, GLuint sphereTextureID, float shininess, const glm::vec3& specularColor, const glm::vec3& translation) {
    vector<GLfloat> vertices;
    vector<GLuint> indices;

    int precision = 50; // adjust this for more or fewer triangles

    // Calculate the vertexs, normals, and texture coords
    for (int i = 0; i <= precision; i++) {
        float theta = float(i) * glm::pi<float>() / precision;
        for (int j = 0; j <= precision; j++) {
            float phi = float(j) * 2 * glm::pi<float>() / precision;

            // Vertex position
            float x = radius * sin(theta) * cos(phi);
            float y = radius * cos(theta);
            float z = radius * sin(theta) * sin(phi);

            // Vertex normal
            glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));

            // Texture coordinates
            float u = float(j) / precision;
            float v = 1.0f - float(i) / precision; // Flip so 0 is at the top

            vertices.insert(vertices.end(), { x, y, z, normal.x, normal.y, normal.z, u, v });
        }
    }

    // calculate indices
    for (int i = 0; i < precision; i++) {
        for (int j = 0; j < precision; j++) {
            // Vertex indices for the quad forming two triangles
            GLuint bottomLeft = i * (precision + 1) + j;
            GLuint bottomRight = i * (precision + 1) + j + 1;
            GLuint topLeft = (i + 1) * (precision + 1) + j;
            GLuint topRight = (i + 1) * (precision + 1) + j + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomRight);
            indices.push_back(bottomLeft);

            // Second triangle
            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomRight);
        }
    }

    // Set up the VAO/VBO
    glGenVertexArrays(1, &sphere.sphereVAO);
    glBindVertexArray(sphere.sphereVAO);

    glGenBuffers(1, &sphere.sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphere.sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    // Set vertex attribute pointers
    GLint posAttrib = 0, normAttrib = 1, texAttrib = 2;
    glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(posAttrib);

    glVertexAttribPointer(normAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(normAttrib);

    glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(texAttrib);

    // Create and bind Element Buffer Object (EBO)
    glGenBuffers(1, &sphere.sphereEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere.sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Store the transform
    sphere.translation = glm::translate(glm::mat4(1.0f), translation);

    // Store texture ID
    sphere.texture = sphereTextureID;
    sphere.sphereIndices = indices.size();
    sphere.sphereMaterial.shininess = shininess;
    sphere.sphereMaterial.specularColor = specularColor;
}

/*
*  Function to create light cubes, all the same size
*  Used to represent the lights in the scene
*  @params vector: Vector to hold all the light cubes
*/ 

void CreateLightCubes(vector<LightCube>& lCubes) {
    LightCube cubes;

    // Vertices for a cube with a width, height, and depth of 1.0f (centered at the origin)
    float vertices[] = {
        // Position
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0, // Front face
        1, 5, 6, 6, 2, 1, // Right face
        5, 4, 7, 7, 6, 5, // Back face
        4, 0, 3, 3, 7, 4, // Left face
        3, 2, 6, 6, 7, 3, // Top face
        4, 5, 1, 1, 0, 4  // Bottom face
    };

    // Create and bind Vertex Array Object (VAO)
    glGenVertexArrays(1, &cubes.lCubeVAO);
    glBindVertexArray(cubes.lCubeVAO);

    // Create and bind Vertex Buffer Object (VBO)
    glGenBuffers(1, &cubes.lCubeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cubes.lCubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Create and bind Element Buffer Object (EBO)
    glGenBuffers(1, &cubes.lCubeEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubes.lCubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Unbind VBO and VAO (do NOT unbind the EBO while VAO is bound)
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    lCubes.push_back(cubes);
}

/*
* Method to load a texture from an image file and create a texture object
* takes file name input and loads that file and flips it vertically
* It then generates an OpenGL texture object and binds the loaded image data to it.
* The function sets the texture wrapping and filtering parameters and generates mipmaps.
* @params texturePath: The path to the texture that should be loaded
*/
GLuint LoadTexture(const std::string& texturePath) {
    int width, height, channels;
    unsigned char* image = stbi_load(texturePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!image) {
        std::cerr << "Failed to load texture: " << texturePath << std::endl;
        return 0;
    }

    // call function to flip image
    flipImageVertically(image, width, height, channels);

    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);

    // Set texture wrapping and filtering options (optional)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (channels == 3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels == 4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else
    {
        cout << "Not implemented to handle image with " << channels << " channels" << endl;
        return false;
    }

    // Generate mipmaps (optional, but recommended)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Free the image data
    stbi_image_free(image);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

// Method to destroy texture program
void DestroyTexture(GLuint textureId) {
    glDeleteTextures(1, &textureId);
}

/*
* Render function to display the scene
* Sets up the view and projection matrices
* Binds the shader program
* Applies the combined model matrix for transformation as well as a combined model matrix
* Combined model matrix is utilized to transform the cylinders and torus as a single object
* Iterates through the stored structs and renders each object
* @params cylinders: Vector that holds all the cylinder objects for rendering
*         cubes: Vector that holds all the cube objects for rendering
*         lightCubes: Vector that holds all lightCube objects for rendering
*/
void Render(const vector<Cylinder>& cylinders, const vector<Cube>& cubes, const vector<LightCube>& lightCubes) {
    glEnable(GL_DEPTH_TEST);

    // Clear the color and depth buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Use the shader program
    glUseProgram(objectProgramId);

    // Transforms the camera
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection;

    // sets the project to Ortho or Perspective based on current keyboard input
    if (camera.GetProjectionMatrix()) {
        projection = glm::perspective(glm::radians(camera.Zoom), (GLfloat)SCR_WIDTH / (GLfloat)SCR_HEIGHT, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

    // Set the view and projection matrices
    glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Rotation around axis for cylinders and torus
    float rotationAngleX = glm::radians(-90.0f); // Adjust the angle as needed for X-axis
    float rotationAngleY = glm::radians(35.0f); // Adjust the angle as needed for Y-axis
    float rotationAngleZ = glm::radians(0.0f); // Adjust the angle as needed for Z-axis
    glm::mat4 rotationMatrixX = glm::rotate(glm::mat4(1.0f), rotationAngleX, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotationMatrixY = glm::rotate(glm::mat4(1.0f), rotationAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotationMatrixZ = glm::rotate(glm::mat4(1.0f), rotationAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));

    // Create a single model matrix for the combined transformation of cylinders and torus
    glm::mat4 combinedModelMatrix = glm::mat4(1.0f);

    // Apply transformations to the combined model matrix for cylinders and torus
    glm::vec3 translation(-1.0f, 1.0f, 0.6f);
    combinedModelMatrix = glm::rotate(combinedModelMatrix, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    combinedModelMatrix = glm::translate(combinedModelMatrix, translation);
    combinedModelMatrix = glm::scale(combinedModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f));

    // Apply the rotation to the combined model matrix (cylinders and torus only)
    glm::mat4 combinedModelMatrixWithRotation = rotationMatrixZ * rotationMatrixY * rotationMatrixX * combinedModelMatrix;

    // Set the combined model matrix uniform for the shader program
    glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(combinedModelMatrix));

    GLint UVScaleLoc = glGetUniformLocation(objectProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(uvScale));

    // Set the point light properties in the shader
    GLint pointLightLoc = glGetUniformLocation(objectProgramId, "pointLights");
    int numPointLights = 3;

    for (int i = 0; i < numPointLights; ++i) {
        std::string baseName = "pointLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(objectProgramId, (baseName + "position").c_str()), 1, glm::value_ptr(pointLights[i].position));
        glUniform3fv(glGetUniformLocation(objectProgramId, (baseName + "color").c_str()), 1, glm::value_ptr(pointLights[i].color));
        glUniform1f(glGetUniformLocation(objectProgramId, (baseName + "intensity").c_str()), pointLights[i].intensity);
        glUniform1f(glGetUniformLocation(objectProgramId, (baseName + "ambientStrength").c_str()), pointLights[i].ambientStrength);
        glUniform1f(glGetUniformLocation(objectProgramId, (baseName + "specularIntensity").c_str()), pointLights[i].specularIntensity);
        glUniform1f(glGetUniformLocation(objectProgramId, (baseName + "highlightSize").c_str()), pointLights[i].highlightSize);
    }

    // Code to Render the cylinders
    for (const auto& cylinder : cylinders) {
        for (const auto& transformMatrix : cylinder.CylinderMatrices) {
            // Set material properties in the shader
            GLint shininessLoc = glGetUniformLocation(objectProgramId, "material.shininess");
            glUniform1f(shininessLoc, cylinder.cylMaterial.shininess);

            GLint specularColorLoc = glGetUniformLocation(objectProgramId, "material.specularColor");
            glUniform3fv(specularColorLoc, 1, glm::value_ptr(cylinder.cylMaterial.specularColor));
            
            // Calculate the combined model matrix for the current cylinder and its specific transformation
            glm::mat4 combinedModelMatrixWithRotationAndTransform = combinedModelMatrixWithRotation * transformMatrix;

            // Set the combined model matrix uniform for the shader program
            glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(combinedModelMatrixWithRotationAndTransform));

            // Bind the appropriate VAO for the sides
            glBindVertexArray(cylinder.cylinderSidesVAO);
            glEnableVertexAttribArray(1); 
            glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderSideTexture);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindTexture(GL_TEXTURE_2D, cylinder.sideTextureID);
            glDrawElements(GL_TRIANGLES, cylinder.cylinderSidesIndices, GL_UNSIGNED_INT, 0);
            glDisableVertexAttribArray(1); 

            // Bind the appropriate VAO for the top circle
            glBindVertexArray(cylinder.cylinderTopVAO);
            glEnableVertexAttribArray(1); 
            glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderTopTexture);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindTexture(GL_TEXTURE_2D, cylinder.topBottomTextureID);
            glDrawElements(GL_TRIANGLES, cylinder.cylinderTopIndices, GL_UNSIGNED_INT, 0);
            glDisableVertexAttribArray(1); 

            // Bind the appropriate VAO for the bottom circle
            glBindVertexArray(cylinder.cylinderBottomVAO);
            glEnableVertexAttribArray(1);
            glBindBuffer(GL_ARRAY_BUFFER, cylinder.cylinderBottomTexture);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
            glBindTexture(GL_TEXTURE_2D, cylinder.topBottomTextureID);
            glDrawElements(GL_TRIANGLES, cylinder.cylinderBottomIndices, GL_UNSIGNED_INT, 0);
            glDisableVertexAttribArray(1);
        }
    }

    // Code to Render the torus
    for (const auto& transformMatrix : torus.torusMatrices) {
        // Set material properties in the shader
        GLint shininessLoc = glGetUniformLocation(objectProgramId, "material.shininess");
        glUniform1f(shininessLoc, torus.torusMaterial.shininess);

        GLint specularColorLoc = glGetUniformLocation(objectProgramId, "material.specularColor");
        glUniform3fv(specularColorLoc, 1, glm::value_ptr(torus.torusMaterial.specularColor));
        
        // Calculate the combined model matrix for the torus and its specific transformation
        glm::mat4 combinedModelMatrixWithRotationAndTransform = combinedModelMatrixWithRotation * transformMatrix;

        // Set the combined model matrix uniform for the shader program
        glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(combinedModelMatrixWithRotationAndTransform));

        glBindVertexArray(torus.torusVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, torus.torusTextureID);
        glDrawElements(GL_TRIANGLES, torus.torusIndices, GL_UNSIGNED_INT, 0);
    }

    // Code to Render the plane
    for (const auto& transformMatrix : plane.planeMatrices) {
        // Set material properties in the shader
        GLint shininessLoc = glGetUniformLocation(objectProgramId, "material.shininess");
        glUniform1f(shininessLoc, plane.planeMaterial.shininess);

        GLint specularColorLoc = glGetUniformLocation(objectProgramId, "material.specularColor");
        glUniform3fv(specularColorLoc, 1, glm::value_ptr(plane.planeMaterial.specularColor));
        
        glm::mat4 planeMatrix = transformMatrix;

        // Set the combined model matrix uniform for the shader program
        glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(planeMatrix));

        glBindVertexArray(plane.planeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, plane.planeTextureID);
        glDrawElements(GL_TRIANGLES, plane.planeIndices, GL_UNSIGNED_INT, 0);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Code to Render Cubes
    for (const auto& cube : cubes) {
        // Set material properties in the shader
        GLint shininessLoc = glGetUniformLocation(objectProgramId, "material.shininess");
        glUniform1f(shininessLoc, cube.cubeMaterial.shininess);

        GLint specularColorLoc = glGetUniformLocation(objectProgramId, "material.specularColor");
        glUniform3fv(specularColorLoc, 1, glm::value_ptr(cube.cubeMaterial.specularColor));
        
        glBindVertexArray(cube.cubeVAO);

        // Set the model matrix for this cube
        glm::mat4 model = cube.translation * cube.rotation;
        glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(model));

        for (int i = 0; i < 6; ++i) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cube.textures[i]);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(i * 6 * sizeof(GLuint)));
        }
    }

    // Code to Render the Sphere
    // Set material properties in the shader
    GLint shininessLoc = glGetUniformLocation(objectProgramId, "material.shininess");
    glUniform1f(shininessLoc, sphere.sphereMaterial.shininess);

    GLint specularColorLoc = glGetUniformLocation(objectProgramId, "material.specularColor");
    glUniform3fv(specularColorLoc, 1, glm::value_ptr(sphere.sphereMaterial.specularColor));
    
    // Set the model matrix for this sphere
    glm::mat4 model = sphere.translation;
    glUniformMatrix4fv(glGetUniformLocation(objectProgramId, "model"), 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(sphere.sphereVAO);

    // Bind the sphere's texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sphere.texture);

    // Draw the sphere
    glDrawElements(GL_TRIANGLES, sphere.sphereIndices, GL_UNSIGNED_INT, 0);

    // Use the shader program for lights
    glUseProgram(lightProgramId);

    // Set the view and projection matrices for the lights
    glUniformMatrix4fv(glGetUniformLocation(lightProgramId, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(lightProgramId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Render light cubes
    for (const auto& pointLight : pointLights) {
        for (const auto& lightCube : lightCubes) {
            // Calculate the combined model matrix for the light cube and its specific transformation
            glm::mat4 lightCubeModelMatrix = glm::mat4(1.0f);
            lightCubeModelMatrix = glm::translate(lightCubeModelMatrix, pointLight.position);
            lightCubeModelMatrix = glm::scale(lightCubeModelMatrix, glm::vec3(0.2f));

            // Set the combined model matrix uniform for the shader program
            glUniformMatrix4fv(glGetUniformLocation(lightProgramId, "model"), 1, GL_FALSE, glm::value_ptr(lightCubeModelMatrix));

            glBindVertexArray(lightCube.lCubeVAO);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        }
    }

    // Unbind the VAO and textures
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}


/*
* Create shader program
* Compiles the vertex and fragment shaders
* Links them into a shader program
* @params vtxShaderSource: Vertex shader source code
*         fragShaderSource: Fragment shader source code
*         programId: Reference to the generated shader program
* @return true if the shader program creation is successful, false otherwise
*/
bool CreateShaders(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId) {
    // Create vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vtxShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "Vertex shader compilation failed:\n" << infoLog << endl;
        return false;
    }

    // Create fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Fragment shader compilation failed:\n" << infoLog << endl;
        return false;
    }

    // Create shader program
    programId = glCreateProgram();
    glAttachShader(programId, vertexShader);
    glAttachShader(programId, fragmentShader);
    glLinkProgram(programId);

    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(programId, 512, nullptr, infoLog);
        cout << "Shader program linking failed:\n" << infoLog << endl;
        return false;
    }

    // Cleanup shader objects
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

/*
* Destroy Shaders function
* Destorys program
*/
void DestroyShaders(GLuint programId) {
    glDeleteProgram(programId);
}

/*
* Entry point of the program
* Initializes the GLFW library and creates a window
* Loads OpenGL function pointers with GLAD
* Creates shader program
* Loads all textures to map
* Creates mesh objects for cylinders and torus
* Enters the main render loop
* Cleans up resources before exiting
* @params argc: Number of command-line arguments
*         argv: Array of command-line argument strings
* @return EXIT_SUCCESS if the program runs successfully, EXIT_FAILURE otherwise
*/
int main(int argc, char* argv[]) {
    // Initialize GLFW and create a window
    if (!Initialize(argc, argv, &window))
        return EXIT_FAILURE;

    // Create the shader program for the objects
    if (!CreateShaders(vertexShaderSource, fragmentShaderSource, objectProgramId)) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Create the shader program for the lights
    if (!CreateShaders(lightVertexShaderSource, lightFragmentShaderSource, lightProgramId)) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // load all textures to be utilized
    textures["cylTopLargeTexture"] = LoadTexture("cylTopLarge.png");
    textures["cylTopSmallTexture"] = LoadTexture("cylTopSmall.png");
    textures["cylSidesLongTexture"] = LoadTexture("cylLongSide.png");
    textures["cylSidesTexture"] = LoadTexture("cylSide.png");
    textures["Torus"] = LoadTexture("torus.png");
    textures["Plane"] = LoadTexture("plane.png");
    textures["Sphere"] = LoadTexture("marble.png");
    textures["smallLeftSide"] = LoadTexture("left_face.png");
    textures["smallRightSide"] = LoadTexture("right_face.png");
    textures["smallFrontSide"] = LoadTexture("front_face.png");
    textures["smallBackSide"] = LoadTexture("back_face.png");
    textures["smallTopSide"] = LoadTexture("top_face.png");
    textures["largeLeftSide"] = LoadTexture("Lleft_face.png");
    textures["largeRightSide"] = LoadTexture("Lright_face.png");
    textures["largeFrontSide"] = LoadTexture("Lfront_face.png");
    textures["largeBackSide"] = LoadTexture("Lback_face.png");
    textures["largeTopSide"] = LoadTexture("Ltop_face.png");


    // Declare a vectors to hold all the cylinder, cube, and lightCube objects
    vector<Cylinder> cylinders;
    vector<Cube> cubes;
    vector<LightCube> lCubes;

    // Create all mesh objects
    CreateCylinderMesh(0.1175f, 1.4f, 32, 12, textures["cylTopSmallTexture"], textures["cylSidesLongTexture"], 16.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), cylinders);
    CreateCylinderMesh(0.25f, 0.08f, 24, 8, textures["cylTopSmallTexture"], textures["cylSidesTexture"], 16.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, -0.7f, 0.0f), cylinders);
    CreateCylinderMesh(0.3f, 0.08f, 24, 8, textures["cylTopLargeTexture"], textures["cylSidesTexture"], 16.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.7f, 0.0f), cylinders);
    CreateTorusMesh(0.04f, 0.26f, 20, 34, textures["Torus"], 128.0f, glm::vec3(0.98f, 0.92f, 0.84f), glm::vec3(0.0f, -0.7f, 0.0f));
    CreatePlane(18.0f, 18.0f, textures["Plane"], 16.0f, glm::vec3(0.98f, 0.92f, 0.84f), glm::vec3(0.0f, 0.0f, 0.0f));
    CreateCubeMesh(1.8f, 1.6f, 2.8f, textures["largeLeftSide"], textures["largeRightSide"], textures["largeFrontSide"], textures["largeBackSide"], textures["largeTopSide"], textures["largeTopSide"],
        128.0f, glm::vec3(0.98f, 0.92f, 0.84f), glm::vec3(2.0f, 0.8f, 1.1f), -40.0f, cubes);
    CreateCubeMesh(1.4f, 0.6, 1.4f, textures["smallLeftSide"], textures["smallRightSide"], textures["smallFrontSide"], textures["smallBackSide"], textures["smallTopSide"], textures["smallTopSide"],
        128.0f, glm::vec3(0.98f, 0.92f, 0.84f), glm::vec3(-2.0f, 0.3f, 2.5f), 10.0f, cubes);
    CreateSphereMesh(0.22f, textures["Sphere"], 128.0f, glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.22f, 2.5f));
    CreateLightCubes(lCubes);
    CreateLightCubes(lCubes);

    glUseProgram(objectProgramId);
    
    // Assign properties to point lights
    // point light 1
    pointLights[0].position = glm::vec3(-4.0f, 8.0f, 1.0f);
    pointLights[0].color = glm::vec3(0.98f, 0.92f, 0.84f);
    pointLights[0].intensity = 0.6f;
    pointLights[0].ambientStrength = 0.2f;
    pointLights[0].specularIntensity = 1.0f;
    pointLights[0].highlightSize = 0.1f;

    // point light 2
    pointLights[1].position = glm::vec3(4.0f, 8.0f, -1.0f);
    pointLights[1].color = glm::vec3(0.98f, 0.92f, 0.84f);
    pointLights[1].intensity = 0.6f;
    pointLights[1].ambientStrength = 0.2f;
    pointLights[1].specularIntensity = 1.0f;
    pointLights[1].highlightSize = 0.1f;

    // point light 3
    pointLights[2].position = glm::vec3(9.0f, 1.0f, 0.0f);
    pointLights[2].color = glm::vec3(1.0f, 1.0f, 1.0f);
    pointLights[2].intensity = 0.2f;
    pointLights[2].ambientStrength = 0.8f;
    pointLights[2].specularIntensity = 0.25f;
    pointLights[2].highlightSize = 0.1f;

    glUniform1i(glGetUniformLocation(objectProgramId, "uTexture"), 0);

    // Main render loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ProcessInput(window);

        Render(cylinders, cubes, lCubes);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Clean up resources
    DestroyShaders(objectProgramId);
    DestroyShaders(lightProgramId);

    DestroyTexture(textures["Plane"]);
    DestroyTexture(textures["cylTopLargeTexture"]);
    DestroyTexture(textures["cylTopSmallTexture"]);
    DestroyTexture(textures["cylSidesLongTexture"]);
    DestroyTexture(textures["cylSidesTexture"]);
    DestroyTexture(textures["Torus"]);
    DestroyTexture(textures["smallLeftSide"]);
    DestroyTexture(textures["smallRightSide"]);
    DestroyTexture(textures["smallTopSide"]);
    DestroyTexture(textures["smallBackSide"]);
    DestroyTexture(textures["smallFrontSide"]);
    DestroyTexture(textures["largeLeftSide"]);
    DestroyTexture(textures["largeRightSide"]);
    DestroyTexture(textures["largeTopSide"]);
    DestroyTexture(textures["largeBackSide"]);
    DestroyTexture(textures["largeFrontSide"]);
    DestroyTexture(textures["Sphere"]);

    glfwTerminate();

    return EXIT_SUCCESS;
}
