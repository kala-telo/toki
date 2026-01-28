#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#ifdef __linux__
#define RGFW_WAYLAND
#endif
#define RGFWDEF
#include <RGFW.h>

#define CLAY_IMPLEMENTATION
#include <clay.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GLES3/gl3.h>
#endif
#define GLSL_VERSION "#version 330 core"
#define CLAY_RENDERER_GLES3_IMPLEMENTATION
#include <clay_renderer_gles3.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_LOADER_STB_IMPLEMENTATION
#include <clay_renderer_gles3_loader_stb.h>
