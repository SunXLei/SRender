#pragma once
#include "maths.h"
#include "../shader/shader.h"
#include "../win32/win32.h"
#include "./spainlock.hpp"
#define PI 3.14159265359
#define EPSILON 1e-5f
#define EPSILON2 1e-5f



const int WINDOW_HEIGHT = 600;
const int WINDOW_WIDTH = 800;

//rasterize triangle
void rasterize_singlethread(vec4 *clipcoord_attri, unsigned char* framebuffer, float *zbuffer, IShader& shader);
void rasterize_multithread(vec4 *clipcoord_attri, unsigned char* framebuffer, float *zbuffer, IShader& shader);
void draw_triangles(unsigned char* framebuffer, float *zbuffer,IShader& shader,int nface);