/**
 * Single file OpenGL ES start example minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ gles_triangle_rotate.cpp -o gles_triangle_rotate -lglfw -lGLESv2
 *
 * Run:
 * $ ./gles_triangle_rotate
 *
 * Dependencies:
 *  * C++11
 *  * GLFW 3.0+
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

in vec2 aPos;
out vec3 fragColor;

uniform mat4 transform;

void main() {
    gl_Position = transform * vec4(aPos, 0.0, 1.0);
}
)";

const char* fragment_src = R"(#version 310 es
precision highp float;

out vec4 outColor;

uniform vec3 uColor;

void main() {
    outColor = vec4(uColor, 1.0f);
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

    // 10. Specify the vertices.
    const float vertices[] = {
        -0.5, 0.5,
        0.5, 0.5,
        0.0, -0.5
    };
    {
        // 10.1. Query the "aPos" vertex input attribute's position.
        int aPosLoc = glGetAttribLocation(shader_program, "aPos");

        // 10.2. Specify the vertex data for the "aPos": vec2.
        glVertexAttribPointer(aPosLoc, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), vertices);

        // 10.3. Enable the usage of the vertex input array.
        glEnableVertexAttribArray(aPosLoc);
    }

    // 11. Query the uniform location
    int uniformColorLoc;
    {
        uniformColorLoc = glGetUniformLocation(shader_program, "uColor");
    }

    // 12. Calculate transformation matrix
    {
        glm::mat4 identity = glm::mat4(1.0f);
        glm::mat4 transform = glm::rotate(identity, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

        int transformLoc = glGetUniformLocation(shader_program, "transform");

        glUseProgram(shader_program);
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
        glUseProgram(0);
    }

    static float color = 0;
    // X. Create a render loop.
    while (!glfwWindowShouldClose(window))
    {
        // X. Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // X. Clear the color image.
        glClearColor(0.0, 0.3, 0.3, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // X. Use the shader program to draw.
        glUseProgram(shader_program);

        // XX. Update the uniform.
        color += 0.01;
        glUniform3f(uniformColorLoc, color, 0.1, 0.1);
        if (color > 1.0) { color = 0.0; }

        // X. Draw the triangles.
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // X. Swap the fron-back buffers to display the rendered image in the window.
        glfwSwapBuffers(window);
    }

    // XX. Destroy the window.
    glfwDestroyWindow(window);
    // XX. Destroy the GLFW.
    glfwTerminate();

    return 0;
}

