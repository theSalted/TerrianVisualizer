﻿/*
  CSCI 420 Computer Graphics, Computer Science, USC
  Assignment 1: Height Fields with Shaders.
  C/C++ starter code

  Student username: <type your USC username here>
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"

#include <iostream>
#include <cstring>
#include <memory>
#include "main.h"

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0;   // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0;  // 1 if pressed, 0 if not

typedef enum
{
    DEFAULT,
    CTRL,
    SHIFT
} CONTROL_STATE;
CONTROL_STATE controlState = CTRL;

// Transformations of the terrain.
float terrainRotate[3] = {90.0f, 0.0f, 0.0f};
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = {0.0f, 0.0f, 0.0f};
float terrainScale[3] = {1.0f, 1.0f, 1.0f};

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "Terrian Visualizer";

// Mode of the application
// 0 (mode 1): vertext; 1 (mode 2) line; 2 (mode 3) triangles; 3 (mode 4) smoothed
int mode = 0;

// Number of vertices in the single triangle (starter code).
int numVertices;
int numLineVertices;

// Helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;
VBO vboVertices;
VBO vboColors;
VAO vao;
// Add these at the top with other global variables
VBO vboLineVertices;
VBO vboLineColors;
VAO vaoLines;



// Write a screenshot to the specified filename.
void saveScreenshot(const char *filename)
{
    std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else
        cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
    // Do some stuff...
    // For example, here, you can save the screenshots to disk (to make the animation).

    // Notify GLUT that it should call displayFunc.
    glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
    glViewport(0, 0, w, h);

    // When the window has been resized, we need to re-set our projection matrix.
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.LoadIdentity();
    // You need to be careful about setting the zNear and zFar.
    // Anything closer than zNear, or further than zFar, will be culled.
    const float zNear = 0.1f;
    const float zFar = 10000.0f;
    const float humanFieldOfView = 60.0f;
    matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
    // Mouse has moved, and one of the mouse buttons is pressed (dragging).

    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};
    float average = (mousePosDelta[0] - mousePosDelta[1]) * 0.5f;
    switch (controlState)
    {
    // rotate the terrain
    case DEFAULT:
        if (leftMouseButton)
        {
            // control x,y translation via the left mouse button
            terrainTranslate[0] += mousePosDelta[0] * 0.01f;
            terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
        }
        if (middleMouseButton)
        {
            terrainTranslate[2] += average * 0.01f;
        }
        if (rightMouseButton)
        {
            // control x,y rotation via the left mouse button
            terrainRotate[0] += mousePosDelta[1];
            terrainRotate[1] += mousePosDelta[0];
        }
        break;

    // scale the terrain
    case SHIFT:
        if (leftMouseButton)
        {
            // control x scaling via the left mouse button
            terrainScale[0] *= 1.0f + average * 0.01f;
        }
        if (middleMouseButton)
        {
            // control y scaling via the left mouse button
            terrainScale[1] *= 1.0f - average * 0.01f;
        }
        if (rightMouseButton)
        {
            // control y scaling via the left mouse button
            terrainScale[2] *= 1.0f - average * 0.01f;
        }
        break;

    // translate the terrain
    case CTRL:
        if (leftMouseButton)
        {
            // control z rotation via the middle mouse button
            terrainRotate[2] += mousePosDelta[1];
        }
        // if (middleMouseButton)
        // {
        //   // control z translation via the middle mouse button
        //   terrainTranslate[2] += mousePosDelta[1] * 0.01f;
        // }
        break;
    }

    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
    // Mouse has moved.
    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
    // A mouse button has has been pressed or depressed.

    // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
    switch (button)
    {
    case GLUT_LEFT_BUTTON:
        leftMouseButton = (state == GLUT_DOWN);
        break;

    case GLUT_MIDDLE_BUTTON:
        middleMouseButton = (state == GLUT_DOWN);
        break;

    case GLUT_RIGHT_BUTTON:
        rightMouseButton = (state == GLUT_DOWN);
        break;
    }

    // Keep track of whether CTRL and SHIFT keys are pressed.
    switch (glutGetModifiers())
    {
    case GLUT_ACTIVE_CTRL:
        cout << "You pressed the GLUT_ACTIVE_CTRL." << endl;
        controlState = CTRL;
        break;

    case GLUT_ACTIVE_SHIFT:
        cout << "You pressed the GLUT_ACTIVE_SHIFT." << endl;
        controlState = SHIFT;
        break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
        controlState = DEFAULT;
        break;
    }

    // Store the new mouse position.
    mousePos[0] = x;
    mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: // ESC key
            exit(0); // exit the program
            break;

        case '1':
            cout << "Point mode activated." << endl;
            mode = 0;
            pipelineProgram.SetUniformVariablei("mode", mode);
            break;

        case '2':
            cout << "Line mode activated." << endl;
            mode = 1;
            pipelineProgram.SetUniformVariablei("mode", mode);
            break;

        case '3':
            cout << "You pressed the 3." << endl;
            mode = 2;
            pipelineProgram.SetUniformVariablei("mode", mode);
            break;

        case '4':
            cout << "You pressed the 4." << endl;
            mode = 3;
            pipelineProgram.SetUniformVariablei("mode", mode);
            break;

        case ' ':
            cout << "You pressed the spacebar." << endl;
            break;

        case 'x':
            // Take a screenshot.
            saveScreenshot("screenshot.jpg");
            break;

        case 'r':
            // Reset the terrain to its default position.
            terrainRotate[0] = 90.0f;
            terrainRotate[1] = 0.0f;
            terrainRotate[2] = 0.0f;
            terrainTranslate[0] = 0.0f;
            terrainTranslate[1] = 0.0f;
            terrainTranslate[2] = 0.0f;
            terrainScale[0] = 1.0f;
            terrainScale[1] = 1.0f;
            terrainScale[2] = 1.0f;
    }
}

void displayFunc()
{
    // This function performs the actual rendering.

    // First, clear the screen.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up the camera position, focus point, and the up vector.
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.LoadIdentity();
    matrix.LookAt(0.0, 0.0, 5.0,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0);

    // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
    // ...
    matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
    matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0);
    matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0);
    matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0);
    // float magnitude = std::sqrt(terrainScale[0] * terrainScale[0] + terrainScale[1] * terrainScale[1]);
    // matrix.Scale(magnitude, magnitude, terrainScale[2]);

    matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

    // Read the current modelview and projection matrices from our helper class.
    // The matrices are only read here; nothing is actually communicated to OpenGL yet.
    float modelViewMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::ModelView);
    matrix.GetMatrix(modelViewMatrix);

    float projectionMatrix[16];
    matrix.SetMatrixMode(OpenGLMatrix::Projection);
    matrix.GetMatrix(projectionMatrix);

    // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
    // Important: these matrices must be uploaded to *all* pipeline programs used.
    // In TV, there is only one pipeline program, but in future there will be several of them.
    // In such a case, you must separately upload to *each* pipeline program.
    // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
    pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
    pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

    // Execute the rendering.
    // Bind the VAO that we want to render. Remember, one object = one VAO.
    // Execute the rendering based on the current mode
    if (mode == 0) // Point mode
    {
        vao.Bind();
        glDrawArrays(GL_POINTS, 0, numVertices);
    }
    else if (mode == 1) // Line mode
    {
        vaoLines.Bind();
        glDrawArrays(GL_LINES, 0, numLineVertices);
    }
    // Swap the double-buffers.
    glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
    // Load the image from a jpeg disk file into main memory.
    std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
    if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
    {
        cout << "Error reading image " << argv[1] << "." << endl;
        exit(EXIT_FAILURE);
    }

    // Set the background color.
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

    // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
    glEnable(GL_DEPTH_TEST);

    // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
    // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
    // We only have one set of shaders, and therefore, there is only one pipeline program.
    // In future, we will need to shade different objects with different shaders, and therefore, we will have
    // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
    // Load and set up the pipeline program, including its shaders.
    if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
    {
        cout << "Failed to build the pipeline program." << endl;
        throw 1;
    }
    cout << "Successfully built the pipeline program." << endl;

    // Set up the uniform variable for mode.
    pipelineProgram.SetUniformVariablei("mode", mode);

    // Bind the pipeline program that we just created.
    // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
    // any object rendered from that point on, will use those shaders.
    // When the application starts, no pipeline program is bound, which means that rendering is not set up.
    // So, at some point (such as below), we need to bind a pipeline program.
    // From that point on, exactly one pipeline program is bound at any moment of time.
    pipelineProgram.Bind();

    // Prepare the triangle position and color data for the VBO.
    // Prepare the triangle position and color data for the VBO.
    // The code below sets up a single triangle (3 vertices).
    // The triangle will be rendered using GL_TRIANGLES (in displayFunc()).
    // Prepare the triangle position and color data for the VBO.
    // The code below sets up a single triangle (3 vertices).
    // The triangle will be rendered using GL_TRIANGLES (in displayFunc()).

    int width = heightmapImage->getWidth();
    int height = heightmapImage->getHeight();
    
    initPointMode(height, width, heightmapImage); // 4 values per color
    initLineMode(height, width, heightmapImage);

    

    // Check for any OpenGL errors.
    std::cout << "GL error status is: " << glGetError() << std::endl;
}

void initPointMode(int height, int width, std::__1::unique_ptr<ImageIO> &heightmapImage)
{
    int resolution = width * height;
    numVertices = resolution; // This must be a global variable, so that we know how many vertices to render in glDrawArrays.

    // // Vertex positions.
    std::unique_ptr<float[]> positions = std::make_unique<float[]>(numVertices * 3);
    // positions[0] = 0.0; positions[1] = 0.0; positions[2] = 0.0; // (x,y,z) coordinates of the first vertex
    // positions[3] = 0.0; positions[4] = 1.0; positions[5] = 0.0; // (x,y,z) coordinates of the second vertex
    // positions[6] = 1.0; positions[7] = 0.0; positions[8] = 0.0; // (x,y,z) coordinates of the third vertex

    // // Vertex colors.
    std::unique_ptr<float[]> colors = std::make_unique<float[]>(numVertices * 4);
    // colors[0] = 0.0; colors[1] = 0.0;  colors[2] = 1.0;  colors[3] = 1.0; // (r,g,b,a) channels of the first vertex
    // colors[4] = 1.0; colors[5] = 0.0;  colors[6] = 0.0;  colors[7] = 1.0; // (r,g,b,a) channels of the second vertex
    // colors[8] = 0.0; colors[9] = 1.0; colors[10] = 0.0; colors[11] = 1.0; // (r,g,b,a) channels of the third vertex

    int idx = 0;
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            float x = 1.0f * i / (width - 1);                     // Normalized x position
            float z = -1.0f * j / (height - 1);                   // Normalized z position (flip j to match OpenGL coordinate system)
            float y = heightmapImage->getPixel(i, j, 0) / 255.0f; // Normalized height value

            // Set vertex positions (x, y, z)
            positions[idx * 3 + 0] = x;
            positions[idx * 3 + 1] = y * 0.1;
            positions[idx * 3 + 2] = z;

            // Set vertex colors (r, g, b, a)
            colors[idx * 4 + 0] = 1.0f * y; // Red
            colors[idx * 4 + 1] = 1.0f * y; // Green
            colors[idx * 4 + 2] = 1.0f * y; // Blue
            colors[idx * 4 + 3] = 1.0f;     // Alpha

            ++idx;
        }
    }

    // Create the VBOs.
    // We make a separate VBO for vertices and colors.
    // This operation must be performed BEFORE we initialize any VAOs.
    vboVertices.Gen(numVertices, 3, positions.get(), GL_STATIC_DRAW); // 3 values per position
    vboColors.Gen(numVertices, 4, colors.get(), GL_STATIC_DRAW);

    // Create the VAOs. There is a single VAO in this example.
    // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
    // A VAO contains the geometry for a single object. There should be one VAO per object.
    // In this project, "geometry" means vertex positions and colors. In future 2, it will also include
    // vertex normal and vertex texture coordinates for texture mapping.
    vao.Gen();

    // Set up the relationship between the "position" shader variable and the VAO.
    // Important: any typo in the shader variable name will lead to malfunction.
    vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboVertices, "position");

    // Set up the relationship between the "color" shader variable and the VAO.
    // Important: any typo in the shader variable name will lead to malfunction.
    vao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboColors, "color");
}

void initLineMode(int height, int width, std::unique_ptr<ImageIO> &heightmapImage)
{
    numLineVertices = 2 * (((width - 1) * height) + (width * (height - 1)));
    std::unique_ptr<float[]> positions = std::make_unique<float[]>(numLineVertices * 3);
    std::unique_ptr<float[]> colors = std::make_unique<float[]>(numLineVertices * 4);

    int idx = 0;
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            float x = static_cast<float>(i) / (width - 1);
            float z = -static_cast<float>(j) / (height - 1);
            float y = heightmapImage->getPixel(i, j, 0) / 255.0f * 0.1f; // Scale the height value

            // Color based on height
            float color = y * 10.0f; // Adjust scaling as needed
            
            // Right neighbor
            if (i < width - 1) {
                float xRight = static_cast<float>(i + 1) / (width - 1);
                float yRight = heightmapImage->getPixel(i + 1, j, 0) / 255.0f * 0.1f;
                float zRight = z;

                // Positions for line segment
                positions[idx * 3 + 0] = x;
                positions[idx * 3 + 1] = y;
                positions[idx * 3 + 2] = z;
                positions[(idx + 1) * 3 + 0] = xRight;
                positions[(idx + 1) * 3 + 1] = yRight;
                positions[(idx + 1) * 3 + 2] = zRight;

                for (int k = 0; k < 2; ++k) {
                    colors[(idx + k) * 4 + 0] = color;
                    colors[(idx + k) * 4 + 1] = color;
                    colors[(idx + k) * 4 + 2] = color;
                    colors[(idx + k) * 4 + 3] = 1.0f;
                }

                idx += 2; // Increment idx only when data is added
            }

            // Bottom neighbor
            if (j < height - 1) {
                float xDown = x;
                float yDown = heightmapImage->getPixel(i, j + 1, 0) / 255.0f * 0.1f;
                float zDown = -static_cast<float>(j + 1) / (height - 1);

                positions[idx * 3 + 0] = x;
                positions[idx * 3 + 1] = y;
                positions[idx * 3 + 2] = z;
                positions[(idx + 1) * 3 + 0] = xDown;
                positions[(idx + 1) * 3 + 1] = yDown;
                positions[(idx + 1) * 3 + 2] = zDown;

                for (int k = 0; k < 2; ++k) {
                    colors[(idx + k) * 4 + 0] = color;
                    colors[(idx + k) * 4 + 1] = color;
                    colors[(idx + k) * 4 + 2] = color;
                    colors[(idx + k) * 4 + 3] = 1.0f;
                }

                idx += 2; // Increment idx only when data is added
            }
        }
    }

    // Generate VBOs
    vboLineVertices.Gen(numLineVertices, 3, positions.get(), GL_STATIC_DRAW);
    vboLineColors.Gen(numLineVertices, 4, colors.get(), GL_STATIC_DRAW);

    // Generate VAO
    vaoLines.Gen();
    vaoLines.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboLineVertices, "position");
    vaoLines.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &vboLineColors, "color"); 
}



int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "The arguments are incorrect." << endl;
        cout << "usage: ./TerrianVisualizer <heightmap file>" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Initializing GLUT..." << endl;
    glutInit(&argc, argv);

    cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);

    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

    // Tells GLUT to use a particular display function to redraw.
    glutDisplayFunc(displayFunc);
    // Perform animation inside idleFunc.
    glutIdleFunc(idleFunc);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);

// init glew
#ifdef __APPLE__
    // nothing is needed on Apple
#else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
        cout << "error: " << glewGetErrorString(result) << endl;
        exit(EXIT_FAILURE);
    }
#endif

    // Perform the initialization.
    initScene(argc, argv);

    // Sink forever into the GLUT loop.
    glutMainLoop();
}
