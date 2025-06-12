//
//  Display a Rubik's cube (3x3x3)
//

#include "Angel.h"
#include <vector>
#include <cstdlib>  // For rand() and srand()
#include <ctime>    // For time()

typedef vec4  color4;
typedef vec4  point4;

// Each subcube has 36 vertices (6 faces, 2 triangles/face, 3 vertices/triangle)
const int NumVerticesPerCube = 36;
// 3x3x3 = 27 subcubes in a Rubik's cube
const int NumCubes = 27;

// Store the original vertices of a unit cube
point4 unit_vertices[8] = {
    point4( -0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5,  0.5,  0.5, 1.0 ),
    point4(  0.5, -0.5,  0.5, 1.0 ),
    point4( -0.5, -0.5, -0.5, 1.0 ),
    point4( -0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5,  0.5, -0.5, 1.0 ),
    point4(  0.5, -0.5, -0.5, 1.0 )
};

// Colors for cube faces
color4 face_colors[6] = {
    color4( 1.0, 0.0, 0.0, 1.0 ),  // red (front)
    color4( 1.0, 0.5, 0.0, 1.0 ),  // orange (back)
    color4( 0.0, 1.0, 0.0, 1.0 ),  // green (right)
    color4( 0.0, 0.0, 1.0, 1.0 ),  // blue (left)
    color4( 1.0, 1.0, 1.0, 1.0 ),  // white (top)
    color4( 1.0, 1.0, 0.0, 1.0 )   // yellow (bottom)
};

// Black color for internal faces
color4 black = color4(0.0, 0.0, 0.0, 1.0);

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
int      Axis = Xaxis;
GLfloat  Theta[NumAxes] = { 30.0, 30.0, 0.0 };

// Variables for slice rotation
bool isRotating = false;
int rotatingSlice = -1;
int rotationAxis = -1;
float rotationAngle = 0.0f;
float rotationIncrement = 3.0f;
int rotationDirection = 1;

// Variables for scrambling
bool isScrambling = false;
int scramblingMoves = 0;
int maxScramblingMoves = 20;
int currentScramblingMove = 0;

// Variables for mouse interaction
bool leftMousePressed = false;
double lastX = 0.0, lastY = 0.0;

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

// Gap between subcubes
const float gap = 0.01;
// Size of each subcube
const float cubeSize = 0.3;

// Buffer objects
GLuint vao;
GLuint buffer;
GLuint program;

// Structure to store a single subcube
struct Subcube {
    int x, y, z;          // Grid position (0-2)
    vec3 position;        // 3D position
    mat4 transform;       // Transformation matrix
    point4 vertices[8];   // Transformed vertices
    int indices[36];      // Index mapping for vertices
    color4 colors[36];    // Colors for each vertex
    bool drawn;           // Should this cube be drawn

    // Initialize a subcube at grid position (x,y,z)
    void init(int _x, int _y, int _z) {
        x = _x;
        y = _y;
        z = _z;
        
        // Calculate 3D position
        position.x = (x - 1) * (cubeSize + gap);
        position.y = (y - 1) * (cubeSize + gap);
        position.z = (z - 1) * (cubeSize + gap);
        
        // Initialize transform
        transform = Translate(position);
        
        // Initialize vertices by scaling and translating unit cube
        for (int i = 0; i < 8; i++) {
            vertices[i] = unit_vertices[i];
            vertices[i].x *= cubeSize;
            vertices[i].y *= cubeSize;
            vertices[i].z *= cubeSize;
            vertices[i] = transform * vertices[i];
        }
        
        drawn = true;
    }
    
    // Update the transform and vertices of this subcube
    void updateTransform(mat4 newTransform) {
        transform = newTransform;
        for (int i = 0; i < 8; i++) {
            point4 baseVertex = unit_vertices[i];
            baseVertex.x *= cubeSize;
            baseVertex.y *= cubeSize;
            baseVertex.z *= cubeSize;
            vertices[i] = transform * baseVertex;
        }
    }
    
