/**
 * Single file OpenGL ES Poor man's "wireframe" logic.
 *
 * Compile with shaderc:
 * $ g++ x_gles_wireframe.cpp -o x_gles_wireframe -lglfw -lGLESv2
 *
 * Run:
 * $ ./x_gles_wireframe
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

const char* vertex_src = R"(#version 310 es
precision highp float;

out vec3 fragColor;

vec2 positions[6] = vec2[](
    vec2(-0.5, 0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, -0.5),

    vec2(-0.5, 0.5),
    vec2(0.5, 0.5),
    vec2(0.5, -0.5)
);

vec3 VBC[3] = vec3[](
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

out vec3 wireframeDistance;

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);

    // Poor man's "wireframe"
    // For each vertex assign a different vec3 value:
    // 1st vertice: vec3(1.0f, 0.0f, 0.0f)
    // 2nd vertice: vec3(0.0f, 1.0f, 0.0f)
    // 3rd vertice: vec3(0.0f, 0.0f, 1.0f)
    // This will be used in the frament shader.
    wireframeDistance = vec3(0.0f);
    wireframeDistance[gl_VertexID % 3] = 1.0f;
}
)";

const char* fragment_src = R"(#version 310 es
precision highp float;

in vec3 wireframeDistance;

out vec4 outColor;

uniform int wireframeToggle;

void main() {
    float alpha;

    switch (wireframeToggle) {
        case 0:
        case 1: alpha = 1.0f; break;
        case 2: alpha = 0.0f; break;
    }

    // Check if one of the "wireframeDistance" vec3 values are less then the 0.0f value.
    // If it is true, then this position is possibly at the edge of the vertice color it
    // as a wireframe.
    if (wireframeToggle > 0 && any(lessThan(wireframeDistance, vec3(0.01f)))) {
        outColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    } else {
        outColor = vec4(1.0f, 0.5f, 0.1f, alpha);
    }

    /* Withtout if/switch
    float wireframe = float(any(lessThan(bar, vec3(0.01f)))) * float(wireframeToggle > 0);
    vec3 baseColor = vec3(1.0, 0.5f, 0.1f);
    outColor = vec4(baseColor, float(wireframeToggle < 2)) + wireframe;
    */
}
)";

static void ErrorCallbackGLFW(int error, const char* description) {
    printf("Glfw Error %d: %s\n", error, description);
}

// Wireframe modes:
/*
 0: No Wireframe, fill color
 1: Wireframe, fill color
 2: Wireframe, no fill color
 */
static int wireframeToggle = 0;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
   if (key == GLFW_KEY_W && action == GLFW_PRESS) {
        wireframeToggle = (wireframeToggle + 1) % 3;
        printf("Wireframe Mode: %d\n", wireframeToggle);
    }
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
    glfwSetKeyCallback(window, key_callback);

    printf("Press 'W' to switch wireframe mode.\n");

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

    // "Transparency":
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int uniformWireframeToggleLocation = glGetUniformLocation(shader_program, "wireframeToggle");

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

        // X. Draw the triangles.
        glUniform1i(uniformWireframeToggleLocation, wireframeToggle);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // X. Swap the fron-back buffers to display the rendered image in the window.
        glfwSwapBuffers(window);
    }

    // XX. Destroy the window.
    glfwDestroyWindow(window);
    // XX. Destroy the GLFW.
    glfwTerminate();

    return 0;
}

