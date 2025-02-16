#include <iostream>
#include <Windows.h>
#include "mem.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_opengl2.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "config.h"
#include "offsets.h"
#include "glDraw.h"
#include "numbers"
#include <intrin.h>

typedef BOOL(__stdcall* hooked_wglSwapBuffers)(HDC hdc);
hooked_wglSwapBuffers o_wglSwapBuffers;

typedef int(__fastcall* SDL_SetCursor)(int mode);
SDL_SetCursor SDL_setCursor;

typedef int(__fastcall* SDL_ShowCursor)(int mode);
SDL_ShowCursor SDL_showCursor;

typedef __int64(__fastcall* Shoot)(__int64 entity, Vector3* a2);
Shoot shootHook;

typedef __int64(__fastcall* TriggerBot)(__int64 a1, __int64 a2, __int64 a3, float* a4);
TriggerBot triggerBotHook;

typedef float(__fastcall* VisCheck)(Vector3 a1, Vector3 a2, float a3, int a4, int a5, int a6);

BYTE oSwapBuffersBytes[15] = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x40 };
BYTE oTraceLineBytes[18] = { 0x48, 0x89, 0x54, 0x24, 0x10, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x41, 0x57, 0x48, 0x8D, 0x6C, 0x24, 0xC8 };
BYTE oShootBytes[17] = { 0x40, 0x55, 0x53, 0x41, 0x56, 0x48, 0x8D, 0x6C, 0x24, 0xD0, 0x48, 0x81, 0xEC, 0x30, 0x01, 0x00, 0x00 };
BYTE oTriggerBotBytes[15] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x53, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56 };

HWND gameHWND = FindWindowA(NULL, "Cube 2: Sauerbraten");

extern void executeFeatures();

extern "C" void oneHit();
extern "C" uintptr_t jmpBackOneHit = 0;
extern "C" bool bOnehit = false;

