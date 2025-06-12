// Gouraud (per vertex) shading vs Phong (per fragment) shading of a sphere model

//  Light and material properties are sent to the shader as uniform
//    variables.  Vertex positions and normals are sent as vertex attributes
//

#include "Angel.h"
#include <fstream>
#include <iostream>

using namespace std;

const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumVertices         = 3 * NumTriangles;

// Texture-related variables
GLuint textures[2];  // Array to hold texture objects
int currentTexture = 0;  // Index of current texture
vec2 texCoords[NumVertices];  // Texture coordinates for vertices

// Image-related variables
int imageWidth, imageHeight;
unsigned char* imageData;

typedef vec4 point4;
typedef vec4 color4;

point4 points[NumVertices];
vec3   normals[NumVertices];

enum {Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3};
GLfloat Theta[NumAxes] = {0.0, 0.0, 0.0};

// Model-view and projection matrices uniform location
GLuint  ModelView, Projection, Lighting;

GLfloat scaleFactor = 0.3;
bool fixedLight = true; // true = fixed light, false = moving light
int textureFlag = 0; // 0 = no texture, 1 = texture, 2 = wireframe
int textureIndex = 0; // 0 = basketball, 1 = earth
int materialIndex = 0; // 0 = plastic, 1 = metallic

// Bouncing animation variables
float sphereY = 2.0f / scaleFactor;
float sphereX = -2.0f / scaleFactor;
float velocityY = 0.0f;
float speed = 1.5f;
float velocityX = 2.0f * speed;
float gravity = -9.81f * speed;      // Gravity constant
float groundY = -1.0f;       // Ground level
float bounceEnergy = 0.7f;   // Energy retention after bounce (0.7 = 70% energy kept)
float lastTime = 0.0f;       // Last frame time

float tempVelocityX, tempVelocityY;
bool paused = false;
bool selfRotate = false;

// Add shader program variables
GLuint gouraudProgram, phongProgram;
bool usePhongShader = false; // false = gouraud, true = phong
int mode = 0;

point4 light_position;
color4 light_ambient;
color4 light_diffuse;
color4 light_specular;

color4 material_ambient;
color4 material_diffuse;
color4 material_specular;
float material_shininess;

color4 ambient_product;
color4 diffuse_product;
color4 specular_product;

//----------------------------------------------------------------------------

int Index = 0;

// Function to calculate texture coordinates for a point on sphere
vec2 calculateTexCoords(const point4& p) {
    float s = 0.5 + atan2(p.x, p.z) / (2.0 * M_PI);
    float t = 0.5 - asin(p.y) / M_PI;
    return vec2(s, t);
}

void
triangle( const point4& a, const point4& b, const point4& c )
{
    //normal vector is computed per vertex
    
    vec3 norm = normalize(vec3 (a.x,a.y,a.z));
    normals[Index] = vec3(norm.x, norm.y, norm.z);  points[Index] = a;  texCoords[Index] = calculateTexCoords(a);  Index++;
    norm = normalize(vec3 (b.x,b.y,b.z));
    normals[Index] = vec3(norm.x, norm.y, norm.z);   points[Index] = b;  texCoords[Index] = calculateTexCoords(b);  Index++;
    norm = normalize(vec3 (c.x,c.y,c.z));
    normals[Index] = vec3(norm.x, norm.y, norm.z);   points[Index] = c;  texCoords[Index] = calculateTexCoords(c);  Index++;
}
//----------------------------------------------------------------------------

point4
unit( const point4& p )
{
    float len = p.x*p.x + p.y*p.y + p.z*p.z;
    
    point4 t;
    if ( len > DivideByZeroTolerance ) {
        t = p / sqrt(len);
        t.w = 1.0;
    }
    
    return t;
}

void
divide_triangle( const point4& a, const point4& b,
                const point4& c, int count )
{
    if ( count > 0 ) {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle(  a, v1, v2, count - 1 );
        divide_triangle(  c, v2, v3, count - 1 );
        divide_triangle(  b, v3, v1, count - 1 );
        divide_triangle( v1, v3, v2, count - 1 );
    }
    else {
        triangle( a, b, c );
    }
}

