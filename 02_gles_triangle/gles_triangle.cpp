/**
 * Single file OpenGL ES start example minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ gles_triangle.cpp -o gles_triangle -lEGL -lGLESv2
 *
 * Run:
 * $ ./gles_triangle
 *
 * Dependencies:
 *  * C++11
 *  * Open GL ES 3.1+
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

#include <fstream>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <GLFW/glfw3.h>

// From the EGL_KHR_create_context extension:
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

const char* vertex_src = R"(#version 310 es
precision highp float;

out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.0, -0.5)
);

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
)";

const char* fragment_src = R"(#version 310 es
precision highp float;

out vec4 outColor;

void main() {
    outColor = vec4(1.0f, 0.5f, 0.1f, 1.0f);
}
)";

static void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity,
                          GLsizei length, const GLchar* message, const void *userParam) {

    printf("-> %s\n", message);
}

int main(int argc, char **argv) {
    const char* outputFileName = "out.ppm";
    int renderImageWidth = 256;
    int renderImageHeight = 256;

    // 1. Access the display
    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    // 2. Initialize EGL and get the version information.
    int major = 0;
    int minor = 0;
    eglInitialize(display, &major, &minor);
    printf("EGL %d.%d\n", major, minor);

    // 3. Use the Open GL ES API subset.
    eglBindAPI(EGL_OPENGL_ES_API);

    // 4. Select an EGL configuration
    EGLConfig config;
    {
        // 4.1. At the moment select the first config matching the input attributes.
        static const EGLint configAttribs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
            EGL_NONE
        };
        EGLint numConfigs;

        // 4.2. Actually choose the configuration.
        if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
            printf("Error: couldn't get an EGL visual config\n");
            return -1;
        }

        if (numConfigs != 1) {
            return -2;
        }
    }

    // 5. Create an EGL OpenGL ES context.
    EGLContext context;
    {
        static const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
            EGL_NONE
        };

        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (!context) {
            printf("Error: eglCreateContext failed\n");
            return -3;;
        }
    }

    // 6. Create a PBufferSurface.
    /* PBufferSurface is an in memory surface. */
    EGLSurface surface;
    {
        static const EGLint pbufferAttribs[] = {
            EGL_WIDTH, renderImageWidth,
            EGL_HEIGHT, renderImageHeight,
            EGL_NONE
        };

        surface = eglCreatePbufferSurface(display, config, pbufferAttribs);

        if (surface == EGL_NO_SURFACE) {
            printf("Error: unable to create PBufferSurface\n");
            return -1;
        }
    }

    // 7. Activate the context.
    {
        eglMakeCurrent(display, surface, surface, context);
    }

    // X. Extra: add a debug callback to have info on GL/ES errors.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_error, NULL);

    // 8. Specifly our viewport.
    {
        glViewport(0, 0, renderImageWidth, renderImageWidth);
    }

    // 9. Create the vertex shader.
    unsigned int vertex_shader;
    {
        // 9.1. Create a vertex shader object.
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        // 9.2. Specify the shader source.
        glShaderSource(vertex_shader, 1, &vertex_src, NULL);

        // 9.3. Compile the shader.
        glCompileShader(vertex_shader);

        // 9.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(vertex_shader, 512, NULL, info);
            printf("Vertex shader error:\n%s\n", info);
            return -3;
        }
    }

    // 10. Create the fragment shader.
    unsigned int fragment_shader;
    {
        // 10.1. Create a fragment shader object.
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

        // 10.2. Specify the shader source.
        glShaderSource(fragment_shader, 1, &fragment_src, NULL);

        // 10.3. Compile the shader.
        glCompileShader(fragment_shader);

        // 10.4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(fragment_shader, 512, NULL, info);
            printf("Fragment shader error:\n%s\n", info);
            return -3;
        }
    }

    // 11. Create a shader program and attach the vertex/fragment shaders.
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

    // 12. The vertex and fragment shaders can be removed after linking.
    {
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    // 13. Do the draw
    {
        glClearColor(0.0, 0.5, 0.5, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // 14. Read back rendered image.
    /* glReadPixels will wait for the draw to finish. */
    {
        // 14.1. Create a vector to store the pixel data.
        /* width * height * component count * pixel size */
        std::vector<uint8_t> pixels;
        pixels.resize(renderImageWidth * renderImageHeight * 4 * sizeof(uint8_t));

        // 14.2. Configure the read back to have a 4 pixel(byte) alignment.
        glPixelStorei(GL_PACK_ALIGNMENT, 4);

        // 14.3. Read back the pixels (4 bytes per pixel) into the vector.
        glReadPixels(0, 0, renderImageWidth, renderImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

        // 14.4. Write out the image to a ppm file.
        {
            std::ofstream file(outputFileName, std::ios::out | std::ios::binary);
            // ppm header
            file << "P6\n" << renderImageWidth << "\n" << renderImageHeight << "\n" << 255 << "\n";

            // ppm binary pixel data
            // As the image format is R8G8B8A8 one "pixel" size is 4 bytes (uint32_t)
            uint32_t *row = (uint32_t*)pixels.data();

            for (uint32_t y = 0; y < renderImageHeight; y++) {
                for (uint32_t x = 0; x < renderImageWidth; x++) {
                    // Only copy the RGB values (3)
                    file.write((const char*)row, 3);
                    row++;
                }
            }
            file.close();
        }
    }

    // XX. Destroy the shader program.
    glDeleteProgram(shader_program);

    // XX. Terminate EGL resources.
    eglTerminate(display);

    return 0;
}

