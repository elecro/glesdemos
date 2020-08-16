add_program(04_gles_texture gles_triangle_texture.cpp)

# image form: https://www.vets4pets.com/siteassets/species/cat/kitten/tiny-kitten-in-field.jpg?w=585&scale=down
add_custom_command(TARGET 04_gles_texture POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                       ${CMAKE_CURRENT_SOURCE_DIR}/kitten_10.jpg ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/)
