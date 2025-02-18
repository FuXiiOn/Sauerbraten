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
#include "string"
#include <sstream>

typedef BOOL(__stdcall* hooked_wglSwapBuffers)(HDC hdc);
hooked_wglSwapBuffers o_wglSwapBuffers;

typedef int(__fastcall* SDL_SetCursor)(int mode);
SDL_SetCursor SDL_setCursor;

typedef int(__fastcall* SDL_ShowCursor)(int mode);
SDL_ShowCursor SDL_showCursor;

typedef __int64(__fastcall* MagicBullet)(__int64 entity, Vector3* a2);
MagicBullet magicBulletHook;

typedef __int64(__fastcall* TriggerBot)(__int64 a1, __int64 a2, __int64 a3, float* a4);
TriggerBot triggerBotHook;

typedef __int64(__fastcall* SilentAim)(Vector3 bulletOrigin, Vector3 bulletDest, __int64 entity);
SilentAim silentHook;

typedef float(__fastcall* VisCheck)(Vector3 a1, Vector3 a2, float a3, int a4, int a5, int a6);

BYTE oSwapBuffersBytes[15] = { 0x48, 0x89, 0x5C, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48, 0x83, 0xEC, 0x40 };
BYTE oOneHitBytes[15] = { 0x01, 0x91, 0x40, 0x03, 0x00, 0x00, 0x89, 0x81, 0x48, 0x03, 0x00, 0x00, 0x48, 0x3B, 0xCF };
BYTE oMagicBulletBytes[17] = { 0x40, 0x55, 0x53, 0x41, 0x56, 0x48, 0x8D, 0x6C, 0x24, 0xD0, 0x48, 0x81, 0xEC, 0x30, 0x01, 0x00, 0x00 };
BYTE oTriggerBotBytes[15] = { 0x48, 0x89, 0x4C, 0x24, 0x08, 0x53, 0x55, 0x56, 0x57, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56 };
BYTE oSilentBytes[18] = { 0x48, 0x89, 0x54, 0x24, 0x10, 0x55, 0x53, 0x56, 0x57, 0x41, 0x55, 0x41, 0x57, 0x48, 0x8D, 0x6C, 0x24, 0xC8 };

HWND gameHWND = FindWindowA(NULL, "Cube 2: Sauerbraten");

extern void executeFeatures();

extern "C" void oneHit();
extern "C" uintptr_t jmpBackOneHit = 0;
extern "C" bool bOnehit = false;

char fileName[MAX_PATH];

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

__int64 __fastcall silentAimHook(Vector3 bulletOrigin, Vector3 bulletDest, __int64 entity){
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	uintptr_t entList = *(uintptr_t*)(moduleBase + 0x346C90);
	int* currPlayers = (int*)(moduleBase + 0x346C9C);

	if ((uintptr_t)entity == (uintptr_t)localPlayer && Config::bSilent) {
		if (closestSilent) {

			int randValue = rand() % 100;

			if (randValue < Config::silentHitChance) {
				bulletDest.x = closestSilent->bodypos.x;
				bulletDest.y = closestSilent->bodypos.y;
				bulletDest.z = closestSilent->bodypos.z;

				return silentHook(bulletOrigin, bulletDest, entity);
			}
		}
	}

	return silentHook(bulletOrigin, bulletDest, entity);
}

