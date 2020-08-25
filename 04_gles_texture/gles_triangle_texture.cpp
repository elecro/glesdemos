/**
 * Single file OpenGL ES start example minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ gles_triangle_texture.cpp -o gles_triangle_texture -lglfw -lGLESv2
 *
 * Run:
 * $ ./gles_texture
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
#include <libgen.h>
#include <stdio.h>

#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char* vertex_src = R"(#version 310 es
precision highp float;

in vec2 aPos;
in vec2 aTex;
out vec3 fragColor;
out vec2 fTex;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    fTex = aTex;
}
)";

const char* fragment_src = R"(#version 310 es
precision highp float;

in vec2 fTex;
out vec4 outColor;

uniform vec3 uColor;
uniform sampler2D image;

void main() {
    outColor = vec4(uColor, 1.0f) * texture(image, fTex /** vec2(2.0, 2.0)*/);
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
        0.0, 0.5,
        0.5, -0.5,
        -0.5, -0.5,
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

    // 12. Create texture
    unsigned int texture;
    {
        char path[1024];
        char *dir = dirname(argv[0]);
        strcpy(path, dir);
        strcat(path, "/kitten_10.jpg");

        int width, height, nrChannels;
        // To use OpenGL UV coord system:
        stbi_set_flip_vertically_on_load(true);
        unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
        printf("Image %s WxH: %dx%d\n", path, width, height);
        glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
/*
        float pixels[] = {
            0.0f, 0.0f, 0.0f,   1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,   0.0f, 0.0f, 0.0f
        };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_FLOAT, pixels);
*/
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // 13. Texture coordinates.
    /* UV coord, */
    const float textureCoord[] = {
    // in stb_image system:
        /*0.5, 0.0,
        0.0, 1.0,
        1.0, 1.0,*/
    // In normal GL system:
        0.5, 1.0,
        1.0, 0.0,
        0.0, 0.0,
    };
    {
        // 13.1.
        int aTexLoc = glGetAttribLocation(shader_program, "aTex");

        // 13.2.
        glVertexAttribPointer(aTexLoc, 2, GL_FLOAT, GL_TRUE, 2 * sizeof(float), textureCoord);

        // 10.3. Enable the usage of the vertex input array.
        glEnableVertexAttribArray(aTexLoc);
    }

    // 14. Connect the "texture" to the sampler in the shader
    {
        // 14.1. Activate the texture unit 1.
        /* Using the texture unit 1 by desgin here for example purposes. */
        glActiveTexture(GL_TEXTURE0 + 1);

        // 14.2. Bind the "texture" to the active texture unit.
        /* After this the GL_TEXTURE0 texture unit is bound to the "texture" object. */
        glBindTexture(GL_TEXTURE_2D, texture);

        // 14.3. No need to keep the active texture unit.
        glActiveTexture(0);

        // 14.3. Query the "image" sampler's location.
        int imageSamplerLoc = glGetUniformLocation(shader_program, "image");

        // Switch to the shader to set the uniform value.
        glUseProgram(shader_program);

        // 14.x. Set the sampler's "value" to the texture unit 1.
        glUniform1i(imageSamplerLoc, 0 + 1);

        // Disable the program for now.
        glUseProgram(0);
    }

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
        glUniform3f(uniformColorLoc, 1.0, 1.0, 1.0);

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