void
tetrahedron( int count )
{
    point4 v[4] = {
        vec4( 0.0, 0.0, 1.0, 1.0 ),
        vec4( 0.0, 0.942809, -0.333333, 1.0 ),
        vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
        vec4( 0.816497, -0.471405, -0.333333, 1.0 )
    };
    
    divide_triangle( v[0], v[1], v[2], count );
    divide_triangle( v[3], v[2], v[1], count );
    divide_triangle( v[0], v[3], v[1], count );
    divide_triangle( v[0], v[2], v[3], count );
}

//----------------------------------------------------------------------------
// Function to skip comments in PPM file
void ignoreComments(std::ifstream& file) {
    char c;
    while (file.get(c)) {
        if (c == '#') {
            while (file.get(c) && c != '\n');
        } else {
            file.unget();
            break;
        }
    }
}

// Function to read PPM image
bool readPPMImage(const std::string& filename) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << std::endl;
        return false;
    }
    std::cout << "Successfully opened file: " << filename << std::endl;

    // Read magic number
    std::string magic;
    ignoreComments(file);
    file >> magic;
    
    // Read dimensions
    ignoreComments(file);
    file >> imageWidth;
    
    ignoreComments(file);
    file >> imageHeight;
    
    ignoreComments(file);
    int maxValue;
    file >> maxValue;
    
    // Calculate data size
    int dataSize = imageWidth * imageHeight * 3;

    // Allocate memory for image data
    imageData = new unsigned char[dataSize];

    int value;
    for (int i = 0; i < dataSize; i++) {
        file >> value;
        if (file.fail()) {
            std::cout << "Error reading ASCII data at position " << i << std::endl;
            delete[] imageData;
            file.close();
            return false;
        }
        imageData[i] = static_cast<unsigned char>(value);
    }

    
    if (file.fail()) {
        std::cout << "Error reading image data" << std::endl;
        std::cout << "Bytes read: " << file.gcount() << std::endl;
        delete[] imageData;
        file.close();
        return false;
    }

    std::cout << "Successfully read image data" << std::endl;
    file.close();
    return true;
}