__int64 __fastcall magicBulletFunct(__int64 entity, Vector3* enemy) {
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	RECT rect;
	GetClientRect(gameHWND, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;
	float fovDegrees = 1.5f * atan(Config::fovRadius / (wndWidth / 2)) * (180.0f / std::numbers::pi);

	if (entity == (uintptr_t)localPlayer && Config::bMagicBullet) {
		if (closestSilent) {

			int randValue = rand() % 100;

			if (randValue < Config::magicHitChance) {
				*enemy = closestSilent->bodypos;
				localPlayer->bodypos = (*enemy - Vector3(0.1f, 0.1f, 0.1f));

				return magicBulletHook(entity, enemy);
			}
			else {
				return magicBulletHook(entity, enemy);
			}
		}
	}

	return magicBulletHook(entity, enemy);
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
	int* currPlayers = (int*)(moduleBase + 0x346C9C);
	float closestDistance = FLT_MAX;
	ent* closestEntity = nullptr;
	ent* bestSilent = nullptr;
	float closestFov = FLT_MAX;
	RECT rect;
	GetClientRect(gameHWND, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;
	float fovDegrees = 1.5f * atan(Config::fovRadius / (wndWidth / 2)) * (180.0f / std::numbers::pi);

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



	if ((Config::bSilent || Config::bMagicBullet) && Config::bSnapLine && *gameState == 0) {
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
			ent* entity = *reinterpret_cast<ent**>(entList + i * 8);

			if (entity == localPlayer) continue;
			if (entity->health < 0 || entity->health > 100) continue;
			if (!Config::bTeammates && *entity->team == *localPlayer->team) continue;

			Vector3 newHead = entity->bodypos;
			newHead.z = entity->bodypos.z - EYE_HEIGHT + PLAYER_HEIGHT / 2;

			if (GL::WorldToScreen(newHead, screenCoords, viewMatrix, wndWidth, wndHeight)) {
				GL::DrawBox(entity, screenCoords);
			}
		}
	}

	ImGui::SetNextWindowSize(ImVec2(500, 350));

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
				ImGui::SliderInt("Hitchance ##silent", &Config::silentHitChance, 0, 100, "%d%%");
			}
			ImGui::Checkbox("Magic Bullet", &Config::bMagicBullet);
			if (Config::bMagicBullet) {
				ImGui::SliderInt("Hitchance ##magic", &Config::magicHitChance, 0, 100, "%d%%");
			}
			ImGui::Checkbox("Snap Lines", &Config::bSnapLine);
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
			ImGui::Checkbox("Draw HealthBar", &Config::bHealthBar);
			ImGui::Checkbox("Draw Names", &Config::bNames);
			ImGui::Checkbox("Draw Distance", &Config::bDistance);
			ImGui::Checkbox("Draw teammates", &Config::bTeammates);
			ImGui::ColorEdit3("Team Color", Config::selectedTeamColor);
			ImGui::ColorEdit3("Enemy Color", Config::selectedEnemyColor);
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

		if (ImGui::BeginTabItem("Config")) {
			static char fileName[MAX_PATH] = "";
			static std::string selectedConfig = "";
			static char localAppdata[MAX_PATH];

			ExpandEnvironmentStringsA("%localappdata%", localAppdata, MAX_PATH);

			std::string configPath = std::string(localAppdata) + "\\" + fileName + ".ini";

			std::string searchPattern = std::string(localAppdata) + "\\*.ini";
			WIN32_FIND_DATAA findFileData;
			HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findFileData);

			std::vector<std::string> configFiles;

			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					configFiles.push_back(findFileData.cFileName);
				} while (FindNextFileA(hFind, &findFileData) != 0);
				FindClose(hFind);
			}

			ImGui::Text("Save/Load configs");

			static bool isConfigSelected = false;

			if (ImGui::BeginListBox("##configs", ImVec2(300, 5 * ImGui::GetTextLineHeightWithSpacing()))) {
				for (const std::string& file : configFiles) {
					if (ImGui::Selectable(file.substr(0, file.find(".ini")).c_str(), selectedConfig == file)) {
						selectedConfig = file;
						isConfigSelected = true;
					}
				}
				ImGui::EndListBox();
			}

			if (isConfigSelected) {
				std::string selectedConfigPath = std::string(localAppdata) + "\\" + selectedConfig;

				if (ImGui::Button("Save Config", ImVec2(120, 20))) {
					WritePrivateProfileStringA("Aimbot", "Aimbot", Config::bAimbot ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "Smoothing", std::to_string(Config::aimSmooth).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "SilentAim", Config::bSilent ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "SilentHitChance", std::to_string(Config::silentHitChance).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "MagicBullet", Config::bMagicBullet ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "MagicHitChance", std::to_string(Config::magicHitChance).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "SnapLines", Config::bSnapLine ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "FOV", Config::bFov ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "FOVRadius", std::to_string(Config::fovRadius).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "VisibleCheck", Config::bVisCheck ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Aimbot", "TriggerBot", Config::bTriggerbot ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "ESP", Config::bEsp ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "DrawHealthBar", Config::bHealthBar ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "DrawNames", Config::bNames ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "DrawDistance", Config::bDistance ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "DrawTeammates", Config::bTeammates ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "TeamColor", (std::to_string(Config::selectedTeamColor[0]) + "," + std::to_string(Config::selectedTeamColor[1]) + "," + std::to_string(Config::selectedTeamColor[2])).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("ESP", "EnemyColor", (std::to_string(Config::selectedEnemyColor[0]) + "," + std::to_string(Config::selectedEnemyColor[1]) + "," + std::to_string(Config::selectedEnemyColor[2])).c_str(), selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "NoRecoil", Config::bKnockback ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "ThirdPerson", Config::bThirdPerson ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "BunnyHop", Config::bBunnyHop ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "Godmode", Config::bGodmode ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "InfiniteAmmo", Config::bInfAmmo ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "RapidFire", Config::bRapidFire ? "1" : "0", selectedConfigPath.c_str());
					WritePrivateProfileStringA("Misc", "OneHit", Config::bOnehit ? "1" : "0", selectedConfigPath.c_str());
				}

				if (ImGui::Button("Load Config", ImVec2(120, 20))) {
					char smoothValue[MAX_PATH];
					char fovValue[MAX_PATH];
					char selectedTeamColorBuffer[MAX_PATH];
					char selectedEnemyColorBuffer[MAX_PATH];
					GetPrivateProfileStringA("ESP", "TeamColor", "0,1.0,0", selectedTeamColorBuffer, MAX_PATH, selectedConfigPath.c_str());
					GetPrivateProfileStringA("ESP", "Enemycolor", "1.0,0,0", selectedEnemyColorBuffer, MAX_PATH, selectedConfigPath.c_str());
					GetPrivateProfileStringA("Aimbot", "Smoothing", "0", smoothValue, MAX_PATH, selectedConfigPath.c_str());
					GetPrivateProfileStringA("Aimbot", "FOVRadius", "0", fovValue, MAX_PATH, selectedConfigPath.c_str());

					std::stringstream stringStreamTeam(selectedTeamColorBuffer);
					std::stringstream stringStreamEnemy(selectedEnemyColorBuffer);
					std::vector<float> teamColors;
					std::vector<float> enemyColors;
					std::string teamItem;
					std::string enemyItem;

					while (std::getline(stringStreamTeam, teamItem, ',')) {
						teamColors.push_back(std::stof(teamItem));
					}
					while (std::getline(stringStreamEnemy, enemyItem, ',')) {
						enemyColors.push_back(std::stof(enemyItem));
					}

					Config::bAimbot = GetPrivateProfileIntA("Aimbot", "Aimbot", 0, selectedConfigPath.c_str());
					Config::aimSmooth = std::stof(smoothValue);
					Config::bSilent = GetPrivateProfileIntA("Aimbot", "SilentAim", 0, selectedConfigPath.c_str());
					Config::silentHitChance = GetPrivateProfileIntA("Aimbot", "SilentHitChance", 0, selectedConfigPath.c_str());
					Config::bMagicBullet = GetPrivateProfileIntA("Aimbot", "MagicBullet", 0, selectedConfigPath.c_str());
					Config::magicHitChance = GetPrivateProfileIntA("Aimbot", "MagicHitChance", 0, selectedConfigPath.c_str());
					Config::bSnapLine = GetPrivateProfileIntA("Aimbot", "SnapLines", 0, selectedConfigPath.c_str());
					Config::bFov = GetPrivateProfileIntA("Aimbot", "FOV", 0, selectedConfigPath.c_str());
					Config::fovRadius = std::stof(fovValue);
					Config::bVisCheck = GetPrivateProfileIntA("Aimbot", "VisibleCheck", 0, selectedConfigPath.c_str());
					Config::bTriggerbot = GetPrivateProfileIntA("Aimbot", "TriggerBot", 0, selectedConfigPath.c_str());
					Config::bEsp = GetPrivateProfileIntA("ESP", "ESP", 0, selectedConfigPath.c_str());
					Config::bHealthBar = GetPrivateProfileIntA("ESP", "DrawHealthBar", 0, selectedConfigPath.c_str());
					Config::bNames = GetPrivateProfileIntA("ESP", "DrawNames", 0, selectedConfigPath.c_str());
					Config::bDistance = GetPrivateProfileIntA("ESP", "DrawDistance", 0, selectedConfigPath.c_str());
					Config::bTeammates = GetPrivateProfileIntA("ESP", "DrawTeammates", 0, selectedConfigPath.c_str());
					Config::selectedTeamColor[0] = teamColors[0]; Config::selectedTeamColor[1] = teamColors[1]; Config::selectedTeamColor[2] = teamColors[2];
					Config::selectedEnemyColor[0] = enemyColors[0]; Config::selectedEnemyColor[1] = enemyColors[1]; Config::selectedEnemyColor[2] = enemyColors[2];
					Config::bKnockback = GetPrivateProfileIntA("Misc", "NoRecoil", 0, selectedConfigPath.c_str());
					Config::bThirdPerson = GetPrivateProfileIntA("Misc", "ThirdPerson", 0, selectedConfigPath.c_str());
					Config::bBunnyHop = GetPrivateProfileIntA("Misc", "BunnyHop", 0, selectedConfigPath.c_str());
					Config::bGodmode = GetPrivateProfileIntA("Misc", "Godmode", 0, selectedConfigPath.c_str());
					Config::bInfAmmo = GetPrivateProfileIntA("Misc", "InfiniteAmmo", 0, selectedConfigPath.c_str());
					Config::bOnehit = GetPrivateProfileIntA("Misc", "Onehit", 0, selectedConfigPath.c_str());
					Config::bRapidFire = GetPrivateProfileIntA("Misc", "RapidFire", 0, selectedConfigPath.c_str());
				}

				if (ImGui::Button("Delete Config", ImVec2(120, 20))) {
					if (DeleteFileA(selectedConfigPath.c_str())) {
						selectedConfig = "";
						isConfigSelected = false;
					}
				}

				if (ImGui::Button("Rename Config", ImVec2(120, 20))) {
					ImGui::OpenPopup("Rename Config");
					ImGui::SetNextWindowSize(ImVec2(150, 80));
				}

				if (ImGui::BeginPopupModal("Rename Config", 0, ImGuiWindowFlags_NoResize)) {
					static char newName[256];
					std::string newNamePath = std::string(localAppdata) + "\\" + newName + ".ini";
					ImGui::InputText("##cfgRename", newName, 256);

					if (ImGui::Button("Rename")) {
						if (newName[0] != '\0') {
							MoveFileA(selectedConfigPath.c_str(), newNamePath.c_str());
							ImGui::CloseCurrentPopup();
							memset(newName, 0, sizeof(newName));
						}
					}
					ImGui::EndPopup();
				}
			}

			ImGui::InputText("Config name", fileName, MAX_PATH);

			if (ImGui::Button("Create Config", ImVec2(120, 20)) && fileName[0] != '\0') {

				if (GetFileAttributesA(configPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
					HANDLE hFile = CreateFileA(configPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile != INVALID_HANDLE_VALUE) {
						WritePrivateProfileSectionA("Aimbot", "", configPath.c_str());
						WritePrivateProfileSectionA("ESP", "", configPath.c_str());
						WritePrivateProfileSectionA("Misc", "", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "Aimbot", Config::bAimbot ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "Smoothing", std::to_string(Config::aimSmooth).c_str(), configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "SilentAim", Config::bSilent ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "SilentHitChance", std::to_string(Config::silentHitChance).c_str(), configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "MagicBullet", Config::bMagicBullet ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "MagicHitChance", std::to_string(Config::magicHitChance).c_str(), configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "SnapLines", Config::bSnapLine ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "FOV", Config::bFov ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "FOVRadius", std::to_string(Config::fovRadius).c_str(), configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "VisibleCheck", Config::bVisCheck ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Aimbot", "TriggerBot", Config::bTriggerbot ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "ESP", Config::bEsp ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "DrawHealthBar", Config::bHealthBar ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "DrawNames", Config::bNames ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "DrawDistance", Config::bDistance ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "DrawTeammates", Config::bTeammates ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("ESP", "TeamColor", (std::to_string(Config::selectedTeamColor[0]) + "," + std::to_string(Config::selectedTeamColor[1]) + "," + std::to_string(Config::selectedTeamColor[2])).c_str(), configPath.c_str());
						WritePrivateProfileStringA("ESP", "EnemyColor", (std::to_string(Config::selectedEnemyColor[0]) + "," + std::to_string(Config::selectedEnemyColor[1]) + "," + std::to_string(Config::selectedEnemyColor[2])).c_str(), configPath.c_str());
						WritePrivateProfileStringA("Misc", "NoRecoil", Config::bKnockback ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "ThirdPerson", Config::bThirdPerson ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "BunnyHop", Config::bBunnyHop ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "Godmode", Config::bGodmode ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "InfiniteAmmo", Config::bInfAmmo ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "RapidFire", Config::bRapidFire ? "1" : "0", configPath.c_str());
						WritePrivateProfileStringA("Misc", "OneHit", Config::bOnehit ? "1" : "0", configPath.c_str());
						CloseHandle(hFile);
					}
				}
			}

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
	magicBulletHook = (MagicBullet)mem::TrampHook((BYTE*)(moduleBase + 0x1DB4C0), (BYTE*)magicBulletFunct, 17);
	silentHook = (SilentAim)mem::TrampHook((BYTE*)(moduleBase + 0x1D5310), (BYTE*)silentAimHook, 18);
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

	Sleep(100);

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	VirtualFree(o_wglSwapBuffers, 0, MEM_RELEASE);
	VirtualFree(magicBulletHook, 0, MEM_RELEASE);
	VirtualFree(triggerBotHook, 0, MEM_RELEASE);

	mem::Patch((BYTE*)wglSwapBuffers, oSwapBuffersBytes, 15);
	mem::Patch((BYTE*)(moduleBase + 0x1DB2A0), oTriggerBotBytes, 15);
	mem::Patch((BYTE*)(moduleBase + 0x1DB4C0), oMagicBulletBytes, 17);
	mem::Patch((BYTE*)(moduleBase + 0x1ECB97), oOneHitBytes, 15);
	mem::Patch((BYTE*)(moduleBase + 0x1D5310), oSilentBytes, 18);

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

