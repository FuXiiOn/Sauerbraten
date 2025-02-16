#include <iostream>
#include <Windows.h>
#include <math.h>
#include <vector>
#include "mem.h"
#include "config.h"
#include "offsets.h"
#include "numbers"
#include "chrono"
#include "algorithm"
#include <thread>

enum weaponID {
	SHOTGUN = 1,
	MACHINEGUN = 2,
	ROCKET = 3,
	SNIPER = 4,
	GRENADELAUNCHER = 5,
	PISTOL = 6
};

HWND gHWND = FindWindowA(NULL, "Cube 2: Sauerbraten");

typedef float(__fastcall* VisCheck)(Vector3 a1, Vector3 a2, float a3, int a4, int a5, int a6);

static bool isVisible(ent* localPlayer, ent* enemy) {
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

void executeFeatures() {
	uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
	server* serverPlayer = *(server**)(moduleBase + 0x3472D0);
	ent* localPlayer = *(ent**)(moduleBase + 0x2A5730);
	int* currPlayers = (int*)(moduleBase + 0x346C9C);
	uintptr_t entList = *(uintptr_t*)(moduleBase + 0x346C90);
	ent* closestEntity = nullptr;
	RECT rect;
	GetClientRect(gHWND, &rect);
	int wndWidth = rect.right - rect.left;
	int wndHeight = rect.bottom - rect.top;

	if (!localPlayer) return;

	if (Config::bAimbot && !Config::bSilent) {
		float closestDistance = FLT_MAX;
		float closestFov = FLT_MAX;
		float bestYaw = 0, bestPitch = 0;
		float fovDegrees = 1.5f * atan(Config::fovRadius / (wndWidth / 2.0f)) * (180.0f / std::numbers::pi);

		static auto lastUpdateTime = std::chrono::steady_clock::now();

		for (unsigned int i = 0; i < *currPlayers; i++) {
			ent* entity = *reinterpret_cast<ent**>(entList + i * 0x8);

			if (!entity) continue;
			if (entity == localPlayer) continue;
			if (entity->health <= 0 || entity->health > 100) continue;
			if (*entity->team == *localPlayer->team) continue;
			if (Config::bVisCheck && !isVisible(localPlayer, entity)) continue;

			float abspos_x = localPlayer->bodypos.x - entity->bodypos.x;
			float abspos_y = localPlayer->bodypos.y - entity->bodypos.y;
			float abspos_z = localPlayer->bodypos.z - entity->bodypos.z;

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

			float fov = hypot(yawDiff, pitchDiff);

			if (Config::bFov) {
				if (fov < closestFov && fov <= fovDegrees) {
					closestEntity = entity;
					closestFov = fov;
					bestYaw = newYaw + 90;
					bestPitch = newPitch;
				}
			}
			else {
				if (distance < closestDistance) {
					closestEntity = entity;
					closestDistance = distance;
					bestYaw = newYaw + 90;
					bestPitch = newPitch;
				}
			}
		}

		auto currentTime = std::chrono::steady_clock::now();
		if (closestEntity && currentTime - lastUpdateTime >= std::chrono::milliseconds(7)) {
			float currentYaw = localPlayer->yaw;
			float currentPitch = localPlayer->pitch;

			float yawDiff = bestYaw - localPlayer->yaw;
			if (yawDiff > 180.0f) yawDiff -= 360.0f;
			if (yawDiff < -180.0f) yawDiff += 360.0f;
			float pitchDiff = bestPitch - localPlayer->pitch;

			float smoothing = std::clamp((1.2f - Config::aimSmooth), 0.05f, 1.0f);

			currentYaw += yawDiff * smoothing;
			currentPitch += pitchDiff * smoothing;

			localPlayer->yaw = currentYaw;
			localPlayer->pitch = currentPitch;

			lastUpdateTime = currentTime;
		}
	}

	if (Config::bKnockback) {
		mem::Nop((BYTE*)(moduleBase + 0x1DB76F), 18);
	}
	else {
		mem::Patch((BYTE*)(moduleBase + 0x1DB76F), (BYTE*)"\xF3\x41\x0F\x11\x4E\x0C\xF3\x41\x0F\x11\x56\x10\xF3\x41\x0F\x11\x5E\x14", 18);
	}

	if (Config::bGodmode) {
		if (!serverPlayer) return;

		localPlayer->health = 999;
		serverPlayer->ent->health = 999;
	}

	if (Config::bRapidFire) {
		if (!serverPlayer) return;

		localPlayer->shootDelay = 0;
		serverPlayer->ent->shootDelay = 0;
	}

	if (Config::bInfAmmo) {
		if (!serverPlayer) return;

		switch (localPlayer->currWeapon) {
		case SHOTGUN:
			localPlayer->shotgunAmmo = 100;
			serverPlayer->ent->shotgunAmmo = 100;
			break;
		case MACHINEGUN:
			localPlayer->machinegunAmmo = 100;
			serverPlayer->ent->machinegunAmmo = 100;
			break;
		case ROCKET:
			localPlayer->rocketAmmo = 100;
			serverPlayer->ent->rocketAmmo = 100;
			break;
		case SNIPER:
			localPlayer->sniperAmmo = 100;
			serverPlayer->ent->sniperAmmo = 100;
			break;
		case GRENADELAUNCHER:
			localPlayer->grenadeAmmo = 100;
			serverPlayer->ent->grenadeAmmo = 100;
			break;
		case PISTOL:
			localPlayer->pistolAmmo = 100;
			serverPlayer->ent->pistolAmmo = 100;
			break;
		default:
			break;
		}
	}

	if (Config::bThirdPerson) {
		*(int*)(moduleBase + 0x32CFA8) = 1;
		mem::Nop((BYTE*)(moduleBase + 0x1461A0), 10);
	}
	else {
		*(int*)(moduleBase + 0x32CFA8) = 0;
	}

	if (Config::bBunnyHop) {
		uintptr_t moduleBase = (uintptr_t)GetModuleHandle(L"sauerbraten.exe");
		uintptr_t localPlayer = *(uintptr_t*)(moduleBase + 0x2A5730);

		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			*(bool*)(localPlayer + 0x73) = true;
		} else{
			*(bool*)(localPlayer + 0x73) = false;
		}
	}
}