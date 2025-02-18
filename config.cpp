#include "config.h"
#include "glDraw.h"

bool Config::showMenu = false;
bool Config::bAimbot = false;
bool Config::bFov = false;
bool Config::bVisCheck = false;
float Config::fovRadius = 25.0f;
bool Config::bTriggerbot = false;
bool Config::bSilent = false;
bool Config::bMagicBullet = false;
bool Config::bSnapLine = false;
int Config::silentHitChance = 100;
int Config::magicHitChance = 100;
float Config::aimSmooth = 0.0f;
bool Config::bEsp = false;
bool Config::bHealthBar = false;
bool Config::bNames = false;
bool Config::bDistance = false;
bool Config::bTeammates = false;
bool Config::bGodmode = false;
bool Config::bInfAmmo = false;
bool Config::bOnehit = false;
bool Config::bFly = false;
bool Config::bBunnyHop = false;
bool Config::bRapidFire = false;
bool Config::bKnockback = false;
bool Config::bThirdPerson = false;
GLfloat Config::selectedTeamColor[3] = { 0,1.0f,0 };
GLfloat Config::selectedEnemyColor[3] = { 1.0f,0,0 };