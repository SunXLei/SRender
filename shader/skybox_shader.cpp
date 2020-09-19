#include "./shader.h"
#include "../core/sample.h"

void SkyboxShader::vertex_shader(int nfaces, int nvertex)
{
	int i = 0;
	vec3 temp = payload.model->vert(nfaces, nvertex);
	vec4 temp_vert = to_vec4(temp, 1.0f);
	vec4 temp_normal = to_vec4(payload.model->normal(nfaces, nvertex), 1.0f);

	payload.uv_attri[nvertex] = payload.model->uv(nfaces, nvertex);
	payload.clipcoord_attri[nvertex] = payload.mvp_matrix * temp_vert;

	//for clipping
	payload.in_uv[nvertex] = payload.uv_attri[nvertex];
	payload.in_clipcoord[nvertex] = payload.clipcoord_attri[nvertex];

	//only model matrix can change normal vector
	for (i = 0; i < 3; i++)
	{
		payload.normal_attri[nvertex][i] = temp_normal[i];
		payload.worldcoord_attri[nvertex][i] = temp_vert[i];
		//for clipping
		payload.in_normal[nvertex][i] = temp_normal[i];
		payload.in_worldcoord[nvertex][i] = temp_vert[i];
	}
}

vec3 SkyboxShader::fragment_shader(float alpha, float beta, float gamma)
{
	vec3 result_color;
	vec4 *clip_coords = payload.clipcoord_attri;
	vec3 *world_coords = payload.worldcoord_attri;

	//interpolate attribute
	float Z = 1.0 / (alpha / clip_coords[0].w() + beta / clip_coords[1].w() + gamma / clip_coords[2].w());
	vec3 worldpos = (alpha*world_coords[0] / clip_coords[0].w() + beta * world_coords[1] / clip_coords[1].w() +
		gamma * world_coords[2] / clip_coords[2].w()) * Z;

	result_color = cubemap_sampling(worldpos, payload.model->environment_map);
	return result_color * 255.f;
}