// Function to setup texture
void setupTexture() {
    glGenTextures(2, textures);  // Generate 2 texture IDs

    glBindTexture(GL_TEXTURE_2D, textures[0]);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, 
                 GL_RGB, GL_UNSIGNED_BYTE, imageData);

    glGenerateMipmap(GL_TEXTURE_2D);

    // Setup first texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Free the image data after uploading to GPU
    delete[] imageData;

    // Setup second texture (you'll need to load another PPM file here)
    // For now, we'll just set up the parameters
    glBindTexture(GL_TEXTURE_2D, textures[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void plasticMaterial(){
    // Light properties for plastic
    light_ambient = color4( 0.6, 0.6, 0.6, 1.0 );    // Moderate ambient light
    light_diffuse = color4( 0.7, 0.7, 0.7, 1.0 );    // Softer diffuse light
    light_specular = color4( 0.3, 0.3, 0.3, 1.0 );   // Very soft specular light

    // Material properties for plastic
    material_ambient = color4( 0.1, 0.1, 0.1, 1.0 );      // Low ambient reflection
    material_diffuse = color4( 0.7, 0.7, 0.7, 1.0 );      // High diffuse reflection
    material_specular = color4( 0.3, 0.3, 0.3, 1.0 );     // Low specular reflection
    material_shininess = 10.0;                            // Low shininess

    ambient_product = light_ambient * material_ambient;
    diffuse_product = light_diffuse * material_diffuse;
    specular_product = light_specular * material_specular;
}

void metallicMaterial(){
    // Light properties for metallic
    light_ambient = color4( 0.9, 0.9, 0.9, 1.0 );    // Strong ambient light
    light_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );    // Full diffuse light
    light_specular = color4( 1.0, 1.0, 1.0, 1.0 );   // Full specular light

    // Material properties for metallic
    material_ambient = color4( 0.4, 0.4, 0.4, 1.0 );      // High ambient reflection
    material_diffuse = color4( 0.3, 0.3, 0.3, 1.0 );      // Low diffuse reflection
    material_specular = color4( 1.0, 1.0, 1.0, 1.0 );     // Full specular reflection
    material_shininess = 100.0;                           // Very high shininess

    ambient_product = light_ambient * material_ambient;
    diffuse_product = light_diffuse * material_diffuse;
    specular_product = light_specular * material_specular;
}

void setupMaterial(){
    // Set uniforms for both shader programs
    for (GLuint program : {gouraudProgram, phongProgram}) {
        glUseProgram(program);
        glUniform4fv( glGetUniformLocation(program, "AmbientProduct"), 1, ambient_product );
        glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"), 1, diffuse_product );
        glUniform4fv( glGetUniformLocation(program, "SpecularProduct"), 1, specular_product );
        glUniform4fv( glGetUniformLocation(program, "LightPosition"), 1, light_position );
        glUniform1f( glGetUniformLocation(program, "Shininess"), material_shininess );
        
        // Set TextureFlag uniform
        glUniform1i(glGetUniformLocation(program, "TextureFlag"), 0); // Initially set to 0 (no texture)
        
        // Get transformation uniform locations
        ModelView = glGetUniformLocation( program, "ModelView" );
        Projection = glGetUniformLocation( program, "Projection" );
        
        mat4 projection = Ortho( -2.0, 2.0, -2.0, 2.0, -2.0, 2.0 );
        glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
    }
}


// OpenGL initialization
void
init()
{
    // Subdivide a tetrahedron into a sphere
    tetrahedron( NumTimesToSubdivide );
    
    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );
    
    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals) + sizeof(texCoords), NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points), sizeof(normals), normals );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals), sizeof(texCoords), texCoords );
    
    // Load both shader programs
    gouraudProgram = InitShader( "vshader.glsl", "fshader.glsl" );
    phongProgram = InitShader( "vshader_phong.glsl", "fshader_phong.glsl" );
    
    // Initially use Gouraud shader
    glUseProgram(gouraudProgram);
    
    // set up vertex arrays for both programs
    GLuint vPosition = glGetAttribLocation( gouraudProgram, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    
    GLuint vNormal = glGetAttribLocation( gouraudProgram, "vNormal" ); 
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)) );
    
    GLuint vTexCoord = glGetAttribLocation( gouraudProgram, "vTexCoord" ); 
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points) + sizeof(normals)) );

    // Set up vertex arrays for phongProgram
    glUseProgram(phongProgram);
    GLuint vPosition_phong = glGetAttribLocation( phongProgram, "vPosition" );
    glEnableVertexAttribArray( vPosition_phong );
    glVertexAttribPointer( vPosition_phong, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

    GLuint vNormal_phong = glGetAttribLocation( phongProgram, "vNormal" );
    glEnableVertexAttribArray( vNormal_phong );
    glVertexAttribPointer( vNormal_phong, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points)) );

    GLuint vTexCoord_phong = glGetAttribLocation( phongProgram, "vTexCoord" );
    glEnableVertexAttribArray( vTexCoord_phong );
    glVertexAttribPointer( vTexCoord_phong, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points) + sizeof(normals)) );

    // Switch back to Gouraud shader
    glUseProgram(gouraudProgram);
    
    
    plasticMaterial();
    
    setupMaterial();
    
    glEnable( GL_DEPTH_TEST );
    glEnable(GL_CULL_FACE); //to discard invisible faces from rendering
    
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); /* white background */
    
    // Read and setup first texture
    if (readPPMImage("basketball.ppm")) {
        setupTexture();
    }
    
    // Read and setup second texture
    if (readPPMImage("earth.ppm")) {  // You'll need to provide a second PPM file
        glBindTexture(GL_TEXTURE_2D, textures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageWidth, imageHeight, 0, 
                     GL_RGB, GL_UNSIGNED_BYTE, imageData);
        delete[] imageData;
    }
    
    glBindTexture(GL_TEXTURE_2D, textures[0]);
}

