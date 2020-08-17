cmake_minimum_required(VERSION 3.8)
project(glesexamples)

include(FindPkgConfig)

pkg_check_modules(GLFW3 glfw3)
pkg_check_modules(GLESv2 glesv2)

include_directories(
  ${GLFW3_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

function(add_program BIN_NAME SRC_NAME)
  add_executable(${BIN_NAME} ${SRC_NAME})
  target_link_libraries(${BIN_NAME} ${GLFW3_LIBRARIES} ${GLESv2_LIBRARIES} m)
  if (ARGV2)
    target_compile_definitions(${BIN_NAME} PRIVATE ${ARGV2})
  endif()
endfunction(add_program)

add_subdirectory(01_gles_glfw)
add_subdirectory(02_gles_triangle)
add_subdirectory(03_gles_vertex_attrib)
add_subdirectory(04_gles_uniform)
add_subdirectory(04_gles_texture)
add_subdirectory(05_gles_glm)
add_subdirectory(06_gles_vao)

add_subdirectory(x_gles_wireframe)
