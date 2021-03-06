#include "./model.h"

#include <io.h> 
#include <iostream>
#include <fstream>
#include <sstream>

#include "../shader/shader.h"

Model::Model(const char *filename, int is_skybox, int is_from_mmd)
	: is_skybox(is_skybox), is_from_mmd(is_from_mmd)
{
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail())
	{
		printf("load model failed\n");
		return;
	}

	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) 
		{
			iss >> trash;
			vec3 v;
			for (int i = 0; i < 3; i++)
				iss >> v[i];
		
			verts.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) 
		{
			iss >> trash >> trash;
			vec3 n;
			for (int i = 0; i < 3; i++) 
				iss >> n[i];

			norms.push_back(n);
		}
		else if (!line.compare(0, 3, "vt ")) 
		{
			iss >> trash >> trash;
			vec2 uv;
			for (int i = 0; i < 2; i++) 
				iss >> uv[i];

			if (is_from_mmd)
				uv[1] = 1 - uv[1];
			uvs.push_back(uv);
		}
		else if (!line.compare(0, 2, "f ")) 
		{
			std::vector<int> f;
			int tmp[3];
			iss >> trash;
			while (iss >> tmp[0] >> trash >> tmp[1] >> trash >> tmp[2]) 
			{
				for (int i = 0; i < 3; i++) 
					tmp[i]--; // in wavefront obj all indices start at 1, not zero

				f.push_back(tmp[0]); f.push_back(tmp[1]); f.push_back(tmp[2]);
			}
			faces.push_back(f);
		}
	}
	std::cerr << "# v# " << verts.size() << " f# " << faces.size() << " vt# " << uvs.size() << " vn# " << norms.size() << std::endl;

	create_map(filename);

	environment_map = NULL;
	if (is_skybox)
	{
		environment_map = new cubemap_t();
		load_cubemap(filename);
	}
}

Model::~Model() 
{
	if (diffusemap) delete diffusemap; diffusemap = NULL;
	if (normalmap) delete normalmap; normalmap = NULL;
	if (specularmap) delete specularmap; specularmap = NULL;
	if (roughnessmap) delete roughnessmap; roughnessmap = NULL;
	if (metalnessmap) delete metalnessmap; metalnessmap = NULL;
	if (occlusion_map) delete occlusion_map; occlusion_map = NULL;
	if (emision_map) delete emision_map; emision_map = NULL;

	if (environment_map)
	{
		for (int i = 0; i < 6; i++)
			delete environment_map->faces[i];
		delete environment_map;
	}
}

void Model::create_map(const char *filename)
{
	diffusemap		= NULL;
	normalmap		= NULL;
	specularmap		= NULL;
	roughnessmap	= NULL;
	metalnessmap	= NULL;
	occlusion_map	= NULL;
	emision_map		= NULL;

	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");

	texfile = texfile.substr(0, dot) + std::string("_diffuse.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		diffusemap = new TGAImage();
		load_texture(filename, "_diffuse.tga", diffusemap);
	}

	texfile = texfile.substr(0, dot) + std::string("_normal.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		normalmap = new TGAImage();
		load_texture(filename, "_normal.tga", normalmap);
	}

	texfile = texfile.substr(0, dot) + std::string("_spec.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		specularmap = new TGAImage();
		load_texture(filename, "_spec.tga", specularmap);
	}

	texfile = texfile.substr(0, dot) + std::string("_roughness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		roughnessmap = new TGAImage();
		load_texture(filename, "_roughness.tga", roughnessmap);
	}

	texfile = texfile.substr(0, dot) + std::string("_metalness.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		metalnessmap = new TGAImage();
		load_texture(filename, "_metalness.tga", metalnessmap);
	}

	texfile = texfile.substr(0, dot) + std::string("_emission.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		emision_map = new TGAImage();
		load_texture(filename, "_emission.tga", emision_map);
	}

	texfile = texfile.substr(0, dot) + std::string("_occlusion.tga");
	if (_access(texfile.data(), 0) != -1)
	{
		occlusion_map = new TGAImage();
		load_texture(filename, "_occlusion.tga", metalnessmap);
	}
}

