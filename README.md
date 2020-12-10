# Minimal Open GL/ES examples

Ubuntu packages: `libglfw3-dev`, `libglm-dev`

```sh
$ cmake -Bbuild -H.
$ make -C build
```

## Using system GLFW

```
$ cmake -Bbuild -H. -DUSE_SYSTEM_GLFW=1
```

## gles_glfw

Base code to create an OpenGL ES context and window with glfw.