    // Apply a rotation to this subcube
    void rotate(int axis, float angle) {
        mat4 rotation;
        if (axis == Xaxis) {
            rotation = RotateX(angle);
        } else if (axis == Yaxis) {
            rotation = RotateY(angle);
        } else { // Zaxis
            rotation = RotateZ(angle);
        }
        
        updateTransform(rotation * transform);
    }
    
    // Determine if this subcube is part of the specified slice
    bool isInSlice(int axis, int slice) {
        if (axis == Xaxis) return x == slice;
        if (axis == Yaxis) return y == slice;
        if (axis == Zaxis) return z == slice;
        return false;
    }
};

// Array of all subcubes
Subcube subcubes[NumCubes];

//----------------------------------------------------------------------------

// Generate geometry for a single subcube
void generateSubcubeGeometry(Subcube &cube) {
    int index = 0;
    
    bool isExternalFront = (cube.z == 2);
    bool isExternalBack = (cube.z == 0);
    bool isExternalRight = (cube.x == 2);
    bool isExternalLeft = (cube.x == 0);
    bool isExternalTop = (cube.y == 2);
    bool isExternalBottom = (cube.y == 0);
    
    // Build indices and colors for all faces
    
    // Front face (vertices 0, 1, 2, 3)
    color4 frontColor = isExternalFront ? face_colors[0] : black;
    cube.indices[index] = 1; cube.colors[index] = frontColor; index++;
    cube.indices[index] = 0; cube.colors[index] = frontColor; index++;
    cube.indices[index] = 3; cube.colors[index] = frontColor; index++;
    cube.indices[index] = 1; cube.colors[index] = frontColor; index++;
    cube.indices[index] = 3; cube.colors[index] = frontColor; index++;
    cube.indices[index] = 2; cube.colors[index] = frontColor; index++;
    
    // Back face (vertices 4, 5, 6, 7)
    color4 backColor = isExternalBack ? face_colors[1] : black;
    cube.indices[index] = 5; cube.colors[index] = backColor; index++;
    cube.indices[index] = 4; cube.colors[index] = backColor; index++;
    cube.indices[index] = 7; cube.colors[index] = backColor; index++;
    cube.indices[index] = 5; cube.colors[index] = backColor; index++;
    cube.indices[index] = 7; cube.colors[index] = backColor; index++;
    cube.indices[index] = 6; cube.colors[index] = backColor; index++;
    
    // Right face (vertices 2, 3, 7, 6)
    color4 rightColor = isExternalRight ? face_colors[2] : black;
    cube.indices[index] = 2; cube.colors[index] = rightColor; index++;
    cube.indices[index] = 3; cube.colors[index] = rightColor; index++;
    cube.indices[index] = 7; cube.colors[index] = rightColor; index++;
    cube.indices[index] = 2; cube.colors[index] = rightColor; index++;
    cube.indices[index] = 7; cube.colors[index] = rightColor; index++;
    cube.indices[index] = 6; cube.colors[index] = rightColor; index++;
    
    // Left face (vertices 0, 1, 5, 4)
    color4 leftColor = isExternalLeft ? face_colors[3] : black;
    cube.indices[index] = 1; cube.colors[index] = leftColor; index++;
    cube.indices[index] = 5; cube.colors[index] = leftColor; index++;
    cube.indices[index] = 4; cube.colors[index] = leftColor; index++;
    cube.indices[index] = 1; cube.colors[index] = leftColor; index++;
    cube.indices[index] = 4; cube.colors[index] = leftColor; index++;
    cube.indices[index] = 0; cube.colors[index] = leftColor; index++;
    
    // Top face (vertices 1, 5, 6, 2)
    color4 topColor = isExternalTop ? face_colors[4] : black;
    cube.indices[index] = 1; cube.colors[index] = topColor; index++;
    cube.indices[index] = 5; cube.colors[index] = topColor; index++;
    cube.indices[index] = 6; cube.colors[index] = topColor; index++;
    cube.indices[index] = 1; cube.colors[index] = topColor; index++;
    cube.indices[index] = 6; cube.colors[index] = topColor; index++;
    cube.indices[index] = 2; cube.colors[index] = topColor; index++;
    
    // Bottom face (vertices 0, 4, 7, 3)
    color4 bottomColor = isExternalBottom ? face_colors[5] : black;
    cube.indices[index] = 0; cube.colors[index] = bottomColor; index++;
    cube.indices[index] = 4; cube.colors[index] = bottomColor; index++;
    cube.indices[index] = 7; cube.colors[index] = bottomColor; index++;
    cube.indices[index] = 0; cube.colors[index] = bottomColor; index++;
    cube.indices[index] = 7; cube.colors[index] = bottomColor; index++;
    cube.indices[index] = 3; cube.colors[index] = bottomColor; index++;
}

