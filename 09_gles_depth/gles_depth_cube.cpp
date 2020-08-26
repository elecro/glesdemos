/**
 * Single file OpenGL ES example with minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ gles_depth_cube.cpp -o gles_depth_cube -lglfw -lGLESv2
 *
 * Run:
 * $ ./gles_depth_cube
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
#include <GLES3/gl32.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const char* cube_vertex_src = R"(#version 310 es
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

const char* cube_fragment_src = R"(#version 310 es
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
    gl_FragDepth = gl_FragCoord.z;
}
)";


const char* texture_display_vertex_src = R"(#version 310 es
precision highp float;

in vec2 aPos;
in vec2 aTex;

out vec2 vTex;

void main() {
    gl_Position = vec4(aPos, 0.0f, 1.0f);

    vTex = aTex;
})";

const char* texture_display_fragment_src = R"(#version 310 es
precision highp float;

in vec2 vTex;

uniform sampler2D inputImage;

out vec4 outColor;

void main() {
    outColor = vec4(texture(inputImage, vTex).rrr, 1.0f);
})";

static void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity,
                          GLsizei length, const GLchar* message, const void *userParam) {

    printf("-> %s\n", message);
}

static void ErrorCallbackGLFW(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

static unsigned int createShaderProgram(const char* vertex_src, const char* fragment_src) {
    // X.1. Create the vertex shader.
    unsigned int vertex_shader;
    {
        // X.1.1. Create a vertex shader object.
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        // X.1.2. Specify the shader source.
        glShaderSource(vertex_shader, 1, &vertex_src, NULL);

        // X.1.3. Compile the shader.
        glCompileShader(vertex_shader);

        // X.1.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(vertex_shader, 512, NULL, info);
            printf("Vertex shader error:\n%s\n", info);
            exit(-3);
        }
    }

    // X.2. Create the fragment shader.
    unsigned int fragment_shader;
    {
        // X.2.1. Create a fragment shader object.
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        // X.2.2. Specify the shader source.
        glShaderSource(fragment_shader, 1, &fragment_src, NULL);

        // X.2.3. Compile the shader.
        glCompileShader(fragment_shader);

        // X.2.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(fragment_shader, 512, NULL, info);
            printf("Fragment shader error:\n%s\n", info);
            exit(-3);
        }
    }

    // X.3. Create a shader program and attach the vertex/fragment shaders.
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
            exit(-3);
        }
    }

    // X.4. The vertex and fragment shaders can be removed after linking.
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    return shader_program;
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

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_error, NULL);

    // 5. Set the view port to match the window size.
    int display_w, display_h;
    {
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
    }

    // 6. Create the shader program for the Cube rendering.
    unsigned int cube_program = createShaderProgram(cube_vertex_src, cube_fragment_src);
    unsigned int texture_program = createShaderProgram(texture_display_vertex_src, texture_display_fragment_src);

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

    unsigned int cube_vbo;
    {
        // V.1.1. Generate the buffer object.
        glGenBuffers(1, &cube_vbo);

        // V.1.2. Bind the VBO to the "GL_ARRAY_BUFFER".
        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);

        // V.1.3. "Upload" the data for the active "GL_ARRAY_BUFFER".
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // V.1.4. Unbind the "GL_ARRAY_BUFFER".
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // V.1.2. Specify the Vertex Array Object for the Cube
    /* VAO is used to describe how the VBOs are accessed (layout/format). */
    unsigned int cube_vao;
    {
        int aPosLoc = glGetAttribLocation(cube_program, "aPos");

        glGenVertexArrays(1, &cube_vao);

        glBindVertexArray(cube_vao);

        glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);

        glVertexAttribPointer(aPosLoc, 3, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), NULL);

        // 10.3. Enable the usage of the vertex input array.
        glEnableVertexAttribArray(aPosLoc);

        glBindVertexArray(0);
    }


    // D.X. Create a quad to render the depth image on.
    unsigned int texture_quad_vbo;
    {
        static const float texture_quad[] = {
            // vec2 aPos  // vec2 aTex
            -1.0,  1.0,  0.0, 1.0,
            -1.0, -1.0,  0.0, 0.0,
             1.0,  1.0,  1.0, 1.0,
             1.0, -1.0,  1.0, 0.0,
        };

        glGenBuffers(1, &texture_quad_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, texture_quad_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texture_quad), texture_quad, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // D.X. Create and configure the VAO for the depth image rendering.
    unsigned int texture_vao;
    {
        int aPosLoc = glGetAttribLocation(texture_program, "aPos");
        int aTexLoc = glGetAttribLocation(texture_program, "aTex");

        glGenVertexArrays(1, &texture_vao);
        glBindVertexArray(texture_vao);
        glBindBuffer(GL_ARRAY_BUFFER, texture_quad_vbo);
        glVertexAttribPointer(aPosLoc, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), NULL);
        glVertexAttribPointer(aTexLoc, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));

        glEnableVertexAttribArray(aPosLoc);
        glEnableVertexAttribArray(aTexLoc);

        glBindVertexArray(0);
    }

    // D.1. Create a depth only texture for FBO.
    unsigned int fboDepthTexture;
    {
        // D.1.1. Generate a texture.
        glGenTextures(1, &fboDepthTexture);

        // D.1.2. Bind the texture as a 2D image.
        glBindTexture(GL_TEXTURE_2D, fboDepthTexture);

        // D.1.3. Initialize the bound 2D image as a DEPTH image.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, display_w, display_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // D.1.4. Configure the bound 2D image's sampler information.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // D.1.5. Unbind the 2D image.
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // D.2. Create a RenderBuffer for the color image (this could be a texture also).
    unsigned int colorRB;
    {
        glGenRenderbuffers(1, &colorRB);
        glBindRenderbuffer(GL_RENDERBUFFER, colorRB);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, display_w, display_h);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    // D.3. Create a depth only FBO using the depth texture.
    unsigned int fboDepth;
    {
        // D.3.1. Generate an FBO.
        glGenFramebuffers(1, &fboDepth);

        // D.3.2. Bind this newly generated FBO.
        glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);

        // D.3.4. Bind the depth texture for the current FBO.
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fboDepthTexture, 0);

        // D.3.5. Attach the render buffer as a color attachment. (See next comment)
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRB);

        /* If the color output is not needed just disable the color attachments:
        // D.3.x. Disable all color attachments:
        GLenum disableColor = GL_NONE;
        glDrawBuffers(1, &disableColor);
        glReadBuffer(disableColor);
        */

        // D.3.x. Check the FBO status.
        GLenum fboResult = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboResult != GL_FRAMEBUFFER_COMPLETE) {
            printf("ERROR::FRAMEBUFFER:: Framebuffer is not complete! (0x%x)\n", fboResult);
            return -1;
        }

        // D.3.6. Unbind the framebuffer.
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // 11. Query the uniform location for the Cube
    int uniformColorLoc = glGetUniformLocation(cube_program, "uColor");
    int modelLoc = glGetUniformLocation(cube_program, "model");
    int viewLoc  = glGetUniformLocation(cube_program, "view");

    int projectionLoc = glGetUniformLocation(cube_program, "projection");
    {
        glm::mat4 projection = glm::mat4(1.0f);
        projection = glm::perspective(glm::radians(45.0f), (float)display_w / (float)display_h, 0.1f, 100.0f);
        glUseProgram(cube_program);

        glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

        glUseProgram(0);
    }

    // D.X. Query uniforms for the texture rendering and configure them.
    {
        // D.X.1. Activate the 5th texture unit.
        glActiveTexture(GL_TEXTURE0 + 5);
        // D.X.2. Connect the fboDepthTexture to the currently active texture unit.
        glBindTexture(GL_TEXTURE_2D, fboDepthTexture);
        // D.X.3. Switch off rom the current active texture unit.
        glActiveTexture(0);

        // D.X.4. Switch to the texture quad drawer program.
        glUseProgram(texture_program);

        int uniformTextureSampler = glGetUniformLocation(texture_program, "inputImage");
        // D.X.5. Configure the "inputImage" sampler to use the 5th texture unit.
        glUniform1i(uniformTextureSampler, 5);

        // D.X.6. Disable the current program.
        glUseProgram(0);
    }

    glEnable(GL_DEPTH_TEST);
    // D.X.3. Enable scissor to draw only onto the specific region.
    glEnable(GL_SCISSOR_TEST);

    static float color = 0;
    // X. Create a render loop.
    while (!glfwWindowShouldClose(window))
    {
        // X. Poll and handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // X. Draw the cube onto the fboDepth.
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fboDepth);
            // X. Configure the render/draw region for the cube.
            glViewport(0, 0, display_w, display_h);
            glScissor(0, 0, display_w, display_h);

            // X. Clear the color image.
            glClearColor(0.0, 0.3, 0.3, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // X. Use the shader program to draw.
            glUseProgram(cube_program);

            // V.3. Use the VAO
            glBindVertexArray(cube_vao);

            // XX. Update the transformation matrix.
            {
                glm::mat4 model         = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
                glm::mat4 view          = glm::mat4(1.0f);

                //model = glm::rotate(model, glm::radians(-55.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
                view  = glm::translate(view, glm::vec3(.0f, 0.0f, -1.5f));
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
        }

        // D.X. Draw the final image.
        {
            // D.X.0. Switch to the output/window framebuffer to draw onto.
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

            // D.X.1. Copy the whole color image onto the output.
            glBlitFramebuffer(0, 0, display_w, display_h, 0, 0, display_w, display_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

            // D.X.2. Configure the draw output to be a smaller "window"/region.
            glViewport(10, 10, 300, 300);
            // D.X.4. Specify the region to draw onto.
            glScissor(10, 10, 300, 300);

            // D.X.5. Clear the draw region.
            glClearColor(0.0, 0.0, 0.0, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // D.X.6. Use the texture quad drawer program.
            glUseProgram(texture_program);

            // D.X.7. Bind the texture quad vao.
            glBindVertexArray(texture_vao);

            // D.X.8. Draw the quad (and render the depth texture).
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // X. Swap the fron-back buffers to display the rendered image in the window.
        glfwSwapBuffers(window);
    }

    // XX. Destroy the window.
    glfwDestroyWindow(window);
    // XX. Destroy the GLFW.
    glfwTerminate();

    return 0;
}