static void HelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::BeginItemTooltip())
	{
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

ent* closestSilent = nullptr;

bool isVisible(ent* localPlayer, ent* enemy) {
	if (!localPlayer || !enemy) return false;

	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	VisCheck oVisCheck = (VisCheck)(moduleBase + 0x1142C0);

	Vector3 ray = enemy->bodypos - localPlayer->bodypos;
	float distance = sqrt(ray.x * ray.x + ray.y * ray.y + ray.z * ray.z);

	if (distance > 0.1f) {
		ray.x /= distance;
		ray.y /= distance;
		ray.z /= distance;
	}

	float result = oVisCheck(localPlayer->bodypos, ray, distance, 0, 0, 0);

	if (result >= distance) {
		return true;
	}

	return false;
}

__int64 __fastcall silentFunct(__int64 entity, Vector3* enemy) {
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	uintptr_t entList = *(uintptr_t*)(moduleBase + 0x346C90);
	int* currPlayers = (int*)(moduleBase + 0x346C9C);
	float* viewMatrix = (float*)mem::FindDMAAddy(moduleBase + 0x34B770, { 0x8 });
	float closestDistance = FLT_MAX;
	ent* closestEntity = nullptr;
	ent* bestSilent = nullptr;
	float closestFov = FLT_MAX;
	RECT rect;
	GetClientRect(gameHWND, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;
	float fovDegrees = 1.5f * atan(Config::fovRadius / (wndWidth / 2)) * (180.0f / std::numbers::pi);

	if (entity == (uintptr_t)localPlayer && Config::bSilent) {

		for (unsigned int i = 0; i < *currPlayers; i++) {
			ent* currEntity = *reinterpret_cast<ent**>(entList + i * 8);

			if (!currEntity) continue;
			if (currEntity == localPlayer) continue;
			if (currEntity->health < 0 || currEntity->health > 100) continue;
			if (*currEntity->team == *localPlayer->team) continue;
			if (Config::bVisCheck && !isVisible(localPlayer, currEntity)) continue;

			float abspos_x = localPlayer->bodypos.x - currEntity->bodypos.x;
			float abspos_y = localPlayer->bodypos.y - currEntity->bodypos.y;
			float abspos_z = localPlayer->bodypos.z - currEntity->bodypos.z;

			float distance = sqrt(abspos_x * abspos_x + abspos_y * abspos_y + abspos_z * abspos_z);

			float azimuth_xy = atan2f(abspos_y, abspos_x);
			float newYaw = azimuth_xy * (180.0 / std::numbers::pi);

			if (newYaw > 180.0f) newYaw -= 360.0f;
			if (newYaw < -180.0f) newYaw += 360.0f;

			float azimuth_z = atan2f(-abspos_z, hypot(abspos_x, abspos_y));
			float newPitch = azimuth_z * (180.0 / std::numbers::pi);

			float yawDiff = newYaw + 90.0f - localPlayer->yaw;
			if (yawDiff > 180.0f) yawDiff -= 360.0f;
			if (yawDiff < -180.0f) yawDiff += 360.0f;

			float pitchDiff = newPitch - localPlayer->pitch;

			float fov = sqrt(yawDiff * yawDiff + pitchDiff * pitchDiff);

			if (Config::bFov) {
				if (fov < closestFov && fov <= fovDegrees) {
					closestEntity = currEntity;
					bestSilent = currEntity;
					closestFov = fov;
				}
			}
			else {
				if (distance < closestDistance) {
					closestEntity = currEntity;
					bestSilent = currEntity;
					closestDistance = distance;
				}
			}
		}

		if (bestSilent) {
			closestSilent = bestSilent;
		}
		else {
			closestSilent = nullptr;
		}

		if (closestEntity) {

			int randValue = rand() % 100;

			if (randValue < Config::hitChance) {
				*enemy = closestEntity->bodypos;
				localPlayer->bodypos = (*enemy - Vector3(0.1f, 0.1f, 0.1f));

				return shootHook(entity, enemy);
			}
			else {
				return shootHook(entity, enemy);
			}
		}
	}

	return shootHook(entity, enemy);
}

__int64 __fastcall hkTriggerBot(__int64 a1, __int64 a2, __int64 a3, float* a4) {
	__int64 result = triggerBotHook(a1, a2, a3, a4);

	if (Config::bTriggerbot) {
		uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
		ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);

		if (result) {
			localPlayer->isShooting = true;
		}
		else if (!GetAsyncKeyState(VK_LBUTTON)) {
			localPlayer->isShooting = false;
		}
	}

	return triggerBotHook(a1, a2, a3, a4);
}

BOOL __stdcall hook_wglSwapBuffers(HDC hdc) {
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	uintptr_t sdlBase = (uintptr_t)GetModuleHandle(L"SDL2.dll");
	uintptr_t entList = *(uintptr_t*)(moduleBase + 0x346C90);
	float* viewMatrix = (float*)mem::FindDMAAddy(moduleBase + 0x34B770, { 0x8 });
	int* gameState = (int*)(moduleBase + 0x345C50);

	RECT rect;
	GetClientRect(gameHWND, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	static HGLRC g_gameContext;
	static HGLRC g_myContext;
	static bool g_contextCreated = false;
	g_gameContext = wglGetCurrentContext();

	if (!g_contextCreated) {
		g_myContext = wglCreateContext(hdc);
		wglMakeCurrent(hdc, g_myContext);
		GL::BuildFont(14);
		g_contextCreated = true;
	}

	wglMakeCurrent(hdc, g_myContext);

	GL::SetupOrtho();

	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (Config::bFov) {
		GL::DrawFOV();
	}

	if (Config::bSilent && Config::bSnapLine && *gameState == 0) {
		if (closestSilent) {
			Vector3 screenCoords;

			if (GL::WorldToScreen(closestSilent->bodypos, screenCoords, viewMatrix, wndWidth, wndHeight)) {
				GL::DrawSnapLine(screenCoords.x, screenCoords.y, 2.0f);
			}
		}
	}

	if (Config::bEsp && *gameState == 0) {
		int* currPlayers = (int*)(moduleBase + 0x346C9C);
		Vector3 screenCoords;
		const float PLAYER_HEIGHT = 7.25f;
		const float EYE_HEIGHT = 10.5f;

		for (int i = 0; i < *currPlayers; i++) {
			ent* entity = *reinterpret_cast<ent**>(entList + i *8);

			if (entity == localPlayer) continue;
			if (entity->health < 0 || entity->health > 100) continue;

			Vector3 newHead = entity->bodypos;
			newHead.z = entity->bodypos.z - EYE_HEIGHT + PLAYER_HEIGHT / 2;

			if (GL::WorldToScreen(newHead, screenCoords, viewMatrix, wndWidth, wndHeight)) {
				GL::DrawBox(entity, screenCoords);
			}
		}
	}

	ImGui::SetNextWindowSize(ImVec2(400, 250));

	if (Config::showMenu) {
		SDL_setCursor(0);
		SDL_showCursor(1);
		ImGui::Begin("Sauerbraten - ExploitCore", &Config::showMenu, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
		ImGui::BeginTabBar("cheat");
		if (ImGui::BeginTabItem("Aimbot")) {
			ImGui::Checkbox("Aimbot", &Config::bAimbot);
			if (Config::bAimbot) {
				ImGui::SliderFloat("Smoothing", &Config::aimSmooth, 0.0f, 1.0f, "%.3f");
			}
			ImGui::Checkbox("Silent Aim", &Config::bSilent);
			if (Config::bSilent) {
				ImGui::SliderInt("Hitchance", &Config::hitChance, 0, 100, "%d%%");
				ImGui::Checkbox("Snap Lines", &Config::bSnapLine);
			}
			ImGui::Checkbox("FOV Radius", &Config::bFov);
			if (Config::bFov) {
				ImGui::SliderFloat("Radius", &Config::fovRadius, 25.0f, 500.0f, "%.3f");
			}
			ImGui::Checkbox("Visible Check", &Config::bVisCheck);
			ImGui::Checkbox("Triggerbot", &Config::bTriggerbot);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("ESP")) {
			ImGui::Checkbox("ESP", &Config::bEsp);
			if (Config::bEsp) {
				ImGui::Checkbox("Draw HealthBar", &Config::bHealthBar);
				ImGui::Checkbox("Draw Names", &Config::bNames);
				ImGui::Checkbox("Draw Distance", &Config::bDistance);
			}
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Misc")) {
			ImGui::Checkbox("No recoil", &Config::bKnockback);
			ImGui::Checkbox("Thirdperson", &Config::bThirdPerson);
			ImGui::Checkbox("Bunnyhop", &Config::bBunnyHop);
			ImGui::Checkbox("Godmode", &Config::bGodmode);
			ImGui::SameLine(); HelpMarker("WORKS ONLY ON SELF-HOSTED SERVERS");
			ImGui::Checkbox("Infinite ammo", &Config::bInfAmmo);
			ImGui::SameLine(); HelpMarker("WORKS ONLY ON SELF-HOSTED SERVERS");
			ImGui::Checkbox("Rapid fire", &Config::bRapidFire);
			ImGui::SameLine(); HelpMarker("WORKS ONLY ON SELF-HOSTED SERVERS");
			ImGui::Checkbox("One hit", &Config::bOnehit);
			ImGui::SameLine(); HelpMarker("WORKS ONLY ON SELF-HOSTED SERVERS");
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
		ImGui::End();
	}
	else if (GetForegroundWindow() == gameHWND && !Config::showMenu) {
		SDL_setCursor(1);
		SDL_showCursor(1);
	}
	else {
		SDL_showCursor(1);
	}

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

	wglMakeCurrent(hdc, g_gameContext);

	GL::RestoreGL();

	return o_wglSwapBuffers(hdc);
}

extern LRESULT WINAPI ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

WNDPROC	oWndProc;
LRESULT __stdcall WndProc(const HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if (!Config::showMenu) {
		return CallWindowProc(oWndProc, hWnd, msg, wParam, lParam);
	}
	else {
		ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
		return true;
	}
}

BOOL WINAPI HackThread(HMODULE hModule) {
	AllocConsole();
	FILE* file;
	freopen_s(&file, "CONOUT$", "w", stdout);
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	uintptr_t wglSwapBuffers = (uintptr_t)GetProcAddress(GetModuleHandle(L"opengl32.dll"), "wglSwapBuffers");

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui_ImplWin32_Init(gameHWND);
	ImGui_ImplOpenGL2_Init();

	SDL_setCursor = (SDL_SetCursor)GetProcAddress(GetModuleHandle(L"SDL2.dll"), "SDL_SetRelativeMouseMode");
	SDL_showCursor = (SDL_ShowCursor)GetProcAddress(GetModuleHandle(L"SDL2.dll"), "SDL_ShowCursor");
	o_wglSwapBuffers = (hooked_wglSwapBuffers)wglSwapBuffers;
	o_wglSwapBuffers = (hooked_wglSwapBuffers)mem::TrampHook((BYTE*)o_wglSwapBuffers, (BYTE*)hook_wglSwapBuffers, 15);
	shootHook = (Shoot)mem::TrampHook((BYTE*)(moduleBase + 0x1DB4C0), (BYTE*)silentFunct, 17);
	triggerBotHook = (TriggerBot)mem::TrampHook((BYTE*)(moduleBase + 0x1DB2A0), (BYTE*)hkTriggerBot, 15);
	mem::Hook((BYTE*)(moduleBase + 0x1ECB97), (BYTE*)oneHit, 15);
	jmpBackOneHit = (moduleBase + 0x1ECB97 + 14);

	oWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(gameHWND, GWLP_WNDPROC, (LONG_PTR)WndProc));

	while (!GetAsyncKeyState(VK_END) & 1) {
		bOnehit = Config::bOnehit;

		if (GetAsyncKeyState(VK_INSERT) & 1) {
			Config::showMenu = !Config::showMenu;
		}

		executeFeatures();

		Sleep(10);
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	VirtualFree(o_wglSwapBuffers, 0, MEM_RELEASE);
	VirtualFree(shootHook, 0, MEM_RELEASE);
	VirtualFree(triggerBotHook, 0, MEM_RELEASE);

	mem::Patch((BYTE*)wglSwapBuffers, oSwapBuffersBytes, 15);
	mem::Patch((BYTE*)(moduleBase + 0x1D5310), oTraceLineBytes, 18);
	mem::Patch((BYTE*)(moduleBase + 0x1DB2A0), oTriggerBotBytes, 15);
	mem::Patch((BYTE*)(moduleBase + 0x1DB4C0), oShootBytes, 17);

	FreeConsole();
	fclose(file);
	FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	{
		HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, 0);
		if (hThread) {
			CloseHandle(hThread);
		}
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

