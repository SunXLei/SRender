#include "./pipeline.h"

static SpinLock sp;

static int is_back_facing(vec3 ndc_pos[3])
{
	vec3 a = ndc_pos[0];
	vec3 b = ndc_pos[1];
	vec3 c = ndc_pos[2];
	float signed_area = a.x() * b.y() - a.y() * b.x() +
		b.x() * c.y() - b.y() * c.x() +
		c.x() * a.y() - c.y() * a.x();   //|AB AC|
	return signed_area <= 0;
}

static vec3 compute_barycentric2D(float x, float y, const vec3* v) 
{
	float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
	float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
	return vec3(c1, c2, 1 - c1 - c2);
}

static void set_color(unsigned char* framebuffer, int x, int y, unsigned char color[])
{
	int i;
	int index = ((WINDOW_HEIGHT - y - 1) * WINDOW_WIDTH + x) * 4; // the origin for pixel is bottom-left, but the framebuffer index counts from top-left

	for (i = 0; i < 3; i++)
		framebuffer[index + i] = color[i];
}

static int is_inside_triangle(float alpha, float beta, float gamma)
{
	int flag = 0;
	// here epsilon is to alleviate precision bug
	if (alpha > -EPSILON && beta > -EPSILON && gamma > -EPSILON)
		flag = 1;

	return flag;
}

static int get_index(int x, int y)
{
	return (WINDOW_HEIGHT - y - 1) * WINDOW_WIDTH + x;
}


typedef enum 
{
	W_PLANE,
	X_RIGHT,
	X_LEFT,
	Y_TOP,
	Y_BOTTOM,
	Z_NEAR,
	Z_FAR
} clip_plane;

// in my implementation, all the w is negative, so here is a little different from openGL
static int is_inside_plane(clip_plane c_plane,vec4 vertex)
{
	switch (c_plane)
	{
		case W_PLANE:
			return vertex.w() <= -EPSILON;
		case X_RIGHT:
			return vertex.x() >= vertex.w();
		case X_LEFT:
			return vertex.x() <= -vertex.w();
		case Y_TOP:
			return vertex.y() >= vertex.w();
		case Y_BOTTOM:
			return vertex.y() <= -vertex.w();
		case Z_NEAR:
			return vertex.z() >= vertex.w();
		case Z_FAR:
			return vertex.z() <= -vertex.w();
		default:
			return 0;
	}
}

// for the deduction of intersection ratio
// refer to: https://fabiensanglard.net/polygon_codec/clippingdocument/Clipping.pdf
static float get_intersect_ratio(vec4 prev, vec4 curv,clip_plane c_plane)
{
	switch (c_plane) 
	{
		case W_PLANE:
			return (prev.w() + EPSILON) / (prev.w() - curv.w());
		case X_RIGHT:
			return (prev.w() - prev.x()) / ((prev.w() - prev.x()) - (curv.w() - curv.x()));
		case X_LEFT:
			return (prev.w() + prev.x()) / ((prev.w() + prev.x()) - (curv.w() + curv.x()));
		case Y_TOP:
			return (prev.w() - prev.y()) / ((prev.w() - prev.y()) - (curv.w() - curv.y()));
		case Y_BOTTOM:
			return (prev.w() + prev.y()) / ((prev.w() + prev.y()) - (curv.w() + curv.y()));
		case Z_NEAR:
			return (prev.w() - prev.z()) / ((prev.w() - prev.z()) - (curv.w() - curv.z()));
		case Z_FAR:
			return (prev.w() + prev.z()) / ((prev.w() + prev.z()) - (curv.w() + curv.z()));
		default:
			return 0;
	}
}