// Initialize all subcubes with their positions and geometry
void initializeSubcubes() {
    int cubeIndex = 0;
    for (int x = 0; x < 3; x++) {
        for (int y = 0; y < 3; y++) {
            for (int z = 0; z < 3; z++) {
                subcubes[cubeIndex].init(x, y, z);
                generateSubcubeGeometry(subcubes[cubeIndex]);
                cubeIndex++;
            }
        }
    }
}

// Draw a single subcube
void drawSubcube(Subcube &cube) {
    if (!cube.drawn) return;
    
    // Upload vertex and color data for this cube
    point4 vertices[NumVerticesPerCube];
    color4 colors[NumVerticesPerCube];
    
    // Map indices to vertices and set colors
    for (int i = 0; i < NumVerticesPerCube; i++) {
        vertices[i] = cube.vertices[cube.indices[i]];
        colors[i] = cube.colors[i];
    }
    
    // Update buffer with vertex and color data
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices), sizeof(colors), colors);
    
    // Draw the subcube
    glDrawArrays(GL_TRIANGLES, 0, NumVerticesPerCube);
}

// Start rotating a slice
void startSliceRotation(int axis, int slice, int direction) {
    if (!isRotating) {
        isRotating = true;
        rotatingSlice = slice;
        rotationAxis = axis;
        rotationDirection = direction;
        rotationAngle = 0.0f;
    }
}

// Perform a random move for scrambling
void performRandomMove() {
    if (isRotating) return;
    
    // Pick a random axis (X, Y, or Z)
    int axis = rand() % 3;
    
    // Pick a random slice (0, 1, or 2)
    int slice = rand() % 3;
    
    // Pick a random direction (1 for clockwise, -1 for counter-clockwise)
    int direction = (rand() % 2) * 2 - 1;
    
    // Start the rotation
    startSliceRotation(axis, slice, direction);
}

// Start scrambling the cube
void startScrambling(int moves) {
    if (!isScrambling && !isRotating) {
        isScrambling = true;
        maxScramblingMoves = moves;
        currentScramblingMove = 0;
        performRandomMove();
    }
}

// Update function to handle cube animations
void update(void)
{
    if (isRotating) {
        // Increment rotation
        rotationAngle += rotationIncrement * rotationDirection;
        
        // Determine if rotation is complete (90 degrees)
        if (fabs(rotationAngle) >= 90.0f + rotationIncrement) {
            // Apply the final rotation to exactly 90 degrees
            float finalAngle = rotationDirection > 0 ? 90.0f + rotationIncrement : -90.0f - rotationIncrement;
            
            // Update cube positions and orientations
            for (int i = 0; i < NumCubes; i++) {
                if (subcubes[i].isInSlice(rotationAxis, rotatingSlice)) {
                    // First apply the exact rotation
                    subcubes[i].rotate(rotationAxis, finalAngle - rotationAngle);
                    
                    // Then update grid positions
                    int oldX = subcubes[i].x;
                    int oldY = subcubes[i].y;
                    int oldZ = subcubes[i].z;
                    
                    // Update grid coordinates based on rotation
                    if (rotationAxis == Xaxis) {
                        // X rotation changes y and z
                        if (rotationDirection > 0) {
                            subcubes[i].y = 2 - oldZ;
                            subcubes[i].z = oldY;
                        } else {
                            subcubes[i].y = oldZ;
                            subcubes[i].z = 2 - oldY;
                        }
                    }
                    else if (rotationAxis == Yaxis) {
                        // Y rotation changes x and z
                        if (rotationDirection > 0) {
                            subcubes[i].x = oldZ;
                            subcubes[i].z = 2 - oldX;
                        } else {
                            subcubes[i].x = 2 - oldZ;
                            subcubes[i].z = oldX;
                        }
                    }
                    else { // Zaxis
                        // Z rotation changes x and y
                        if (rotationDirection > 0) {
                            subcubes[i].x = 2 - oldY;
                            subcubes[i].y = oldX;
                        } else {
                            subcubes[i].x = oldY;
                            subcubes[i].y = 2 - oldX;
                        }
                    }
                }
            }
            
            // Reset rotation state
            isRotating = false;
            
            // If we're scrambling, continue with the next move
            if (isScrambling) {
                currentScramblingMove++;
                if (currentScramblingMove < maxScramblingMoves) {
                    performRandomMove();
                } else {
                    isScrambling = false;
                    rotationIncrement = 3.0f;
                }
            }
        }
        else {
            // Apply incremental rotation to subcubes in the slice
            for (int i = 0; i < NumCubes; i++) {
                if (subcubes[i].isInSlice(rotationAxis, rotatingSlice)) {
                    subcubes[i].rotate(rotationAxis, rotationIncrement * rotationDirection);
                }
            }
        }
    }
    else if (isScrambling && currentScramblingMove < maxScramblingMoves) {
        performRandomMove();
    }
}

