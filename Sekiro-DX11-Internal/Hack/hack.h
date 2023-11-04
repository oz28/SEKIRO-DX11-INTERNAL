#pragma once
#include "../Mathtools/mathtools.h"

extern bool snapline;
extern bool healthesp;
extern bool coordsesp;
extern bool playerskeletesp;
extern float drawRange;
extern bool displayHud;
extern float fps;
extern Vector3 LocalPlayerCoords;

extern bool frameRate240Selected;
extern bool frameRate144Selected;
extern bool frameRate60Selected;
extern bool eject;
extern char fpsconvert[];
extern bool dirESP;

void SetupHack();
void DrawPlayerBones();
void TeleportEveryoneToMe();
void FOV(float value);
void TeleportToEntity( Vector3 target, bool isLocalPlayer, int index);
void clearEntityList();
void DrawBone(Vector3 Bone1, Vector3 Bone2, int BoneColorA, int BoneColorR, int BoneColorG, int BoneColorB);
void ESP();
uintptr_t ComboFindPattern(char* szModule, char* pattern);

DWORD_PTR WINAPI InitiateHooks(LPVOID param);
void DrawNumber2(float x, float y, float z, char* Bonename, int BoneColorR, int BoneColorG, int BoneColorB);
void HUDModification();
void InitFrameRateLock();
void GetFPS();