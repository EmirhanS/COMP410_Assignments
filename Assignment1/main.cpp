//
//  Display a bouncing circle
//

#include "Angel.h"
#include <vector>

const int NumSegments = 100; // Number of segments to approximate the circle
const int NumVertices = NumSegments + 2; // Center + segments + 1 (to close the circle)

vec4 points[NumVertices];
vec4 colors[NumVertices];

// Physics parameters
vec2 position(-0.9, 0.9);   // Initial position at top left
vec2 velocity(0.005, 0.0);   // Initial velocity (only x-axis)
float radius = 0.03;         // Radius of the circle
float gravity = 0.000981;      // Gravity constant
float restitution = 0.8;    // Coefficient of restitution (bounce factor)
const float ground = -1.0 + radius; // Ground level (bottom of window + radius)

// Initial values for reset
const vec2 initialPosition(-0.9, 0.9);
const vec2 initialVelocity(0.005, 0.0);

// Toggle for filled/unfilled shape
bool isFilledShape = true;

// Toggle for circle/square
bool isCircle = true;

// Toggle for color (0 = red, 1 = blue)
bool isRedColor = true;

// Trajectory settings
bool showTrajectory = true;
std::vector<vec2> trajectoryPoints;
const int maxTrajectoryPoints = 1000;
float trajectoryInterval = 0.05;  // Time between trajectory points
float trajectoryTimer = 0.0;      // Timer for recording trajectory

// Color definitions
const vec4 redColor = vec4(1.0, 0.0, 0.0, 1.0);
const vec4 blueColor = vec4(0.0, 0.0, 1.0, 1.0);

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;
GLuint program;  // Shader program

//----------------------------------------------------------------------------

// Update colors based on current color selection
void update_color()
{
    vec4 currentColor = isRedColor ? redColor : blueColor;
    
    // Update all colors in the array
    for (int i = 0; i < NumVertices; i++) {
        colors[i] = currentColor;
    }
    
    // Update buffer data (only color portion)
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
}

// Initialize the circle vertices
void init_circle()
{
    // Center of the circle
    points[0] = vec4(0.0, 0.0, 0.0, 1.0);
    colors[0] = isRedColor ? redColor : blueColor;
    
    // Generate points around the circle
    for (int i = 0; i <= NumSegments; i++) {
        float theta = 2.0f * M_PI * i / NumSegments;
        points[i + 1] = vec4(cos(theta) * radius, sin(theta) * radius, 0.0, 1.0);
        
        // All vertices have the same color
        colors[i + 1] = isRedColor ? redColor : blueColor;
    }
}

// Initialize the square vertices
void init_square()
{
    float side = radius;
    vec4 currentColor = isRedColor ? redColor : blueColor;
    
    for (int i = 0; i < 6; i++){
        colors[i] = currentColor;
    }
    
    points[0] = vec4(-side, side, 0.0, 1.0);// Top-left
    points[1] = vec4(side, side, 0.0, 1.0);     // Top-right
    points[2] = vec4(side, -side, 0.0, 1.0);   // Bottom-right
    points[3] = vec4(side, -side, 0.0, 1.0);   // Bottom-right
    points[4] = vec4(-side, -side, 0.0, 1.0);  // Bottom-left
    points[5] = vec4(-side, side, 0.0, 1.0);// Top-left
}

// Update the vertices based on the current shape
void update_shape()
{
    if (isCircle) {
        init_circle();
    } else {
        init_square();
    }
    
    // Update buffer data
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
}

// Clear the trajectory points
void clear_trajectory()
{
    trajectoryPoints.clear();
}

//----------------------------------------------------------------------------

void init()
{
    // Initialize shape vertices
    if (isCircle) {
        init_circle();
    } else {
        init_square();
    }
    
    // Load shaders and use the resulting shader program
    program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);
    
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
    
    // Set up vertex arrays
    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)));
    
    // Get uniform variable locations
    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");
    
    // Set projection matrix
    mat4 projection = Ortho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
    
    glClearColor(1.0, 1.0, 1.0, 1.0);  // White background
}

//----------------------------------------------------------------------------

// Reset the circle to the top left corner
void resetPosition()
{
    position = initialPosition;
    velocity = initialVelocity;
    clear_trajectory();
}

// Update the circle position for bouncing
void update()
{
    // Apply gravity to vertical velocity
    velocity.y -= gravity;
    
    // Update position based on velocity
    position += velocity;
    
    // Add trajectory point at regular intervals
    trajectoryTimer += 1.0/120.0;  // Assume 120fps
    if (trajectoryTimer >= trajectoryInterval) {
        trajectoryTimer = 0;
            trajectoryPoints.push_back(position);
            // Limit the number of trajectory points
            if (trajectoryPoints.size() > maxTrajectoryPoints) {
                trajectoryPoints.erase(trajectoryPoints.begin());
            }
    }
    
    // Check for collisions with window boundaries
    if (position.x + radius > 1.0) {
        // When circle reaches right wall, reset to top left
        resetPosition();
    }
    else if (position.x - radius < -1.0) {
        position.x = -1.0 + radius;
        velocity.x = -velocity.x * 0.90; // Slight horizontal energy loss
    }
    
    // Ground collision (realistic bouncing)
    if (position.y < ground) {
        position.y = ground; // Position correction
        
        // Apply bounce with restitution coefficient (energy loss)
        if (fabs(velocity.y) > 0.005) { // Only bounce if velocity is significant
            velocity.y = -velocity.y * restitution;
        } else {
            // Stop very small bounces
            velocity.y = 0;
        }
        
        // Apply slight friction when on ground
        if (fabs(velocity.y) < 0.005) {
            velocity.x *= 0.9;
        }
    }
    
    // Top collision
    if (position.y + radius > 1.0) {
        position.y = 1.0 - radius;
        velocity.y = -velocity.y * restitution;
    }
}

