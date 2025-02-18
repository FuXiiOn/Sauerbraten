#include "glDraw.h"
#include "config.h"
#include "offsets.h"
#include <iostream>
#include <Windows.h>
#include <numbers>
#include "gl/GL.h"
#pragma comment(lib, "opengl32.lib")

HWND hwnd = FindWindowA(NULL, "Cube 2: Sauerbraten");

GLfloat white[3] = { 255.0f,255.0f,255.0f};

HDC hdc = 0;
unsigned int base = 0;

void GL::BuildFont(int height) {
	hdc = wglGetCurrentDC();
	base = glGenLists(96);
	HFONT hFont = CreateFontA(-height, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, FF_DONTCARE | DEFAULT_PITCH, "Consolas");
	HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
	wglUseFontBitmaps(hdc, 32, 96, base);
	SelectObject(hdc, hFont);
	DeleteObject(hFont);
}

void GL::Print(float x, float y, const GLfloat color[3], const char* format, ...) {
	glColor3f(color[0], color[1], color[2]);
	glRasterPos2f(x, y);

	char text[100];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, 100, format, args);
	va_end(args);

	glPushAttrib(GL_LIST_BIT);
	glListBase(base - 32);
	glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
	glPopAttrib();
}

float GL::CenterText(float x, float width, float textWidth) {
	return x + (width - textWidth) / 2;
}

void GL::DrawFilledRect(float x, float y, float width, float height, const GLfloat color[3]) {
	glBegin(GL_QUADS);
	glColor3f(color[0], color[1], color[2]);
	glVertex2f(x, y);
	glVertex2f(x + width, y);
	glVertex2f(x + width, y + height);
	glVertex2f(x, y + height);
	glEnd();
}

void GL::DrawBox(ent* entity, Vector3 screenCoords) {
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	const GLfloat* color = (*entity->team == *localPlayer->team) ? Config::selectedTeamColor : Config::selectedEnemyColor;
	
	RECT rect;
	GetClientRect(hwnd, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	const int GAME_UNIT_MAGIC = 1800;

	float distance = localPlayer->bodypos.getDistance(entity->bodypos);

	float scale = (GAME_UNIT_MAGIC / distance) * (wndWidth / 1920.0f);
	float width = scale * 3;
	float height = scale * 6;
	float x = screenCoords.x - width / 2;
	float y = screenCoords.y - height / 2;

	GL::DrawOutline(x, y - 4, width, height, 2.0f, color);

	if (Config::bHealthBar) {
		float healthBar = height * (entity->health / 100.0f);

		GL::DrawFilledRect(x - 6, y + (height - healthBar - 5), 3.0f, healthBar, Config::selectedTeamColor);
	}

	if (Config::bNames) {
		float textX = GL::CenterText(x, width, strlen(entity->name) * 9);

		GL::Print(textX, y - 8, color, "%s", entity->name);
	}

	if (Config::bDistance) {
		GL::Print(x + width + 3, y + height, white, "%i", (int)distance);
	}
}

void GL::DrawOutline(float x, float y, float width, float height, float lineWidth, const GLfloat color[3]) {
	glLineWidth(lineWidth);
	glBegin(GL_LINE_STRIP);
	glColor3f(color[0], color[1], color[2]);
	glVertex2f(x - 0.5f, y - 0.5f);
	glVertex2f(x + width + 0.5f, y - 0.5f);
	glVertex2f(x + width + 0.5f, y + height + 0.5f);
	glVertex2f(x - 0.5f, y + height + 0.5f);
	glVertex2f(x - 0.5f, y - 0.5f);
	glEnd();
}

void GL::DrawSnapLine(float x, float y, float lineWidth) {
	RECT rect;
	GetClientRect(hwnd, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glLineWidth(lineWidth);
    glBegin(GL_LINES);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
    glVertex2f(wndWidth / 2, wndHeight / 2); 
    glVertex2f(x, y);
    glEnd();
	glDisable(GL_BLEND);
}

bool GL::WorldToScreen(Vector3 pos, Vector3& screen, float matrix[16], int wndWidth, int wndHeight) {
	Vector4 clipCoords;
	clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
	clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
	clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
	clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

	if (clipCoords.w < 0.1f) return false;

	Vector3 NDC;
	NDC.x = clipCoords.x / clipCoords.w;
	NDC.y = clipCoords.y / clipCoords.w;
	NDC.z = clipCoords.z / clipCoords.w;

	screen.x = (wndWidth / 2 * NDC.x) + (wndWidth / 2);
	screen.y = -(wndHeight / 2 * NDC.y) + (wndHeight / 2);

	return true;
}

void GL::SetupOrtho() {
	RECT rect;
	GetClientRect(hwnd, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	glViewport(0, 0, wndWidth, wndHeight);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, wndWidth, wndHeight, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
}


void GL::RestoreGL() {
	glPopMatrix();
	glPopAttrib();
}

void GL::DrawFOV() {
	RECT rect;
	GetClientRect(hwnd, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	int centerWidth = wndWidth / 2;
	int centerHeight = wndHeight / 2;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBegin(GL_LINE_LOOP);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
	for (int i = 0; i <= 50; i++) {
		float angle = 2.0f * std::numbers::pi * i / 50;
		float x = Config::fovRadius * cos(angle);
		float y = Config::fovRadius * sin(angle);
		glVertex2f(x + centerWidth, y + centerHeight);
	}
	glEnd();
	glDisable(GL_BLEND);
}