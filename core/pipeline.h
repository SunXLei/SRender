#pragma once
#include "./macro.h"
#include "./maths.h"
#include "./spainlock.hpp"
#include "../shader/shader.h"
#include "../platform/win32.h"

const int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 600;

//rasterizer assembled triangle, interpolate
void rasterize_singlethread(vec4* clipcoord_attri, unsigned char* framebuffer, float* zbuffer, IShader& shader);

//notice this is for each model face
void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface);

//void draw_triangles(unsigned char* framebuffer, float* zbuffer, IShader& shader, int nface);