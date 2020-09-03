/**
 * Single file OpenGL ES compute shader example minimal "helper" methods.
 *
 * Compile with shaderc:
 * $ g++ x_gles_compute_pure.cpp -o x_gles_compute_pure -lEGL -lGLESv2
 *
 * Run:
 * $ ./x_gles_compute_pure
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

// From the EGL_KHR_create_context extension:
#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x0040
#endif

const char* compute_src = R"(#version 310 es

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(std140, binding=0) buffer wBuffer {
    ivec2 values[10];
} data;

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    data.values[pos.x] = ivec2(1000 + pos.x, 2000 + pos.x);
}
)";

static void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity,
                          GLsizei length, const GLchar* message, const void *userParam) {
    printf("-> %s\n", message);
}

int main(int argc, char **argv) {

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
            return -3;
        }
    }

    // 6. Create a PBufferSurface.
    /* PBufferSurface is an in memory surface. */
    /*EGLSurface surface;
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
    }*/

    // 7. Activate the context.
    {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
    }

    // X. Extra: add a debug callback to have info on GL/ES errors.
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_error, NULL);

    // . Create the vertex shader.
    unsigned int compute_shader;
    {
        // .1. Create a compute shader object.
        compute_shader = glCreateShader(GL_COMPUTE_SHADER);

        // .2. Specify the shader source.
        glShaderSource(compute_shader, 1, &compute_src, NULL);

        // .3. Compile the shader.
        glCompileShader(compute_shader);

        // .4. Check if there is any error during compilation.
        int success;
        glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char info[512];
            glGetShaderInfoLog(compute_shader, 512, NULL, info);
            printf("Compute shader error:\n%s\n", info);
            return -3;
        }
    }

    // . Create a shader program and attach the compute shader.
    unsigned int compute_program;
    {
        compute_program = glCreateProgram();
        glAttachShader(compute_program, compute_shader);
        glLinkProgram(compute_program);

        int success;
        glGetProgramiv(compute_program, GL_LINK_STATUS, &success);
        if(!success) {
            char info[512];
            glGetProgramInfoLog(compute_program, 512, NULL, info);
            printf("Program error:\n%s\n", info);
            return -3;
        }
    }

    // . The compute shader can be removed after linking.
    {
        glDeleteShader(compute_shader);
    }

    // . Upload the initial buffer data.
    int itemCount = 10 * (2 + 2); // ivec2 + (2 * int padding)
    size_t calcBufferSize = itemCount * sizeof(int); // ivec2 * 10 in std140 layout
    unsigned int calcBufferId;
    {
        glGenBuffers(1, &calcBufferId);
        glBindBuffer(GL_ARRAY_BUFFER, calcBufferId);
        glBufferData(GL_ARRAY_BUFFER, calcBufferSize, NULL, GL_STATIC_DRAW);
        {
            // . Map the buffer to CPU so a simple copy/assignment can "upload" the data to GPU.
            void *dataPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, calcBufferSize, GL_MAP_WRITE_BIT); //| GL_MAP_FLUSH_EXPLICIT_BIT);
            int *inputPtr = (int*)dataPtr;
            // Generate the input data.
            /* The input data is: 1, 2, 3, 4, ....   */
            for (int idx = 0; idx < itemCount; idx++) {
                inputPtr[idx] = idx + 1;
            }

            //glFlushMappedBufferRange(GL_ARRAY_BUFFER, 0, calcBufferSize);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    {
        glUseProgram(compute_program);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, calcBufferId);

        glDispatchCompute(10, 1, 1);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
        glUseProgram(0);
    }

    {
        glBindBuffer(GL_ARRAY_BUFFER, calcBufferId);
        {
            void *dataPtr = glMapBufferRange(GL_ARRAY_BUFFER, 0, calcBufferSize, GL_MAP_READ_BIT);
            int *readPtr = (int*)dataPtr;
            for (int idx = 0; idx < itemCount; idx++) {
                printf("-> pos: %2d => %2d\n", idx + 1, readPtr[idx]);
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    // XX. Destroy the compute program.
    glDeleteProgram(compute_program);

    // XX. Terminate EGL resources.
    eglTerminate(display);

    return 0;
}