static int clip_with_plane(clip_plane c_plane, int num_vert, payload_t &payload)
{
	int out_vert_num = 0;
	int previous_index, current_index;
	int is_odd = (c_plane + 1) % 2;
	
	// set the right in and out datas
	vec4* in_clipcoord	 = is_odd ? payload.in_clipcoord: payload.out_clipcoord;
	vec3* in_worldcoord  = is_odd ? payload.in_worldcoord: payload.out_worldcoord;
	vec3* in_normal		 = is_odd ? payload.in_normal: payload.out_normal;
	vec2* in_uv			 = is_odd ? payload.in_uv: payload.out_uv;
	vec4* out_clipcoord  = is_odd ? payload.out_clipcoord: payload.in_clipcoord;
	vec3* out_worldcoord = is_odd ? payload.out_worldcoord:payload.in_worldcoord;
	vec3* out_normal	 = is_odd ? payload.out_normal: payload.in_normal;
	vec2* out_uv		 = is_odd ? payload.out_uv: payload.in_uv;

	// tranverse all the edges from first vertex
	for (int i = 0; i < num_vert; i++)
	{
		current_index   = i;
		previous_index  = (i - 1 + num_vert) % num_vert;
		vec4 cur_vertex = in_clipcoord[current_index];
		vec4 pre_vertex = in_clipcoord[previous_index];

		int is_cur_inside = is_inside_plane(c_plane, cur_vertex);
		int is_pre_inside = is_inside_plane(c_plane, pre_vertex);
		if (is_cur_inside != is_pre_inside)
		{
			float ratio = get_intersect_ratio(pre_vertex,cur_vertex,c_plane);

			out_clipcoord[out_vert_num]  = vec4_lerp(pre_vertex,cur_vertex,ratio);
			out_worldcoord[out_vert_num] = vec3_lerp(in_worldcoord[previous_index],in_worldcoord[current_index],ratio);
			out_normal[out_vert_num]     = vec3_lerp(in_normal[previous_index],in_normal[current_index],ratio);
			out_uv[out_vert_num]         = vec2_lerp(in_uv[previous_index],in_uv[current_index],ratio);

			out_vert_num++;
		}

		if (is_cur_inside)
		{
			out_clipcoord[out_vert_num]  = cur_vertex;
			out_worldcoord[out_vert_num] = in_worldcoord[current_index];
			out_normal[out_vert_num]     = in_normal[current_index];
			out_uv[out_vert_num]         = in_uv[current_index];

			out_vert_num++;
		}
	}

	return out_vert_num;
}

static int homo_clipping(payload_t &payload)
{
	int num_vertex = 3;
	num_vertex = clip_with_plane(W_PLANE, num_vertex, payload);
	num_vertex = clip_with_plane(X_RIGHT, num_vertex, payload);
	num_vertex = clip_with_plane(X_LEFT, num_vertex, payload);
	num_vertex = clip_with_plane(Y_TOP, num_vertex, payload);
	num_vertex = clip_with_plane(Y_BOTTOM, num_vertex, payload);
	num_vertex = clip_with_plane(Z_NEAR, num_vertex, payload);
	num_vertex = clip_with_plane(Z_FAR, num_vertex, payload);
	return num_vertex;
}

static void transform_attri(payload_t &payload,int index0,int index1,int index2)
{
	payload.clipcoord_attri[0]	= payload.out_clipcoord[index0];
	payload.clipcoord_attri[1]	= payload.out_clipcoord[index1];
	payload.clipcoord_attri[2]	= payload.out_clipcoord[index2];
	payload.worldcoord_attri[0] = payload.out_worldcoord[index0];
	payload.worldcoord_attri[1] = payload.out_worldcoord[index1];
	payload.worldcoord_attri[2] = payload.out_worldcoord[index2];
	payload.normal_attri[0]		= payload.out_normal[index0];
	payload.normal_attri[1]		= payload.out_normal[index1];
	payload.normal_attri[2]		= payload.out_normal[index2];
	payload.uv_attri[0]			= payload.out_uv[index0];
	payload.uv_attri[1]			= payload.out_uv[index1];
	payload.uv_attri[2]			= payload.out_uv[index2];
}
//drawline
//void drawline(int x0, int y0, int x1, int y1, unsigned char* framebuffer)
//{
//	int dx = x1 - x0;
//	int dy = y1 - y0;
//
//	int dx1 = fabs(dx);
//	int dy1 = fabs(dy);
//	int px = 2 * dy1;
//	int py = 2 * dx1;
//
//	//不陡
//	if (dx1 > dy1)
//	{
//		if (x0 > x1)
//		{
//			int tempX = x0, tempY = y0;
//			x0 = x1; y0 = y1;
//			x1 = tempX, y1 = tempY;
//		}
//
//		int y = y0;
//		for (int x = x0; x <= x1; x++)
//		{
//			//image.set(x, y, white);
//			set_color(framebuffer, x, y, white1);
//
//			//如果超过了半格，提前减一，这是正确的，好好思考
//			if (px > dx1)
//			{
//				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
//				{
//					y += 1;
//				}
//				else
//				{
//					y -= 1;
//				}
//				px += dy1 * 2; // +k
//				px -= 2 * dx1; // +1
//			}
//			else
//			{
//				px += dy1 * 2;// +k
//			}
//		}
//
//	}
//	//陡
//	else
//	{
//		if (y0 > y1)
//		{
//			int tempX = x0, tempY = y0;
//			x0 = x1; y0 = y1;
//			x1 = tempX, y1 = tempY;
//		}
//
//		int x = x0;
//		for (int y = y0; y <= y1; y++)
//		{
//			//image.set(x, y, white);
//
//			set_color(framebuffer, x, y, white1);
//			//如果超过了半格，提前减一，这是正确的，好好思考
//			if (py > dy1)
//			{
//				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
//				{
//					x += 1;
//				}
//				else
//				{
//					x -= 1;
//				}
//				py += dx1 * 2;
//				py -= 2 * dy1;
//			}
//			else
//			{
//				py += dx1 * 2;
//			}
//		}
//	}
//}

