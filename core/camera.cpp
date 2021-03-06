#include "./camera.h"

#include "../platform/win32.h"

Camera::Camera(vec3 e, vec3 t, vec3 up, float aspect):
	eye(e),target(t),up(up),aspect(aspect)
{}

Camera::~Camera()
{}

void updata_camera_pos(Camera &camera)
{
	vec3 from_target = camera.eye - camera.target;			// vector point from target to camera's position
	float radius = from_target.norm();

	float phi     = (float)atan2(from_target[0], from_target[2]); // azimuth angle(·½Î»½Ç), angle between from_target and z-axis£¬[-pi, pi]
	float theta   = (float)acos(from_target[1] / radius);		  // zenith angle(Ìì¶¥½Ç), angle between from_target and y-axis, [0, pi]
	float x_delta = window->mouse_info.orbit_delta[0] / window->width;
	float y_delta = window->mouse_info.orbit_delta[1] / window->height;

	// for mouse wheel
	radius *= (float)pow(0.95, window->mouse_info.wheel_delta);

	float factor = 1.5 * PI;
	// for mouse left button
	phi	  += x_delta * factor;
	theta += y_delta * factor;
	if (theta > PI) theta = PI - EPSILON * 100;
	if (theta < 0)  theta = EPSILON * 100;

	camera.eye[0] = camera.target[0] + radius * sin(phi) * sin(theta);
	camera.eye[1] = camera.target[1] + radius * cos(theta);
	camera.eye[2] = camera.target[2] + radius * sin(theta) * cos(phi);

	// for mouse right button
	factor  = radius * (float)tan(60.0 / 360 * PI) * 2.2;
	x_delta = window->mouse_info.fv_delta[0] / window->width;
	y_delta = window->mouse_info.fv_delta[1] / window->height;
	vec3 left = x_delta * factor * camera.x;
	vec3 up   = y_delta * factor * camera.y;

	camera.eye += (left - up);
	camera.target += (left - up);
}

void handle_mouse_events(Camera& camera)
{
	if (window->buttons[0])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.orbit_delta = window->mouse_info.orbit_pos - cur_pos;
		window->mouse_info.orbit_pos = cur_pos;
	}

	if (window->buttons[1])
	{
		vec2 cur_pos = get_mouse_pos();
		window->mouse_info.fv_delta = window->mouse_info.fv_pos - cur_pos;
		window->mouse_info.fv_pos = cur_pos;
	}

	updata_camera_pos(camera);
}

void handle_key_events(Camera& camera)
{
	float distance = (camera.target - camera.eye).norm();

	if (window->keys['W'])
	{
		camera.eye += -10.0 / window->width * camera.z*distance;
	}
	if (window->keys['S'])
	{
		camera.eye += 0.05f*camera.z;
	}
	if (window->keys[VK_UP] || window->keys['Q'])
	{
		camera.eye += 0.05f*camera.y;
		camera.target += 0.05f*camera.y;
	}
	if (window->keys[VK_DOWN] || window->keys['E'])
	{
		camera.eye += -0.05f*camera.y;
		camera.target += -0.05f*camera.y;
	}
	if (window->keys[VK_LEFT] || window->keys['A'])
	{
		camera.eye += -0.05f*camera.x;
		camera.target += -0.05f*camera.x;
	}
	if (window->keys[VK_RIGHT] || window->keys['D'])
	{
		camera.eye += 0.05f*camera.x;
		camera.target += 0.05f*camera.x;
	}
	if (window->keys[VK_ESCAPE])
	{
		window->is_close = 1;
	}
}

void handle_events(Camera& camera)
{
	//calculate camera axis
	camera.z = unit_vector(camera.eye - camera.target);
	camera.x = unit_vector(cross(camera.up, camera.z));
	camera.y = unit_vector(cross(camera.z, camera.x));

	//mouse and keyboard events
	handle_mouse_events(camera);
	handle_key_events(camera);
}