//----------------------------------------------------------------------------

// Draw trajectory points as dots
void draw_trajectory()
{
    if (!showTrajectory || trajectoryPoints.empty()) {
        return;
    }
    
    // Prepare to draw small circles for trajectory points
    glUseProgram(program);
    
    // Disable depth test (if enabled)
    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    if (depthTestEnabled) glDisable(GL_DEPTH_TEST);
    
    // Current color for trajectory
    vec4 currentColor = isRedColor ? redColor : blueColor;
        
    // Draw each trajectory point as a small shape
    for (int i = 0; i < trajectoryPoints.size(); i++) {
        // Set model view matrix for this trajectory point
        mat4 model_view = Translate(trajectoryPoints[i].x, trajectoryPoints[i].y, 0.0);
                
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
        
        if (isCircle) {
            // Draw small circles for each trajectory point
            for (int j = 0; j < NumVertices; j++) {
                colors[j] = currentColor;
            }
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
            // Draw the circle as a triangle fan (filled) or line loop (unfilled)
            if (isFilledShape) {
                glDrawArrays(GL_TRIANGLE_FAN, 0, NumVertices);
            } else {
                // Skip the center point (index 0) when drawing the outline
                glDrawArrays(GL_LINE_LOOP, 1, NumSegments + 1);
            }
        } else {
            // Draw small squares for each trajectory point
            for (int j = 0; j < 6; j++) {
                colors[j] = currentColor;
            }
            glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
            // Draw the square as triangles (filled) or line loop (unfilled)
            if (isFilledShape) {
                glDrawArrays(GL_TRIANGLES, 0, 6);
            } else {
                // Draw the square outline with just the 4 corners
                glDrawArrays(GL_LINE_LOOP, 0, 6);
            }
        }
    }
    
    // Restore depth test if it was enabled
//    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Draw trajectory first
    draw_trajectory();
    
    // Update buffer with shape data again (as draw_trajectory changes it)
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);
    
    // Apply translation based on current position
    mat4 model_view = Translate(position.x, position.y, 0.0);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    
    if (isCircle) {
        // Draw the circle as a triangle fan (filled) or line loop (unfilled)
        if (isFilledShape) {
            glDrawArrays(GL_TRIANGLE_FAN, 0, NumVertices);
        } else {
            // Skip the center point (index 0) when drawing the outline
            glDrawArrays(GL_LINE_LOOP, 1, NumSegments + 1);
        }
    } else {
        // Draw the square as triangles (filled) or line loop (unfilled)
        if (isFilledShape) {
            glDrawArrays(GL_TRIANGLES, 0, 6);
        } else {
            // Draw the square outline with just the 4 corners
            glDrawArrays(GL_LINE_LOOP, 0, 6);
        }
    }
    
    glFinish();
}

//----------------------------------------------------------------------------

// Print help information about available controls
void print_help() {
    printf("\n--- Bouncing Shape Controls ---\n");
    printf("Q: Quit the application\n");
    printf("I: Reset position to top left\n");
    printf("C: Toggle color (red/blue)\n");
    printf("T: Toggle trajectory display on/off\n");
    printf("Left Mouse Button: Toggle filled/outline shape\n");
    printf("Right Mouse Button: Toggle between circle/square\n");
    printf("H: Display this help message\n");
    printf("-----------------------------\n\n");
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_Q:
                exit(EXIT_SUCCESS);
                break;
            case GLFW_KEY_I:
                resetPosition();
                break;
            case GLFW_KEY_C:
                // Toggle between red and blue color
                isRedColor = !isRedColor;
                update_color();
                break;
            case GLFW_KEY_T:
                // Toggle trajectory display
                showTrajectory = !showTrajectory;
                break;
            case GLFW_KEY_H:
                // Display help information
                print_help();
                break;
        }
    }
}

// Mouse button callback function
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // Toggle between filled and unfilled shape
        isFilledShape = !isFilledShape;
    }
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        // Toggle between circle and square
        isCircle = !isCircle;
        update_shape();
    }
}

//----------------------------------------------------------------------------

int main()
{
    if (!glfwInit())
        exit(EXIT_FAILURE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(512, 512, "Bouncing Circle", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }
    
    // Set callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    
    init();
    
    // Print help information at startup
    printf("Press 'H' for help with controls\n");
    
    double frameRate = 120, currentTime, previousTime = 0.0;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1/frameRate) {
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