//---------------------------------------------------------------------
//
// init
//

void init()
{
    // Load shaders and use the resulting shader program
    program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    
    // Initialize all subcubes
    initializeSubcubes();

    // Create a vertex array object
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create and initialize a buffer object for one cube at a time
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(point4) * NumVerticesPerCube + sizeof(color4) * NumVerticesPerCube, NULL, GL_DYNAMIC_DRAW);
    
    // Set up vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * NumVerticesPerCube));

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    
    // Set projection matrix
    mat4 projection;
    projection = Perspective(45.0, 1.0, 0.1, 100.0); // Use perspective projection for better 3D view
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5, 0.5, 0.5, 1.0); // Dark gray background
}

//---------------------------------------------------------------------
//
// display
//

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Position the camera slightly back to see the entire cube
    const vec3 displacement(0.0, 0.0, -3.0);
    mat4 model_view = (Translate(displacement) *
                       RotateX(Theta[Xaxis]) *
                       RotateY(Theta[Yaxis]) *
                       RotateZ(Theta[Zaxis]));
    
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    
    // Draw each subcube individually
    for (int i = 0; i < NumCubes; i++) {
        drawSubcube(subcubes[i]);
    }
    
    glFinish();
}

// Function to display help information
void displayHelp() {
    std::cout << "\n=== RUBIK'S CUBE CONTROLS ===\n";
    std::cout << "Mouse Controls:\n";
    std::cout << "  Left-click and drag: Rotate the entire cube\n";
    std::cout << "\nKeyboard Controls:\n";
    std::cout << "  h: Display this help message\n";
    std::cout << "  q/ESC: Quit the application\n";
    std::cout << "  s: Scramble the cube (20 random moves)\n";
    std::cout << "\nSlice Rotation Controls:\n";
    std::cout << "  X-axis rotations (Front/Middle/Back):\n";
    std::cout << "    f/c: Front slice clockwise/counter-clockwise\n";
    std::cout << "    m/n: Middle X slice clockwise/counter-clockwise\n";
    std::cout << "    b/v: Back slice clockwise/counter-clockwise\n";
    std::cout << "\n  Y-axis rotations (Top/Middle/Bottom):\n";
    std::cout << "    t/y: Top slice clockwise/counter-clockwise\n";
    std::cout << "    g/j: Middle Y slice clockwise/counter-clockwise\n";
    std::cout << "    u/i: Bottom slice clockwise/counter-clockwise\n";
    std::cout << "\n  Z-axis rotations (Left/Middle/Right):\n";
    std::cout << "    l/k: Left slice clockwise/counter-clockwise\n";
    std::cout << "    o/p: Middle Z slice clockwise/counter-clockwise\n";
    std::cout << "    r/e: Right slice clockwise/counter-clockwise\n";
    std::cout << "===========================\n\n";
}

