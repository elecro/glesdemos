/**
 * Single file OpenGL ES start example minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ gles_cube.cpp -o gles_cube -lglfw -lGLESv2
 *
 * Run:
 * $ ./gles_cube
 *
 * Dependencies:
 *  * C++11
 *  * GLFW 3.0+
 *  * GLM
 *  * Open GL ES 3.0+
 *  * EGL
 *
 * MIT License
 * Copyright (c) 2020 elecro
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * OFTWARE.
 */
#include <stdio.h>

#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* vertex_src = R"(#version 310 es
precision highp float;

in vec3 aPos;
out vec2 checkerCoord;

uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);

    // Move the position coordinate into the [0, 1] range.
    checkerCoord = (vec4(aPos, 1.0).xy + vec2(1.0f)) / vec2(2.0);
}
)";

const char* fragment_src = R"(#version 310 es
precision highp float;

in vec2 checkerCoord;

out vec4 outColor;

uniform vec3 uColor;

float checker(vec2 uv, float repeats)
{
  float cx = floor(repeats * uv.x);
  float cy = floor(repeats * uv.y);
  float result = mod(cx + cy, 2.0);
  return sign(result);
}

void main() {
    vec2 uv = checkerCoord.xy;
    float checkerColor = mix(0.8f, 0.6f, checker(uv, 10.0f));

    outColor = vec4(uColor, 1.0f);
    outColor.rgb *= checkerColor;
}
)";

static void ErrorCallbackGLFW(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

int main(int argc, char **argv) {
    // 0. Add a method to report any GLFW errors.
    glfwSetErrorCallback(ErrorCallbackGLFW);

    // 1. Initialize the GLFW library.
    if (!glfwInit()) {
        return -1;
    }

    // 2. Add hints to use during context creation when a GLFW window is created.
    // Here: require a GL ES 3.0 context.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);

    // 3. Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1024, 600, "GLDEMO", NULL, NULL);
    if (window == NULL) {
        return -2;
    }

    // 4. Activate the window (display it).
    glfwMakeContextCurrent(window);

    // 5. Set the view port to match the window size.
    {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
    }

    // 6. Create the vertex shader.
    unsigned int vertex_shader;
    {
        // 6.1. Create a vertex shader object.
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        // 6.2. Specify the shader source.
        glShaderSource(vertex_shader, 1, &vertex_src, NULL);

        // 6.3. Compile the shader.
        glCompileShader(vertex_shader);

        // 6.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(vertex_shader, 512, NULL, info);
            printf("Vertex shader error:\n%s\n", info);
            return -3;
        }
    }

    // 7. Create the fragment shader.
    unsigned int fragment_shader;
    {
        // 7.1. Create a fragment shader object.
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        // 7.2. Specify the shader source.
        glShaderSource(fragment_shader, 1, &fragment_src, NULL);

        // 7.3. Compile the shader.
        glCompileShader(fragment_shader);

        // 7.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(fragment_shader, 512, NULL, info);
            printf("Fragment shader error:\n%s\n", info);
            return -3;
        }
    }

    // 8. Create a shader program and attach the vertex/fragment shaders.
    unsigned int shader_program;
    {
        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);

        int success;
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if(!success) {
            char info[512];
            glGetProgramInfoLog(shader_program, 512, NULL, info);
            printf("Program error:\n%s\n", info);
            return -3;
        }
    }

    // 9. The vertex and fragment shaders can be removed after linking.
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    // V.1. Create a Vertex Buffer object for vertices data
    const float vertices[] = {
        // Cube coordinates. texture coords are not used for now.
        // stride will be: (3 + 2) * sizeof(float)
        // positions          // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };

    unsigned int vertices_vbo;
    {
        // V.1.1. Generate the buffer object.
        glGenBuffers(1, &vertices_vbo);

        // V.1.2. Bind the VBO to the "GL_ARRAY_BUFFER".
        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);

        // V.1.3. "Upload" the data for the active "GL_ARRAY_BUFFER".
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // V.1.4. Unbind the "GL_ARRAY_BUFFER".
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // V.1.2. Specify the Vertex Array Object.
    /* VAO is used to describe how the VBOs are accessed (layout/format). */
    unsigned int vao;
    {
        int aPosLoc = glGetAttribLocation(shader_program, "aPos");

        glGenVertexArrays(1, &vao);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo);

        glVertexAttribPointer(aPosLoc, 3, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), NULL);

        // 10.3. Enable the usage of the vertex input array.
        glEnableVertexAttribArray(aPosLoc);

        glBindVertexArray(0);
    }

    // 11. Query the uniform location
    int uniformColorLoc;
    {
        uniformColorLoc = glGetUniformLocation(shader_program, "uColor");
    }

    int projectionLoc = glGetUniformLocation(shader_program, "projection");
    {
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f);
        glUseProgram(shader_program);

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

        glUseProgram(0);
    }


    int modelLoc = glGetUniformLocation(shader_program, "model");
    int viewLoc  = glGetUniformLocation(shader_program, "view");

    glEnable(GL_DEPTH_TEST);
    static float color = 0;
    // X. Create a render loop.
    while (!glfwWindowShouldClose(window))
    {
        // X. Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // X. Clear the color image.
        glClearColor(0.0, 0.3, 0.3, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // X. Use the shader program to draw.
        glUseProgram(shader_program);

        // V.3. Use the VAO
        glBindVertexArray(vao);

        // XX. Update the transformation matrix.
        {
            glm::mat4 model         = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
            glm::mat4 view          = glm::mat4(1.0f);

            //model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
            view  = glm::translate(view, glm::vec3(.0f, 0.0f, -3.0f));
            // retrieve the matrix uniform locations
            // pass them to the shaders (3 different ways)
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
        }

        // X. Draw the triangles.
        glUniform3f(uniformColorLoc, 0.1, 0.8, 0.9);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Draw a bit of wireframe. It will be incomplete but it's ok for now.
        glUniform3f(uniformColorLoc, 0.0, 0.0, 0.0);
        glDrawArrays(GL_LINES, 0, 36);

        // X. Swap the fron-back buffers to display the rendered image in the window.
        glfwSwapBuffers(window);
    }

    // XX. Destroy the window.
    glfwDestroyWindow(window);
    // XX. Destroy the GLFW.
    glfwTerminate();

    return 0;
}