void Model::load_cubemap(const char *filename)
{
	environment_map->faces[0] = new TGAImage();
	load_texture(filename, "_right.tga", environment_map->faces[0]);
	environment_map->faces[1] = new TGAImage();
	load_texture(filename, "_left.tga", environment_map->faces[1]);
	environment_map->faces[2] = new TGAImage();
	load_texture(filename, "_top.tga", environment_map->faces[2]);
	environment_map->faces[3] = new TGAImage();
	load_texture(filename, "_bottom.tga", environment_map->faces[3]);
	environment_map->faces[4] = new TGAImage();
	load_texture(filename, "_back.tga", environment_map->faces[4]);
	environment_map->faces[5] = new TGAImage();
	load_texture(filename, "_front.tga", environment_map->faces[5]);
}

int Model::nverts() 
{
	return (int)verts.size();
}

int Model::nfaces() 
{
	return (int)faces.size();
}

std::vector<int> Model::face(int idx) 
{
	std::vector<int> face;
	for (int i = 0; i < 3; i++) 
		face.push_back(faces[idx][i*3]);
	return face;
}

vec3 Model::vert(int i) 
{
	return verts[i];
}

vec3 Model::vert(int iface, int nthvert) 
{
	return verts[faces[iface][nthvert*3]];
}

vec2 Model::uv(int iface, int nthvert) 
{
	return uvs[faces[iface][nthvert * 3+1]];
}

vec3 Model::normal(int iface, int nthvert) 
{
	int idx = faces[iface][nthvert * 3+2];
	return unit_vector(norms[idx]);
}

void Model::load_texture(std::string filename, const char *suffix, TGAImage &img) 
{
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != std::string::npos) 
	{
		texfile = texfile.substr(0, dot) + std::string(suffix);
		img.read_tga_file(texfile.c_str());
		img.flip_vertically();
	}
}

void Model::load_texture(std::string filename, const char *suffix, TGAImage *img) 
{
	std::string texfile(filename);
	size_t dot = texfile.find_last_of(".");
	if (dot != std::string::npos) {
		texfile = texfile.substr(0, dot) + std::string(suffix);
		img->read_tga_file(texfile.c_str());
		img->flip_vertically();
	}
}

vec3 Model::diffuse(vec2 uv) 
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * diffusemap->get_width();
	int uv1 = uv[1] * diffusemap->get_height();
	TGAColor c = diffusemap->get(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f;
	return res;
}

vec3 Model::normal(vec2 uv) 
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * normalmap->get_width();
	int uv1 = uv[1] * normalmap->get_height();
	TGAColor c = normalmap->get(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f*2.f - 1.f; //because the normap_map coordinate is -1 ~ +1
	return res;
}

float Model::roughness(vec2 uv) 
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * roughnessmap->get_width();
	int uv1 = uv[1] * roughnessmap->get_height();
	return roughnessmap->get(uv0, uv1)[0] / 255.f;
}

float Model::metalness(vec2 uv) 
{
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * metalnessmap->get_width();
	int uv1 = uv[1] * metalnessmap->get_height();
	return metalnessmap->get(uv0, uv1)[0] / 255.f;
}

float Model::specular(vec2 uv) 
{
	int uv0 = uv[0] * specularmap->get_width();
	int uv1 = uv[1] * specularmap->get_height();
	return specularmap->get(uv0, uv1)[0] / 1.f;
}

float Model::occlusion(vec2 uv) 
{
	if (!occlusion_map)
		return 1;
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * occlusion_map->get_width();
	int uv1 = uv[1] * occlusion_map->get_height();
	return occlusion_map->get(uv0, uv1)[0] / 255.f;
}

vec3 Model::emission(vec2 uv)
{
	if (!emision_map)
		return vec3(0.0f, 0.0f, 0.0f);
	uv[0] = fmod(uv[0], 1);
	uv[1] = fmod(uv[1], 1);
	int uv0 = uv[0] * emision_map->get_width();
	int uv1 = uv[1] * emision_map->get_height();
	TGAColor c = emision_map->get(uv0, uv1);
	vec3 res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = (float)c[i] / 255.f;
	return res;
}


