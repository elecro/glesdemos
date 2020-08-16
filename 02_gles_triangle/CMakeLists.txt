add_program(02_gles_glfw_triangle gles_glfw_triangle.cpp)

pkg_check_modules(EGL egl)

add_program(02_gles_triangle gles_triangle.cpp)
target_link_libraries(02_gles_triangle ${EGL_LIBRARIES})
