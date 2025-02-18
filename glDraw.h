#pragma once
#include <iostream>
#include <Windows.h>
#include "geom.h"
#include <gl/GL.h>
#include "offsets.h"

namespace GL {
	bool WorldToScreen(Vector3 pos, Vector3& screen, float matrix[16], int wndWidth, int wndHeight);
	void DrawSnapLine(float x, float y, float lineWidth);
	void SetupOrtho();
	void RestoreGL();
	void DrawBox(ent* entity, Vector3 screenCoords);
	void BuildFont(int height);
	void Print(float x, float y, const GLfloat color[3], const char* format, ...);
	float CenterText(float x, float width, float textWidth);
	void DrawFilledRect(float x, float y, float width, float height, const GLfloat color[3]);
	void DrawOutline(float x, float y, float width, float height, float lineWidth, const GLfloat color[3]);
	void DrawFOV();
}