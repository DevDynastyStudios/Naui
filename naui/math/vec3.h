#pragma once
#include <math.h>

typedef struct
{
	float x, y, z;
} Naui_Vec3;

static inline Naui_Vec3 naui_vec3_add(const Naui_Vec3 a, const Naui_Vec3 b)
{
	return (Naui_Vec3){ a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline Naui_Vec3 naui_vec3_sub(const Naui_Vec3 a, const Naui_Vec3 b)
{
	return (Naui_Vec3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

static inline Naui_Vec3 naui_vec3_scale(const Naui_Vec3 v, float s)
{
	return (Naui_Vec3){ v.x * s, v.y * s, v.z * s };
}

static inline Naui_Vec3 naui_vec3_abs(const Naui_Vec3 v)
{
	return (Naui_Vec3){ fabsf(v.x), fabsf(v.y), fabsf(v.z) };
}

static inline Naui_Vec3 naui_vec3_negate(const Naui_Vec3 v)
{
	return (Naui_Vec3){ -v.x, -v.y, -v.z };
}

static inline Naui_Vec3 naui_vec3_min(const Naui_Vec3 a, const Naui_Vec3 b)
{
	return (Naui_Vec3){ fminf(a.x, b.x), fminf(a.y, b.y), fminf(a.z, b.z) };
}

static inline Naui_Vec3 naui_vec3_max(const Naui_Vec3 a, const Naui_Vec3 b)
{
	return (Naui_Vec3){ fmaxf(a.x, b.x), fmaxf(a.y, b.y), fmaxf(a.z, b.z) };
}

static inline Naui_Vec3 naui_vec3_lerp(const Naui_Vec3 a, const Naui_Vec3 b, float t)
{
	return (Naui_Vec3)
	{
		a.x + t * (b.x - a.x),
		a.y + t * (b.y - a.y),
		a.z + t * (b.z - a.z)
	};
}

static inline float naui_vec3_length(const Naui_Vec3 v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline float naui_vec3_distance(const Naui_Vec3 a, const Naui_Vec3 b)
{
	float dx = b.x - a.x;
	float dy = b.y - a.y;
	float dz = b.z - a.z;
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

static inline float naui_vec3_dot(const Naui_Vec3 a, const Naui_Vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}