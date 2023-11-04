#pragma once

struct Vector3
{
	float x, y, z;
};

struct vec4
{
	float w, x, y, z;
};

struct vec2
{
	float x, y;
};


bool WorldToScreen(Vector3 pos, vec2& screen, float matrix[16], int windowWidth, int windowHeight);