// Specify what to do when a keyboard event happens, i.e., when the user presses or releases a key
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
        
    switch (key) {
        case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
            exit(EXIT_SUCCESS);
            break;
            
        // Display help with H key
        case GLFW_KEY_H:
            displayHelp();
            break;
            
        // Scramble the cube with S key
        case GLFW_KEY_S:
            rotationIncrement = 10.0f;
            startScrambling(20);
            break;
            
        // X-axis rotations (Front/middle/back slices)
        case GLFW_KEY_F: // Front slice clockwise
            startSliceRotation(Xaxis, 0, 1);
            break;
        case GLFW_KEY_C: // Front slice counter-clockwise
            startSliceRotation(Xaxis, 0, -1);
            break;
        case GLFW_KEY_M: // Middle X slice clockwise
            startSliceRotation(Xaxis, 1, 1);
            break;
        case GLFW_KEY_N: // Middle X slice counter-clockwise
            startSliceRotation(Xaxis, 1, -1);
            break;
        case GLFW_KEY_B: // Back slice clockwise
            startSliceRotation(Xaxis, 2, 1);
            break;
        case GLFW_KEY_V: // Back slice counter-clockwise
            startSliceRotation(Xaxis, 2, -1);
            break;
            
        // Y-axis rotations (Top/middle/bottom slices)
        case GLFW_KEY_T: // Top slice clockwise
            startSliceRotation(Yaxis, 2, 1);
            break;
        case GLFW_KEY_Y: // Top slice counter-clockwise
            startSliceRotation(Yaxis, 2, -1);
            break;
        case GLFW_KEY_G: // Middle Y slice clockwise
            startSliceRotation(Yaxis, 1, 1);
            break;
        case GLFW_KEY_J: // Middle Y slice counter-clockwise
            startSliceRotation(Yaxis, 1, -1);
            break;
        case GLFW_KEY_U: // Bottom slice clockwise
            startSliceRotation(Yaxis, 0, 1);
            break;
        case GLFW_KEY_I: // Bottom slice counter-clockwise
            startSliceRotation(Yaxis, 0, -1);
            break;
            
        // Z-axis rotations (Left/middle/right slices)
        case GLFW_KEY_L: // Left slice clockwise
            startSliceRotation(Zaxis, 0, 1);
            break;
        case GLFW_KEY_K: // Left slice counter-clockwise
            startSliceRotation(Zaxis, 0, -1);
            break;
        case GLFW_KEY_O: // Middle Z slice clockwise
            startSliceRotation(Zaxis, 1, 1);
            break;
        case GLFW_KEY_P: // Middle Z slice counter-clockwise
            startSliceRotation(Zaxis, 1, -1);
            break;
        case GLFW_KEY_R: // Right slice clockwise
            startSliceRotation(Zaxis, 2, 1);
            break;
        case GLFW_KEY_E: // Right slice counter-clockwise
            startSliceRotation(Zaxis, 2, -1);
            break;
    }
}

// Specify what to do when a mouse event happens
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            leftMousePressed = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            leftMousePressed = false;
        }
    }
}

// Callback for cursor position changes
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (leftMousePressed) {
        // Calculate the change in cursor position
        double dx = xpos - lastX;
        double dy = ypos - lastY;
        
        // Update rotation angles based on mouse movement
        Theta[Yaxis] += dx * 0.5;  // Horizontal movement rotates around Y-axis
        Theta[Xaxis] += dy * 0.5;  // Vertical movement rotates around X-axis
        
        // Keep angles in the range [0, 360]
        Theta[Xaxis] = fmod(Theta[Xaxis], 360.0);
        Theta[Yaxis] = fmod(Theta[Yaxis], 360.0);
        
        // Update last position
        lastX = xpos;
        lastY = ypos;
    }
}

//---------------------------------------------------------------------
//
// main
//

int main()
{
    // Seed random number generator
    srand(static_cast<unsigned int>(time(nullptr)));
    
    if (!glfwInit())
            exit(EXIT_FAILURE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(512, 512, "Spin Cube", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    //Specify which events to recognize and the callback functions to handle them
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    
    init();

    double frameRate = 120, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1/frameRate){
            previousTime = currentTime;
            update();
        }
        
        display();
        glfwSwapBuffers(window);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}




