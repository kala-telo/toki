#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
#define RGFW_WAYLAND
#define RGFWDEF
#include <RGFW.h>

#define CLAY_IMPLEMENTATION
#include <clay.h>

#include <GLES3/gl3.h>
#define GLSL_VERSION "#version 330 core"
#define CLAY_RENDERER_GLES3_IMPLEMENTATION
#include <clay_renderer_gles3.h>
#define STB_IMAGE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define CLAY_RENDERER_GLES3_LOADER_STB_IMPLEMENTATION
#include <clay_renderer_gles3_loader_stb.h>
