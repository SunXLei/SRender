#include <iostream>
#include <time.h>
#include "./core/tgaimage.h"
#include "./shader/shader.h"
#include "./core/model.h"
#include "./win32/win32.h"
#include "./core/camera.h"
#include "./core/pipeline.h"
#include "./core/sample.h"
#include "./core/scene.h"

#define MAX_MODEL_NUM 10

using namespace std;

vec3 eye(0, 1, 5);
vec3 up(0, 1, 0);
vec3 target(0, 1, 0);
mat4 model_mat;
mat4 view;
mat4 perspective;

void clear_zbuffer(int width, int height, float* zbuffer)
{
	for (int i = 0; i < width*height; i++)
		zbuffer[i] = 100000;
}

void clear_framebuffer(int width, int height, unsigned char* framebuffer)
{
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int index = (i * width + j) * 4;

			framebuffer[index + 2] = 80;
			framebuffer[index + 1] = 56;
			framebuffer[index] = 56;
		}
	}
}

void update_matrix_data(Camera &camera, mat4 perspective, IShader *shader_model, IShader *shader_skybox)
{
	view = mat4_lookat(camera.eye, camera.target, camera.up);
	mat4 mvp = perspective * view;
	shader_model->payload.camera_view_matrix = view;
	shader_model->payload.mvp_matrix = mvp;

	if (shader_skybox != NULL)
	{
		mat4 view_skybox = view;
		view_skybox[0][3] = 0;
		view_skybox[1][3] = 0;
		view_skybox[2][3] = 0;
		shader_skybox->payload.camera_view_matrix = view_skybox;
		shader_skybox->payload.mvp_matrix = perspective * view_skybox;
	}
}

static scene_t scenes[]
{
	{"fuhua",build_fuhua_scene},
	{"qiyana",build_qiyana_scene},
	{"yayi",build_yayi_scene},
	{"xier",build_xier_scene},
	{"helmet",build_helmet_scene},
	{"gun",build_gun_scene},
};

int main()
{
	int width = WINDOW_WIDTH, height = WINDOW_HEIGHT;

	//malloc memory for zbuffer and framebuffer
	float *zbuffer;
	unsigned char* framebuffer;
	zbuffer = (float *)malloc(sizeof(float) * width*height);
	framebuffer = (unsigned char*)malloc(sizeof(unsigned char) * width*height * 4);
	memset(framebuffer, 0, sizeof(unsigned char)*width*height * 4);

	//set mvp matrix
	model_mat = mat4::identity();
	view = mat4_lookat(eye, target, up);
	perspective = mat4_perspective(60, (float)(width)/height, -0.1, -10000);

	//create camera
	Camera camera(eye, target, up, (float)(width) / height);

	//initialize models and shaders
	int model_num = 0;
	Model *model[MAX_MODEL_NUM];
	IShader *shader_model;
	IShader *shader_skybox;

	//build scene randomly
	srand((unsigned int)time(NULL));
	int i = rand() % 6;
	scenes[i].build_scene(model, model_num, &shader_model,&shader_skybox,perspective,&camera);

	int num_frames = 0;
	window_init(width, height, "SRender");
	float print_time = platform_get_time();
	while (!window->is_close)
	{
		float curr_time = platform_get_time();

		//reset buffer
		clear_framebuffer(width, height, framebuffer);
		clear_zbuffer(width, height, zbuffer);

		//handle camera events and update view, perspective matrix
		handle_events(camera);
		update_matrix_data(camera, perspective, shader_model, shader_skybox);

		//draw loop
		for (int m = 0; m < model_num; m++)
		{
			shader_model->payload.model = model[m];
			if(shader_skybox)
				shader_skybox->payload.model = model[m];

			IShader *shader;
			if (model[m]->is_skybox)
				shader = shader_skybox;
			else
				shader = shader_model;
	
			int num = model[m]->nfaces();
			for (int i = 0; i < num; i++)
			{
				draw_triangles(framebuffer, zbuffer, *shader, i);
			}
		}

		//calculate FPS
		num_frames += 1;
        if (curr_time - print_time >= 1) {
            int sum_millis = (int)((curr_time - print_time) * 1000);
            int avg_millis = sum_millis / num_frames;
            printf("fps: %3d, avg: %3d ms\n", num_frames, avg_millis);
            num_frames = 0;
            print_time = curr_time;
        }

		//reset mouse information
		window->mouse_info.wheel_delta = 0;
		window->mouse_info.orbit_delta = vec2(0,0);
		window->mouse_info.fv_delta = vec2(0, 0);

		//display image
		window_draw(framebuffer);
		msg_dispatch();
	}


	//free memory
	free(framebuffer);
	free(zbuffer);
	delete shader_model;
	if (shader_skybox != NULL)
		delete shader_skybox;
	for (int i = 0; i < model_num; i++)
	{
		if (model[i] != NULL)
			delete model[i];
	}
	window_destroy();

	system("pause");
	return 0;
}
