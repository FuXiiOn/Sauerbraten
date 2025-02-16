#pragma once
#include <iostream>
#include "geom.h"

class ent
{
public:
	Vector3 bodypos; //0x0000
	Vector3 velocity; //0x000C
	char pad_0018[24]; //0x0018
	Vector3 headpos; //0x0030
	float yaw; //0x003C
	float pitch; //0x0040
	char pad_0044[308]; //0x0044
	int32_t health; //0x0178
	char pad_017C[16]; //0x017C
	int32_t currWeapon; //0x018C
	int32_t shootDelay; //0x0190
	int32_t chainsaw; //0x0194
	int32_t shotgunAmmo; //0x0198
	int32_t machinegunAmmo; //0x019C
	int32_t rocketAmmo; //0x01A0
	int32_t sniperAmmo; //0x01A4
	int32_t grenadeAmmo; //0x01A8
	int32_t pistolAmmo; //0x01AC
	char pad_01B0[76]; //0x01B0
	bool isShooting; //0x01FC
	char pad_01FD[104]; //0x01FD
	char N00000260[15]; //0x0265
	char name[7]; //0x0274
	char pad_027B[248]; //0x027B
	char N000005D0[5]; //0x0373
	char team[4]; //0x0378
	char pad_037C[3112]; //0x037C
}; //Size: 0x0FA4
static_assert(sizeof(ent) == 0xFA4);

class server
{
public:
	class entClass* ent; //0x0000
	char pad_0008[4288]; //0x0008
}; //Size: 0x10C8
static_assert(sizeof(server) == 0x10C8);

class entClass
{
public:
	char pad_0000[832]; //0x0000
	int32_t health; //0x0340
	int32_t N000003BB; //0x0344
	int32_t armor; //0x0348
	char pad_034C[8]; //0x034C
	int32_t currWeapon; //0x0354
	int32_t shootDelay; //0x0358
	int32_t chainsawAmmo; //0x035C
	int32_t shotgunAmmo; //0x0360
	int32_t machinegunAmmo; //0x0364
	int32_t rocketAmmo; //0x0368
	int32_t sniperAmmo; //0x036C
	int32_t grenadeAmmo; //0x0370
	int32_t pistolAmmo; //0x0374
	char pad_0378[1328]; //0x0378
}; //Size: 0x08A8
static_assert(sizeof(entClass) == 0x8A8);

class entListClass
{
public:
	class ent* localPlayer; //0x0000
	class ent* ent1; //0x0008
	class ent* ent2; //0x0010
	class ent* ent3; //0x0018
	class ent* ent4; //0x0020
	class ent* ent5; //0x0028
	class ent* ent6; //0x0030
	class ent* ent7; //0x0038
	class ent* ent8; //0x0040
	class ent* ent9; //0x0048
	class ent* ent10; //0x0050
	class ent* ent11; //0x0058
	class ent* ent12; //0x0060
	class ent* ent13; //0x0068
	class ent* ent14; //0x0070
	class ent* ent15; //0x0078
	class ent* ent16; //0x0080
	class ent* ent17; //0x0088
	class ent* ent18; //0x0090
	class ent* ent19; //0x0098
	class ent* ent20; //0x00A0
	class ent* ent21; //0x00A8
	class ent* ent22; //0x00B0
	class ent* ent23; //0x00B8
	class ent* ent24; //0x00C0
	class ent* ent25; //0x00C8
	class ent* ent26; //0x00D0
	class ent* ent27; //0x00D8
	class ent* ent28; //0x00E0
	class ent* ent29; //0x00E8
	class ent* ent30; //0x00F0
	class ent* ent31; //0x00F8
	class ent* ent32; //0x0100
	char pad_0108[1912]; //0x0108
}; //Size: 0x0880
static_assert(sizeof(entListClass) == 0x880);