/*
void rasterize_wireframe(vector<vec3> &triangle, TGAImage &image)
{
	mat4 mvp = perspective * view;

	vec4 A = to_vec4(triangle[0], 1);
	vec4 B = to_vec4(triangle[1], 1);
	vec4 C = to_vec4(triangle[2], 1);

	vec4 v[] = {
				mvp * A,
				mvp * B,
				mvp * C
	};

	int flag = 0;
	//Homogeneous division
	for (auto& vec : v) {
		if (vec.w() > -1)
			flag = 1;
		vec[0] /= vec.w();
		vec[1] /= vec.w();
		vec[2] /= vec.w();
	}

	if (flag)
		return;

	for (auto & vert : v)
	{
		vert[0] = 0.5*width*(vert.x() + 1.0);
		vert[1] = 0.5*height*(vert.y() + 1.0);
		vert[2] = -vert.z();
		//std::cout << vert << endl;

		if (vert[0] < 0 || vert[0]>width || vert[1]<0 || vert[1]>height)
			flag = 1;
	}
	if (flag)
		return;


	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0; i < 3; i++) {
		xmin = float_min(xmin, v[i][0]);
		xmax = float_max(xmax, v[i][0]);
		ymin = float_min(ymin, v[i][1]);
		ymax = float_max(ymax, v[i][1]);
	}
	//光栅化
	//for (int x = (int)xmin; x <= (int)xmax; x++)
	//{
	//	for (int y = (int)ymin; y <= (int)ymax; y++)
	//	{
	//		vec3 bc_screen = computeBarycentric2D(x + 0.5, y + 0.5, v);
	//		if (bc_screen[0] < 0 || bc_screen[1] < 0 || bc_screen[2] < 0) continue;
	//		image.set(x, y, green);
	//	}

	//}

	//线框
	drawline(v[0][0], v[0][1], v[1][0], v[1][1], image, white);
	drawline(v[1][0], v[1][1], v[2][0], v[2][1], image, white);
	drawline(v[2][0], v[2][1], v[0][0], v[0][1], image, white);

}
*/

void rasterize_singlethread(vec4 *clipcoord_attri, unsigned char *framebuffer, float *zbuffer, IShader &shader)
{
	vec3 ndc_pos[3];
	vec3 screen_pos[3];
	unsigned char c[3];
	int width  = window->width;
	int height = window->height;
	int is_skybox = shader.payload.model->is_skybox;

	// homogeneous division
	for (int i = 0; i < 3; i++)
	{
		ndc_pos[i][0] = clipcoord_attri[i][0] / clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] / clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] / clipcoord_attri[i].w();
	}

	// viewport transformation
	for (int i = 0; i < 3; i++)
	{
		screen_pos[i][0] = 0.5*(width-1)*(ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5*(height-1)*(ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = is_skybox ? 1000:-clipcoord_attri[i].w();	//view space z-value
	}

	// backface clip (skybox didnit need it)
	if (!is_skybox)
	{
		if (is_back_facing(ndc_pos))
			return;
	}

	// get bounding box
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (int i = 0; i < 3; i++) 
	{
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	// rasterization
	for (int x = (int)xmin; x <= (int)xmax; x++)
	{
		for (int y = (int)ymin; y <= (int)ymax; y++)
		{
			vec3 barycentric = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = barycentric.x(); float beta = barycentric.y(); float gamma = barycentric.z();

			if (is_inside_triangle(alpha, beta, gamma))
			{
				int index = get_index(x, y);
				//interpolation correct term
				float normalizer = 1.0 / (alpha / clipcoord_attri[0].w() + beta / clipcoord_attri[1].w() + gamma / clipcoord_attri[2].w());
				//for larger z means away from camera, needs to interpolate z-value as a property			
				float z = (alpha * screen_pos[0].z() / clipcoord_attri[0].w() + beta * screen_pos[1].z() / clipcoord_attri[1].w() +
					gamma * screen_pos[2].z() / clipcoord_attri[2].w()) * normalizer;

				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;
					vec3 color = shader.fragment_shader(alpha, beta, gamma);

					//clamp color value
					for (int i = 0; i < 3; i++)
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
					}
					set_color(framebuffer, x, y, c);
				}
			}
		}
	}
}