//----------------------------------------------------------------------------

void resetPosition() {
    sphereY = 2.0f / scaleFactor;
    sphereX = -2.0f / scaleFactor;
    velocityY = 0.0f;
    velocityX = 2.0f * speed;
}

void
display( void )
{
    // Calculate delta time
    float currentTime = glfwGetTime();
    float deltaTime = currentTime - lastTime;
    lastTime = currentTime;

    // Update bouncing physics
    velocityY += gravity * deltaTime * speed;
    sphereY += velocityY * deltaTime;
    sphereX += velocityX * deltaTime;
    
    // Get scaled boundaries
    float scaledGroundY = groundY / scaleFactor;
    float scaledRightEdge = 2.0f / scaleFactor;
    
    // Check for ground collision and bounce
    if (sphereY <= scaledGroundY) {
        sphereY = scaledGroundY;
        velocityY = -velocityY * bounceEnergy; // Reverse and reduce velocity
    }

    // Check for horizontal boundaries
    if (sphereX >= scaledRightEdge) {
        resetPosition();
    }

    if (selfRotate){
        Theta[Yaxis] += 1.0;
        if (Theta[Yaxis] > 360.0) {
            Theta[Yaxis] -= 360.0;
        }
    }

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    mat4 model_view = Scale(scaleFactor, scaleFactor, scaleFactor) * 
                     Translate(sphereX, sphereY, 0.0) *
                     (RotateX( Theta[Xaxis] ) *
                      RotateY( Theta[Yaxis] ) *
                      RotateZ( Theta[Zaxis] ) );
    
    glUniformMatrix4fv( ModelView, 1, GL_TRUE, model_view );

    // Update light position based on mode
    point4 light_position;
    if (fixedLight) {
        light_position = point4(0.0, 0.0, 2.0, 1.0); // Fixed position
    } else {
        light_position = point4(sphereX * scaleFactor, sphereY * scaleFactor, 2.0, 1.0); // Moves with sphere
    }
    
    // Update light position for both shader programs
    for (GLuint program : {gouraudProgram, phongProgram}) {
        glUseProgram(program);
        glUniform4fv(glGetUniformLocation(program, "LightPosition"), 1, light_position);
    }
    
    // --- Bind Current Texture ---
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures[currentTexture]);
    GLuint prog = usePhongShader ? phongProgram : gouraudProgram;
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "tex"), textureIndex);
    glUniform1i(glGetUniformLocation(prog, "TextureFlag"), textureFlag);
    
    if (textureFlag == 2) {
        // Wireframe mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        // Solid mode
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glDrawArrays( GL_TRIANGLES, 0, NumVertices );
    glFlush();
}

//----------------------------------------------------------------------------

