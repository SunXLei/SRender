#pragma once
#include <cmath>
#include <iostream>

#include "./macro.h"

class vec2 {
public:
	vec2();
	vec2(float e0, float e1);

	float x() const;
	float y() const;
	float& operator[](int i);
	float operator[](int i) const;

	vec2 operator-() const;
	vec2& operator+=(const vec2& v);
	vec2& operator*=(const float t);
	vec2& operator/=(const float t);

	float norm() const;
	float norm_squared() const;

public:
	float e[2];
};

class vec3 {
public:
	vec3();
	vec3(float e0, float e1, float e2);

	float x() const;
	float y() const;
	float z() const;
	float& operator[](int i);
	float operator[](int i) const;

	vec3 operator-() const;
	vec3& operator+=(const vec3& v);
	vec3& operator*=(const float t);
	vec3& operator/=(const float t);

	float norm() const;
	float norm_squared() const;

public:
	float e[3];
};

class vec4 {
public:
	vec4();
	vec4(float e0, float e1, float e2, float e3);

	float x() const;
	float y() const;
	float z() const;
	float w() const;
	float& operator[](int i);
	float operator[](int i) const;

	vec4& operator*=(const float t);
	vec4& operator/=(const float t);

public:
	float e[4];
};

class mat3 {
public:
	mat3();

	vec3& operator[](int i);
	vec3 operator[](int i) const;

	mat3 transpose() const;
	mat3 inverse() const;
	mat3 inverse_transpose() const;
	static mat3 identity();

public:
	vec3 rows[3];
};

class mat4 {
public:
	mat4();

	vec4& operator[](int i);
	vec4 operator[](int i) const;

	mat4 transpose() const;
	mat4 inverse() const;
	mat4 inverse_transpose() const;
	static mat4 identity();

public:
	vec4 rows[4];
};

/* vec2 related functions */
std::ostream& operator<<(std::ostream& out, const vec2& v);
vec2 operator+(const vec2& u, const vec2& v);
vec2 operator-(const vec2& u, const vec2& v);
vec2 operator*(const vec2& u, const vec2& v);
vec2 operator*(double t, const vec2& v);
vec2 operator*(const vec2& v, double t);
vec2 operator/(vec2 v, double t);

/* vec3 related functions */
std::ostream& operator<<(std::ostream& out, const vec3& v);
vec3 operator+(const vec3& u, const vec3& v);
vec3 operator-(const vec3& u, const vec3& v);
vec3 operator*(const vec3& u, const vec3& v);
vec3 operator*(double t, const vec3& v);
vec3 operator*(const vec3& v, double t);
vec3 operator/(vec3 v, double t);
double dot(const vec3& u, const vec3& v);
vec3 cross(const vec3& u, const vec3& v);
vec3 unit_vector(const vec3& v);
vec3 cwise_product(const vec3& a, const vec3& b);

/* vec4 related functions */
std::ostream& operator<<(std::ostream& out, const vec4& v);
vec4 to_vec4(const vec3& u, float w);
vec4 operator-(const vec4& u, const vec4& v);
vec4 operator+(const vec4& u, const vec4& v);
vec4 operator*(double t, const vec4& v);
vec4 operator*(const vec4& v, double t);

/* mat related functions */
std::ostream& operator<<(std::ostream& out, const mat3& m);
std::ostream& operator<<(std::ostream& out, const mat4& m);
vec4 operator*(const mat4& m, const vec4 v);
mat4 operator*(const mat4& m1, const mat4& m2);

/* transformation related functions */
mat4 mat4_translate(float tx, float ty, float tz);
mat4 mat4_scale(float sx, float sy, float sz);
mat4 mat4_rotate_x(float angle);
mat4 mat4_rotate_y(float angle);
mat4 mat4_rotate_z(float angle);
mat4 mat4_lookat(vec3 eye, vec3 target, vec3 up);
mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far);
mat4 mat4_perspective(float fovy, float aspect, float near, float far);

/* untility functions */
float float_max(float a, float b);
float float_min(float a, float b);
float float_clamp(float f, float min, float max);
float float_lerp(float start, float end, float alpha);
vec2 vec2_lerp(vec2& start, vec2& end, float alpha);
vec3 vec3_lerp(vec3& start, vec3& end, float alpha);
vec4 vec4_lerp(vec4& start, vec4& end, float alpha);