/*
void rasterize_multithread(vec4 *clipcoord_attri, unsigned char* framebuffer, float *zbuffer, IShader& shader)
{
	vec3 ndc_pos[3];
	vec3 screen_pos[3];
	unsigned char c[3];
	int width = window->width;
	int height = window->height;
	int i = 0, j = 0, flag = 0;

	//Homogeneous division
	for (i = 0; i < 3; i++)
	{
		//simple clip
		if (clipcoord_attri[i].w() > -0.1f)
			flag = 1;

		ndc_pos[i][0] = clipcoord_attri[i][0] /= clipcoord_attri[i].w();
		ndc_pos[i][1] = clipcoord_attri[i][1] /= clipcoord_attri[i].w();
		ndc_pos[i][2] = clipcoord_attri[i][2] /= clipcoord_attri[i].w();
	}

	//viewport transform
	for (i = 0; i < 3; i++)
	{
		screen_pos[i][0] = 0.5*width*(ndc_pos[i][0] + 1.0);
		screen_pos[i][1] = 0.5*height*(ndc_pos[i][1] + 1.0);
		screen_pos[i][2] = -clipcoord_attri[i].w();

		//simple clip
		if (screen_pos[i][0] < 0 || screen_pos[i][0]>width
			|| screen_pos[i][1]<0 || screen_pos[i][1]>height)
			flag = 1;
	}

	//check clip condition
	if (flag)
		return;

	//backface clip
	if (is_back_facing(ndc_pos))
		return;

	//bounding box
	float xmin = 10000, xmax = -10000, ymin = 10000, ymax = -10000;
	for (i = 0; i < 3; i++) {
		xmin = float_min(xmin, screen_pos[i][0]);
		xmax = float_max(xmax, screen_pos[i][0]);
		ymin = float_min(ymin, screen_pos[i][1]);
		ymax = float_max(ymax, screen_pos[i][1]);
	}

	//rasterization
	for (int x = (int)xmin; x <= (int)xmax; x++)
	{
		for (int y = (int)ymin; y <= (int)ymax; y++)
		{
			vec3 result = compute_barycentric2D((float)(x + 0.5), (float)(y + 0.5), screen_pos);
			float alpha = result.x(); float beta = result.y(); float gamma = result.z();

			if (is_inside_triangle(alpha, beta, gamma))
			{
				int index = get_index(x, y);
				//interpolation correct term
				float normalizer = 1.0 / (alpha / clipcoord_attri[0].w() + beta / clipcoord_attri[1].w() + gamma / clipcoord_attri[2].w());
				//for larger z means away from camera, needs to interpolate z-value as a property
				float z = (alpha*screen_pos[0].z() / clipcoord_attri[0].w() + beta * screen_pos[1].z() / clipcoord_attri[1].w() +
					gamma * screen_pos[2].z() / clipcoord_attri[2].w()) * normalizer;

				if (zbuffer[index] > z)
				{
					zbuffer[index] = z;
					vec3 color = shader.fragment_shader(alpha, beta, gamma);

					//clamp color value
					for (i = 0; i < 3; i++)
					{
						c[i] = (int)float_clamp(color[i], 0, 255);
						//c[i] = 200;
					}

					//use spainlock to check whether really need to write the color into buffer and if needed, rewrite the z-value also.
					//（this is for prevent writing conflict）
					sp.lock();
					if (zbuffer[index] > z - EPSILON)
					{
						zbuffer[index] = z;
						set_color(framebuffer, x, y, c);
					}
					sp.unlock();
				}
			}
		}
	}
}
*/

void draw_triangles(unsigned char *framebuffer, float *zbuffer, IShader &shader, int nface)
{
	// vertex shader
	for (int i = 0; i < 3; i++)
	{
		shader.vertex_shader(nface, i);
	}

	// homogeneous clipping
	int num_vertex = homo_clipping(shader.payload);

	// triangle assembly and reaterize
	for (int i = 0; i < num_vertex - 2; i++) {
		int index0 = 0;
		int index1 = i + 1;
		int index2 = i + 2;
		// transform data to real vertex attri
		transform_attri(shader.payload, index0, index1, index2);

		rasterize_singlethread(shader.payload.clipcoord_attri, framebuffer,zbuffer,shader);
	}
}