void printHelp() {
    std::cout << "\n=== Input Controls ===\n"
              << "ESC/Q: Exit program\n"
              << "R: Reset sphere position\n"
              << "S: Toggle between Gouraud and Phong shading\n"
              << "O: Change shading mode\n"
              << "M: Toggle between plastic and metallic materials\n"
              << "Z: Zoom in\n"
              << "W: Zoom out\n"
              << "L: Toggle between fixed and moving light\n"
              << "Arrow Keys: Rotate sphere\n"
              << "I: Toggle between basketball and earth textures\n"
              << "T: Toggle texture display mode (no texture/texture/wireframe)\n"
              << "SPACE: Pause/resume animation\n"
              << "K: Toggle self-rotation\n"
              << "H: Show this help message\n"
              << "===================\n" << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch( key ) {
        case GLFW_KEY_H:
            if (action == GLFW_PRESS) {
                printHelp();
            }
            break;
        case GLFW_KEY_ESCAPE: case GLFW_KEY_Q:
            exit( EXIT_SUCCESS );
            break;
        case GLFW_KEY_R:
            if (action == GLFW_PRESS) {
                resetPosition();
            }
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS) {
                usePhongShader = !usePhongShader;
                glUseProgram(usePhongShader ? phongProgram : gouraudProgram);
            }
            break;
        case GLFW_KEY_O:
            if (action == GLFW_PRESS) {
                mode = (mode + 1) % 3;
                glUniform1i(glGetUniformLocation(gouraudProgram, "mode"), mode);
            }
            break;

        case GLFW_KEY_M:
            if (action == GLFW_PRESS) {
                materialIndex = (materialIndex + 1) % 2;
                if (materialIndex == 0){
                    plasticMaterial();
                }
                else{
                    metallicMaterial();
                }
                setupMaterial();
            }
            break;
        // "Zoom-in" to the object
        case GLFW_KEY_Z:
            scaleFactor *= 1.1;
            break;
        // "Zoom-out" from the object
        case GLFW_KEY_W:
            scaleFactor *= 0.9;
            break;
        case GLFW_KEY_L:
            if (action == GLFW_PRESS) {
                fixedLight = !fixedLight; // Toggle between fixed and moving light
            }
            break;
        
        case GLFW_KEY_UP:
            Theta[Xaxis] += 3.0;
            
            if (Theta[Xaxis] > 360.0){
                Theta[Xaxis] -= 360.0;
            }
            
        case GLFW_KEY_DOWN:
            Theta[Xaxis] -= 3.0;
            
            if (Theta[Xaxis] < -360.0){
                Theta[Xaxis] += 360.0;
            }
            break;

            
        case GLFW_KEY_LEFT:
            Theta[Yaxis] += 3.0;
            
            if (Theta[Yaxis] > 360.0){
                Theta[Yaxis] -= 360.0;
            }
            break;
            
        case GLFW_KEY_RIGHT:
            Theta[Yaxis] -= 3.0;
            
            if (Theta[Yaxis] < -360.0){
                Theta[Yaxis] += 360.0;
            }
            break;
            
        case GLFW_KEY_I:
            if (action == GLFW_PRESS) {
                currentTexture = (currentTexture + 1) % 2;
            }
            break;
        case GLFW_KEY_T:
            if (action == GLFW_PRESS) {
                textureFlag = (textureFlag + 1) % 3;  // Now cycles through 3 states
            }
            break;

        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) {
                paused = !paused;
                if (paused) {
                    tempVelocityX = velocityX;
                    tempVelocityY = velocityY;
                    gravity = 0.0f;
                    velocityX = 0.0f;
                    velocityY = 0.0f;
                } else {
                    velocityX = tempVelocityX;
                    velocityY = tempVelocityY;
                    gravity = -9.81f * speed;
                }
            }
            break;
        
        case GLFW_KEY_K:
            if (action == GLFW_PRESS) {
                selfRotate = !selfRotate;
            }
            break;
        
        default:
            break;
    }
    
}

//----------------------------------------------------------------------------

void framebuffer_size_callback(GLFWwindow* window, int width, int height)

{
    glViewport( 0, 0, width, height );
    
    GLfloat left = -2.0, right = 2.0;
    GLfloat top = 2.0, bottom = -2.0;
    GLfloat zNear = -20.0, zFar = 20.0;
    
    GLfloat aspect = GLfloat(width)/height;
    
    if ( aspect > 1.0 ) {
        left *= aspect;
        right *= aspect;
    }
    else {
        top /= aspect;
        bottom /= aspect;
    }
    
    mat4 projection = Ortho( left, right, bottom, top, zNear, zFar );
    glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
}

//----------------------------------------------------------------------------

int
main( int argc, char **argv )
{
    
    if (!glfwInit())
            exit(EXIT_FAILURE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(1024, 1024, "Shading", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!window)
        {
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    init();
    

    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
