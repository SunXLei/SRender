#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include "./tgaimage.h"
#include "maths.h"

typedef struct cubemap cubemap_t;

class Model {
private:
	std::vector<vec3> verts_;
	std::vector<std::vector<int> > faces_; // attention, this Vec3i means vertex/uv/normal
	std::vector<vec3> norms_;
	std::vector<vec2> uv_;
	TGAImage diffusemap_;
	TGAImage normalmap_;
	TGAImage specularmap_;
	TGAImage roughnessmap_;
	TGAImage metalnessmap_;
	TGAImage occlusion_map;
	TGAImage emision_map;
	

	void load_cubemap(const char *filename);
	void load_texture(std::string filename, const char *suffix, TGAImage &img);
	void load_texture(std::string filename, const char *suffix, TGAImage *img);
public:
	Model(const char *filename, int is_skybox = 0, int is_from_mmd = 0);
	~Model();
	//skybox
	cubemap_t *environment_map;
	int is_skybox;
	int is_from_mmd;

	int nverts();
	int nfaces();
	vec3 normal(int iface, int nthvert);
	vec3 normal(vec2 uv);
	vec3 vert(int i);
	vec3 vert(int iface, int nthvert);

	vec2 uv(int iface, int nthvert);
	vec3 diffuse(vec2 uv);
	float roughness(vec2 uv);
	float metalness(vec2 uv);
	vec3 emission(vec2 uv);
	float occlusion(vec2 uv);
	float specular(vec2 uv);

	//vec3 right_sample(vec2 uv);
	//vec3 left_sample(vec2 uv);
	//vec3 top_sample(vec2 uv);
	//vec3 bottom_sample(vec2 uv);
	//vec3 front_sample(vec2 uv);
	//vec3 back_sample(vec2 uv);
	std::vector<int> face(int idx);
};
#endif //__MODEL_H__