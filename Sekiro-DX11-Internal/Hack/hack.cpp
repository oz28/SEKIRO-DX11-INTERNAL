// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include <Windows.h>
#include <dwmapi.h>
#include <TlHelp32.h>
#include <string>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <cstdlib>
#include <sstream>
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_impl_win32.h"
#include "../ImGui/imgui_impl_dx11.h"
#include "hack.h"
#include "../Mathtools/mathtools.h"
#include "stringapiset.h"
#include <d3d11.h>
#include <atlconv.h>
#include <atlstr.h>
#include <codecvt>
#define _USE_MATH_DEFINES
#include <math.h>
#include "../Hooks/hooks.h"
#include "..//Offsets/offsets.hpp"

using namespace std;

const int frameRate240 = 998803592;
const int frameRate144 = 1004768825;
const int frameRate60 = 1015580809;

bool nameesp = false;
bool nameesp2 = false;
bool boneesp = false;
bool snapline = false;
bool healthesp = false;
bool coordsesp = false;
bool playerskeletesp = false;
bool dirESP = false;
bool displayHud = true;

bool frameRate240Selected = false;
bool frameRate144Selected = false;
bool frameRate60Selected = true;

bool isRunning;
bool eject = false;

//#define INRANGE(x,a,b)	(x >= a && x <= b) 
//#define getBits( x )	(INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
//#define getByte( x )	(getBits(x[0]) << 4 | getBits(x[1]))

int counter = 0;
int rwidth, rhight;
char fpsconvert[99];

Vector3 LocalPlayerCoords;
uintptr_t* sekiroEntitylistadress;
uintptr_t sekiroEntitylistadressval;
size_t ViewMatrix = 0x0;
size_t GameResolution = 0x0;
HMODULE hmodule;
HWND hwnd;

float Matrix[16];

char* module = _strdup("sekiro.exe");
uintptr_t frameLock = (ComboFindPattern(module, (char*)"88 88 3C 4C 89 AB")) - 0x1;
uintptr_t frameLockSpeedFix = (ComboFindPattern(module, (char*)"F3 0F 58 ?? 0F C6 ?? 00 0F 51 ?? F3 0F 59 ?? ?? ?? ?? ?? 0F 2F")) + 15;
uintptr_t* frameLockSpeedFixaddr = (uintptr_t*)0x1432892D8;

uintptr_t viewbase;
uintptr_t resbase;
uintptr_t moduleBase;

void SetupHack() {
	viewbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + offsets::dwViewmatrixBase);
	resbase = (uintptr_t)((uintptr_t)GetModuleHandle("sekiro.exe") + offsets::dwResInfoBase);

}

void InitFrameRateLock() {
	DWORD dwOldProtect, dwBkup;

	uintptr_t* frameLockaddr = (uintptr_t*)frameLock;


	if (frameRate240Selected) {
		VirtualProtect(frameLockaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(int*)frameLockaddr = frameRate240;
		VirtualProtect(frameLockaddr, 50, dwOldProtect, &dwBkup);


		VirtualProtect(frameLockSpeedFixaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)frameLockSpeedFixaddr = 120.0f;
		VirtualProtect(frameLockSpeedFixaddr, 50, dwOldProtect, &dwBkup);
	}

	if (frameRate144Selected) {
		VirtualProtect(frameLockaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(int*)frameLockaddr = frameRate144;
		VirtualProtect(frameLockaddr, 50, dwOldProtect, &dwBkup);

		VirtualProtect(frameLockSpeedFixaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)frameLockSpeedFixaddr = 72.0f;
		VirtualProtect(frameLockSpeedFixaddr, 50, dwOldProtect, &dwBkup);
	}

	if (frameRate60Selected || eject) {
		VirtualProtect(frameLockaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(int*)frameLockaddr = frameRate60;
		VirtualProtect(frameLockaddr, 50, dwOldProtect, &dwBkup);


		VirtualProtect(frameLockSpeedFixaddr, 50, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)frameLockSpeedFixaddr = 30.0f;
		VirtualProtect(frameLockSpeedFixaddr, 50, dwOldProtect, &dwBkup);
	}
}

void GetFPS() {
	uintptr_t fpsbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + offsets::dwEngineFPSBase);
	uintptr_t fpsptr1 = *(uintptr_t*)(fpsbase + 0xB0);
	uintptr_t fpsptr2 = *(uintptr_t*)(fpsptr1 + 0x110);
	uintptr_t fpsptr3 = *(uintptr_t*)(fpsptr2 + 0x228);
	uintptr_t fpsptr4 = *(uintptr_t*)(fpsptr3 + 0x180);

	float fps = *(float*)(fpsptr4 + 0xA54);

	sprintf(fpsconvert, "%.5f", fps);

}

float Get3dDistance(Vector3 LocalPlayerCoords, Vector3 targetCoords)
{

	return (float)sqrt(pow(double(targetCoords.x - LocalPlayerCoords.x), 2.0) + pow(double(targetCoords.y - LocalPlayerCoords.y), 2.0) + pow(double(targetCoords.z - LocalPlayerCoords.z), 2.0));

}

void DrawNumber2(float x, float y, float z, char* Bonename, int BoneColorR, int BoneColorG, int BoneColorB)
{

	Vector3 Bone;
	Bone.x = x;
	Bone.y = y;
	Bone.z = z;
	float Matrix[16];

	uintptr_t viewbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + 0x03D88368);

	uintptr_t* viewMatrix = (uintptr_t*)(viewbase + 0x20);

	if (viewbase == 0)
	{
		ViewMatrix = 0;
	}
	else
	{
		ViewMatrix = (size_t)(uintptr_t*)viewMatrix;
	}

	if (ViewMatrix != 0)
	{
		memcpy(&Matrix, (PBYTE*)ViewMatrix, sizeof(Matrix));
	}


	vec2 vScreenBone;
	if (WorldToScreen(Bone, vScreenBone, Matrix, rwidth, rhight))
	{
		ImGui::Begin("##stfuffdfs", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
		auto draw = ImGui::GetBackgroundDrawList();
		draw->AddText(ImVec2(vScreenBone.x, vScreenBone.y), IM_COL32(BoneColorR, BoneColorG, BoneColorB, 255), Bonename);
		ImGui::End();
	}
}

void DrawBone(Vector3 Bone1, Vector3 Bone2, int BoneColorA, int BoneColorR, int BoneColorG, int BoneColorB)
{


	float Matrix[16];

	uintptr_t viewbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + 0x03D88368);

	uintptr_t* viewMatrix = (uintptr_t*)(viewbase + 0x20);

	if (viewbase == 0)
	{
		ViewMatrix = 0;
	}
	else
	{
		ViewMatrix = (size_t)(uintptr_t*)viewMatrix;
	}

	if (ViewMatrix != 0)
	{
		memcpy(&Matrix, (PBYTE*)ViewMatrix, sizeof(Matrix));
	}


	vec2 vScreenBone1;
	vec2 vScreenBone2;
	if (WorldToScreen(Bone1, vScreenBone1, Matrix, rwidth, rhight))
	{
		if (WorldToScreen(Bone2, vScreenBone2, Matrix, rwidth, rhight))
		{

			ImGui::Begin("##stfuffdfs", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
			auto draw = ImGui::GetBackgroundDrawList();
			draw->AddLine(ImVec2(vScreenBone1.x, vScreenBone1.y), ImVec2(vScreenBone2.x, vScreenBone2.y), IM_COL32(BoneColorR, BoneColorG, BoneColorB, BoneColorA), 0.5f);
			ImGui::End();
		}
	}
}


void DrawBone2(float x, float y, float z, float x1, float y1, float z1, int BoneColorA, int BoneColorR, int BoneColorG, int BoneColorB)
{
	Vector3 Bone1;
	Bone1.x = x;
	Bone1.y = y;
	Bone1.z = z;


	Vector3 Bone2;
	Bone2.x = x1;
	Bone2.y = y1;
	Bone2.z = z1;


	float Matrix[16];

	uintptr_t viewbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + 0x03D88368);

	uintptr_t* viewMatrix = (uintptr_t*)(viewbase + 0x20);

	if (viewbase == 0)
	{
		ViewMatrix = 0;
	}
	else
	{
		ViewMatrix = (size_t)(uintptr_t*)viewMatrix;
	}

	if (ViewMatrix != 0)
	{
		memcpy(&Matrix, (PBYTE*)ViewMatrix, sizeof(Matrix));
	}


	vec2 vScreenBone1;
	vec2 vScreenBone2;
	if (WorldToScreen(Bone1, vScreenBone1, Matrix, rwidth, rhight))
	{
		if (WorldToScreen(Bone2, vScreenBone2, Matrix, rwidth, rhight))
		{

			ImGui::Begin("##stfuffdfs", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
			auto draw = ImGui::GetBackgroundDrawList();
			draw->AddLine(ImVec2(vScreenBone1.x, vScreenBone1.y), ImVec2(vScreenBone2.x, vScreenBone2.y), IM_COL32(BoneColorR, BoneColorG, BoneColorB, BoneColorA), 0.5f);
			ImGui::End();
		}
	}
}


void FOV(float value) {
	DWORD dwOldProtect, dwBkup;

	uintptr_t baseFOV = ((uintptr_t)GetModuleHandle("sekiro.exe") + 0x3289098);
	uintptr_t* baseFOVADDR = (uintptr_t*)baseFOV;



	if (baseFOV != 0) {
		VirtualProtect((BYTE*)baseFOV, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)baseFOVADDR = value;
		VirtualProtect((BYTE*)baseFOV, 5, dwOldProtect, &dwBkup);

	}


}
template <typename T, std::size_t N> T* end_(T(&arr)[N]) { return arr + N; }

template <typename T, std::size_t N> T* begin_(T(&arr)[N]) { return arr; }

void clearEntityList() {
	for (size_t i = 0; i < 500; i++)
	{
		ents[i] = NULL;
	}
}

void TeleportEveryoneToMe() {
	for (size_t i = 1; i < 500; i++)
	{
		DWORD dwOldProtect, dwBkup;
		uintptr_t* LocalPlayerBase = (uintptr_t*)ents[0];

		uintptr_t LocalPlayerBasex = (uintptr_t)LocalPlayerBase + 0x80;
		uintptr_t LocalPlayerBasey = (uintptr_t)LocalPlayerBase + 0x84;
		uintptr_t LocalPlayerBasez = (uintptr_t)LocalPlayerBase + 0x88;

		uintptr_t* targetPlayerBase = (uintptr_t*)ents[i];

		uintptr_t targetPlayerBaseX = (uintptr_t)targetPlayerBase + 0x80;
		uintptr_t targetPlayerBaseY = (uintptr_t)targetPlayerBase + 0x84;
		uintptr_t targetPlayerBaseZ = (uintptr_t)targetPlayerBase + 0x88;

		if (targetPlayerBase == 0) {
			return;
		}

		VirtualProtect((BYTE*)(targetPlayerBaseX), 99, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)(targetPlayerBaseX) = *(float*)(LocalPlayerBasex)+0.2f;
		*(float*)(targetPlayerBaseY) = *(float*)(LocalPlayerBasey)+0.2f;
		*(float*)(targetPlayerBaseZ) = *(float*)(LocalPlayerBasez)+0.2f;
		VirtualProtect((BYTE*)targetPlayerBaseX, 99, dwOldProtect, &dwBkup);
	}
}

void TeleportToEntity(Vector3 target, bool isLocalPlayer, int index) {

	if (isLocalPlayer) {
		DWORD dwOldProtect, dwBkup;
		uintptr_t* LocalPlayerBase = (uintptr_t*)ents[0];

		uintptr_t LocalPlayerBasex = (uintptr_t)LocalPlayerBase + 0x80;
		uintptr_t LocalPlayerBasey = (uintptr_t)LocalPlayerBase + 0x84;
		uintptr_t LocalPlayerBasez = (uintptr_t)LocalPlayerBase + 0x88;

		VirtualProtect((BYTE*)(LocalPlayerBasex), 99, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)(LocalPlayerBasex) = target.x;
		*(float*)(LocalPlayerBasey) = target.y;
		*(float*)(LocalPlayerBasez) = target.z;
		VirtualProtect((BYTE*)LocalPlayerBasex, 99, dwOldProtect, &dwBkup);
	}
	else
	{
		DWORD dwOldProtect, dwBkup;
		uintptr_t* targetPlayerBase = (uintptr_t*)ents[index];

		uintptr_t targetPlayerBaseX = (uintptr_t)targetPlayerBase + 0x80;
		uintptr_t targetPlayerBaseY = (uintptr_t)targetPlayerBase + 0x84;
		uintptr_t targetPlayerBaseZ = (uintptr_t)targetPlayerBase + 0x88;

		VirtualProtect((BYTE*)(targetPlayerBaseX), 99, PAGE_EXECUTE_READWRITE, &dwOldProtect);
		*(float*)(targetPlayerBaseX) = target.x;
		*(float*)(targetPlayerBaseY) = target.y;
		*(float*)(targetPlayerBaseZ) = target.z;
		VirtualProtect((BYTE*)targetPlayerBaseX, 99, dwOldProtect, &dwBkup);

	}

}

Vector3 playerbones[300];
int r = 0;
void DrawPlayerBones()
{
	if (playerskeletesp) {


		static uintptr_t offsetBonePos = 0x10;
		static uintptr_t offsetBone = 0x30;

		uintptr_t base = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + 0x03AFB218);
		if (base == 0) {
			return;
		}
		uintptr_t boneptr1 = *(uintptr_t*)(base + 0x0);

		uintptr_t boneptr2 = *(uintptr_t*)(boneptr1 + 0x48);
		uintptr_t boneptr3 = *(uintptr_t*)(boneptr2 + 0x1F0);
		uintptr_t boneptr4 = *(uintptr_t*)(boneptr3 + 0x2D0);
		uintptr_t boneptr5 = *(uintptr_t*)(boneptr4 + 0x58);
		uintptr_t* boneptr6addr = (uintptr_t*)(boneptr5 + 0xC);

		ImGui::Begin("Draw Range");

		ImGui::InputInt("BoneRange", &r);

		ImGui::End();


		for (size_t i = 0; i < r; i++)
		{
			float x = *(float*)(boneptr5 + 0xC + offsetBone * i + offsetBonePos * 0x0); // start
			float y = *(float*)(boneptr5 + 0xC + offsetBone * i + offsetBonePos * 0x1);	 // start
			float z = *(float*)(boneptr5 + 0xC + offsetBone * i + offsetBonePos * 0x2); // start

			playerbones[i].x = x;
			playerbones[i].y = y;
			playerbones[i].z = z;

			char array[999];
			sprintf(array, "%d", i);

			//DrawNumber(playerbones[i], array, 255, 0, 0);

		}





		for (size_t i = 0; i < 200; i++)
		{
			//spine
			DrawBone(playerbones[1], playerbones[4], 255, 0, 255, 0);
			DrawBone(playerbones[4], playerbones[5], 255, 0, 255, 0);
			DrawBone(playerbones[5], playerbones[6], 255, 0, 255, 0);
			DrawBone(playerbones[6], playerbones[7], 255, 0, 255, 0);
			DrawBone(playerbones[7], playerbones[8], 255, 0, 255, 0);
			DrawBone(playerbones[8], playerbones[9], 255, 0, 255, 0);

			//leftshoulder
			DrawBone(playerbones[7], playerbones[95], 255, 0, 255, 0);
			DrawBone(playerbones[95], playerbones[97], 255, 0, 255, 0);
			DrawBone(playerbones[97], playerbones[98], 255, 0, 255, 0);
			//left thumb
			DrawBone(playerbones[98], playerbones[99], 255, 0, 255, 0);
			DrawBone(playerbones[99], playerbones[100], 255, 0, 255, 0);
			DrawBone(playerbones[100], playerbones[101], 255, 0, 255, 0);
			//left finger
			DrawBone(playerbones[98], playerbones[102], 255, 0, 255, 0);
			DrawBone(playerbones[102], playerbones[103], 255, 0, 255, 0);
			DrawBone(playerbones[103], playerbones[104], 255, 0, 255, 0);
			//left middle
			DrawBone(playerbones[98], playerbones[105], 255, 0, 255, 0);
			DrawBone(playerbones[105], playerbones[106], 255, 0, 255, 0);
			DrawBone(playerbones[106], playerbones[107], 255, 0, 255, 0);

			//left ring
			DrawBone(playerbones[98], playerbones[108], 255, 0, 255, 0);
			DrawBone(playerbones[108], playerbones[109], 255, 0, 255, 0);
			DrawBone(playerbones[109], playerbones[110], 255, 0, 255, 0);

			//leftlittle
			DrawBone(playerbones[98], playerbones[111], 255, 0, 255, 0);
			DrawBone(playerbones[111], playerbones[112], 255, 0, 255, 0);
			DrawBone(playerbones[112], playerbones[113], 255, 0, 255, 0);

			//left leg
			DrawBone(playerbones[1], playerbones[142], 255, 0, 255, 0);
			DrawBone(playerbones[142], playerbones[144], 255, 0, 255, 0);
			DrawBone(playerbones[144], playerbones[145], 255, 0, 255, 0);
			DrawBone(playerbones[145], playerbones[146], 255, 0, 255, 0);

			//right leg
			DrawBone(playerbones[1], playerbones[147], 255, 0, 255, 0);
			DrawBone(playerbones[147], playerbones[149], 255, 0, 255, 0);
			DrawBone(playerbones[149], playerbones[150], 255, 0, 255, 0);
			DrawBone(playerbones[150], playerbones[151], 255, 0, 255, 0);


			//rightshulder
			DrawBone(playerbones[7], playerbones[117], 255, 0, 255, 0);
			DrawBone(playerbones[117], playerbones[119], 255, 0, 255, 0);
			DrawBone(playerbones[119], playerbones[120], 255, 0, 255, 0);

			//right thumb
			DrawBone(playerbones[120], playerbones[121], 255, 0, 255, 0);
			DrawBone(playerbones[121], playerbones[122], 255, 0, 255, 0);
			DrawBone(playerbones[122], playerbones[123], 255, 0, 255, 0);

			//right finger
			DrawBone(playerbones[120], playerbones[124], 255, 0, 255, 0);
			DrawBone(playerbones[124], playerbones[125], 255, 0, 255, 0);
			DrawBone(playerbones[125], playerbones[126], 255, 0, 255, 0);

			//right middle fing
			DrawBone(playerbones[120], playerbones[127], 255, 0, 255, 0);
			DrawBone(playerbones[127], playerbones[128], 255, 0, 255, 0);
			DrawBone(playerbones[128], playerbones[129], 255, 0, 255, 0);

			//right ring
			DrawBone(playerbones[120], playerbones[130], 255, 0, 255, 0);
			DrawBone(playerbones[130], playerbones[131], 255, 0, 255, 0);
			DrawBone(playerbones[131], playerbones[132], 255, 0, 255, 0);

			//right little
			DrawBone(playerbones[120], playerbones[133], 255, 0, 255, 0);
			DrawBone(playerbones[133], playerbones[134], 255, 0, 255, 0);
			DrawBone(playerbones[134], playerbones[135], 255, 0, 255, 0);
		}

		Sleep(1);
	}





}

float drawRange = 0;


// convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

void HUDModification() {

	uintptr_t hudbase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + offsets::dwHudBase);
	uintptr_t entList = *(uintptr_t*)(hudbase + 0x50);

	if (displayHud) {
		short show = 768;
		*(short*)(entList + 0x8) = show;
	}
	else
	{
		short hide = 0;
		*(short*)(entList + 0x8) = hide;
	}





}


void ESP()
{
	char* entityName;
	char* localPlayerName;

	uintptr_t* viewMatrix = (uintptr_t*)(viewbase + 0x20);

	if (viewbase == 0)
	{
		ViewMatrix = 0;
	}
	else
	{
		ViewMatrix = (size_t)(uintptr_t*)viewMatrix;
	}

	uintptr_t* resadd = (uintptr_t*)(resbase);

	int x = *(int*)(resbase);
	int y = *(int*)(resbase + 0x4);

	GameResolution = (size_t)(uintptr_t*)resbase;

	memcpy(&rwidth, (PBYTE*)GameResolution, sizeof(rwidth));
	memcpy(&rhight, (PBYTE*)(GameResolution + 0x4), sizeof(rhight));
	if (ViewMatrix != 0)
	{
		memcpy(&Matrix, (PBYTE*)ViewMatrix, sizeof(Matrix));
	}


	uintptr_t ingameFlagBase = *(uintptr_t*)((uintptr_t)GetModuleHandle("sekiro.exe") + offsets::dwIngameFlag);


	for (int i = 0; i < 255; i++)
	{

		if (ents[i] != 0)
		{
			vec2 vScreen;
			if (WorldToScreen(ents[i]->coords, vScreen, Matrix, rwidth, rhight))
			{
				if (ents[i] != 0 && ents[i]->boneptr1->healthptr1->healthptr2->health > 0 && ents[i]->boneptr1->healthptr1->healthptr2->health << 9999) {
					int health = ents[i]->boneptr1->healthptr1->healthptr2->health;

					char array[99];
					sprintf(array, "%d", health);

					LocalPlayerCoords.x = ents[0]->coords.x;
					LocalPlayerCoords.y = ents[0]->coords.y;
					LocalPlayerCoords.z = ents[0]->coords.z;

					char array2[99];
					float distance = Get3dDistance(LocalPlayerCoords, ents[i]->coords);
					sprintf(array2, "%.2f", distance);
					if (playerskeletesp)
					{
						string entIDstr = wstring_to_utf8(ents[i]->boneptr1->healthptr1->healthptr2->entID);
						const char* entID = entIDstr.c_str();

						if (entIDstr == "c1010") {

							//spine to head
							DrawBone2(ents[i]->boneptr1->boneptr2->b2_X, ents[i]->boneptr1->boneptr2->b2_Y, ents[i]->boneptr1->boneptr2->b2_Z, ents[i]->boneptr1->boneptr2->b41f_X, ents[i]->boneptr1->boneptr2->b41f_Y, ents[i]->boneptr1->boneptr2->b41f_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b41f_X, ents[i]->boneptr1->boneptr2->b41f_Y, ents[i]->boneptr1->boneptr2->b41f_Z, ents[i]->boneptr1->boneptr2->b42_X, ents[i]->boneptr1->boneptr2->b42_Y, ents[i]->boneptr1->boneptr2->b42_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b42_X, ents[i]->boneptr1->boneptr2->b42_Y, ents[i]->boneptr1->boneptr2->b42_Z, ents[i]->boneptr1->boneptr2->b43_X, ents[i]->boneptr1->boneptr2->b43_Y, ents[i]->boneptr1->boneptr2->b43_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b43_X, ents[i]->boneptr1->boneptr2->b43_Y, ents[i]->boneptr1->boneptr2->b43_Z, ents[i]->boneptr1->boneptr2->b44_X, ents[i]->boneptr1->boneptr2->b44_Y, ents[i]->boneptr1->boneptr2->b44_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b44_X, ents[i]->boneptr1->boneptr2->b44_Y, ents[i]->boneptr1->boneptr2->b44_Z, ents[i]->boneptr1->boneptr2->b100_X, ents[i]->boneptr1->boneptr2->b100_Y, ents[i]->boneptr1->boneptr2->b100_Z, 255, 0, 255, 0);

							DrawBone2(ents[i]->boneptr1->boneptr2->b2_X, ents[i]->boneptr1->boneptr2->b2_Y, ents[i]->boneptr1->boneptr2->b2_Z, ents[i]->boneptr1->boneptr2->b4_X, ents[i]->boneptr1->boneptr2->b4_Y, ents[i]->boneptr1->boneptr2->b4_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b4_X, ents[i]->boneptr1->boneptr2->b4_Y, ents[i]->boneptr1->boneptr2->b4_Z, ents[i]->boneptr1->boneptr2->b13_X, ents[i]->boneptr1->boneptr2->b13_Y, ents[i]->boneptr1->boneptr2->b13_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b13_X, ents[i]->boneptr1->boneptr2->b13_Y, ents[i]->boneptr1->boneptr2->b13_Z, ents[i]->boneptr1->boneptr2->b7_X, ents[i]->boneptr1->boneptr2->b7_Y, ents[i]->boneptr1->boneptr2->b7_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b7_X, ents[i]->boneptr1->boneptr2->b7_Y, ents[i]->boneptr1->boneptr2->b7_Z, ents[i]->boneptr1->boneptr2->b8_X, ents[i]->boneptr1->boneptr2->b8_Y, ents[i]->boneptr1->boneptr2->b8_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b8_X, ents[i]->boneptr1->boneptr2->b8_Y, ents[i]->boneptr1->boneptr2->b8_Z, ents[i]->boneptr1->boneptr2->b9_X, ents[i]->boneptr1->boneptr2->b9_Y, ents[i]->boneptr1->boneptr2->b9_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b9_X, ents[i]->boneptr1->boneptr2->b9_Y, ents[i]->boneptr1->boneptr2->b9_Z, ents[i]->boneptr1->boneptr2->b11_X, ents[i]->boneptr1->boneptr2->b11_Y, ents[i]->boneptr1->boneptr2->b11_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b11_X, ents[i]->boneptr1->boneptr2->b11_Y, ents[i]->boneptr1->boneptr2->b11_Z, ents[i]->boneptr1->boneptr2->b10_X, ents[i]->boneptr1->boneptr2->b10_Y, ents[i]->boneptr1->boneptr2->b10_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b11_X, ents[i]->boneptr1->boneptr2->b11_Y, ents[i]->boneptr1->boneptr2->b11_Z, ents[i]->boneptr1->boneptr2->b10_X, ents[i]->boneptr1->boneptr2->b10_Y, ents[i]->boneptr1->boneptr2->b10_Z, 255, 0, 255, 0);


							//rLeg

							DrawBone2(ents[i]->boneptr1->boneptr2->b2_X, ents[i]->boneptr1->boneptr2->b2_Y, ents[i]->boneptr1->boneptr2->b2_Z, ents[i]->boneptr1->boneptr2->b14_X, ents[i]->boneptr1->boneptr2->b14_Y, ents[i]->boneptr1->boneptr2->b14_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b14_X, ents[i]->boneptr1->boneptr2->b14_Y, ents[i]->boneptr1->boneptr2->b14_Z, ents[i]->boneptr1->boneptr2->b23_X, ents[i]->boneptr1->boneptr2->b23_Y, ents[i]->boneptr1->boneptr2->b23_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b23_X, ents[i]->boneptr1->boneptr2->b23_Y, ents[i]->boneptr1->boneptr2->b23_Z, ents[i]->boneptr1->boneptr2->b16_X, ents[i]->boneptr1->boneptr2->b16_Y, ents[i]->boneptr1->boneptr2->b16_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b16_X, ents[i]->boneptr1->boneptr2->b16_Y, ents[i]->boneptr1->boneptr2->b16_Z, ents[i]->boneptr1->boneptr2->b18_X, ents[i]->boneptr1->boneptr2->b18_Y, ents[i]->boneptr1->boneptr2->b18_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b18_X, ents[i]->boneptr1->boneptr2->b18_Y, ents[i]->boneptr1->boneptr2->b18_Z, ents[i]->boneptr1->boneptr2->b19_X, ents[i]->boneptr1->boneptr2->b19_Y, ents[i]->boneptr1->boneptr2->b19_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b19_X, ents[i]->boneptr1->boneptr2->b19_Y, ents[i]->boneptr1->boneptr2->b19_Z, ents[i]->boneptr1->boneptr2->b21_X, ents[i]->boneptr1->boneptr2->b21_Y, ents[i]->boneptr1->boneptr2->b21_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b21_X, ents[i]->boneptr1->boneptr2->b21_Y, ents[i]->boneptr1->boneptr2->b21_Z, ents[i]->boneptr1->boneptr2->b20_X, ents[i]->boneptr1->boneptr2->b20_Y, ents[i]->boneptr1->boneptr2->b20_Z, 255, 0, 255, 0);


							//Larm

							DrawBone2(ents[i]->boneptr1->boneptr2->b44_X, ents[i]->boneptr1->boneptr2->b44_Y, ents[i]->boneptr1->boneptr2->b44_Z, ents[i]->boneptr1->boneptr2->b45_X, ents[i]->boneptr1->boneptr2->b45_Y, ents[i]->boneptr1->boneptr2->b45_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b45_X, ents[i]->boneptr1->boneptr2->b45_Y, ents[i]->boneptr1->boneptr2->b45_Z, ents[i]->boneptr1->boneptr2->b48_X, ents[i]->boneptr1->boneptr2->b48_Y, ents[i]->boneptr1->boneptr2->b48_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b48_X, ents[i]->boneptr1->boneptr2->b48_Y, ents[i]->boneptr1->boneptr2->b48_Z, ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, 255, 0, 255, 0);

							//L_thumb
							DrawBone2(ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, ents[i]->boneptr1->boneptr2->b52_X, ents[i]->boneptr1->boneptr2->b52_Y, ents[i]->boneptr1->boneptr2->b52_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b52_X, ents[i]->boneptr1->boneptr2->b52_Y, ents[i]->boneptr1->boneptr2->b52_Z, ents[i]->boneptr1->boneptr2->b53_X, ents[i]->boneptr1->boneptr2->b53_Y, ents[i]->boneptr1->boneptr2->b53_Z, 255, 0, 255, 0);

							//L_finger
							DrawBone2(ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, ents[i]->boneptr1->boneptr2->b54_X, ents[i]->boneptr1->boneptr2->b54_Y, ents[i]->boneptr1->boneptr2->b54_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b54_X, ents[i]->boneptr1->boneptr2->b54_Y, ents[i]->boneptr1->boneptr2->b54_Z, ents[i]->boneptr1->boneptr2->b55_X, ents[i]->boneptr1->boneptr2->b55_Y, ents[i]->boneptr1->boneptr2->b55_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b55_X, ents[i]->boneptr1->boneptr2->b55_Y, ents[i]->boneptr1->boneptr2->b55_Z, ents[i]->boneptr1->boneptr2->b56_X, ents[i]->boneptr1->boneptr2->b56_Y, ents[i]->boneptr1->boneptr2->b56_Z, 255, 0, 255, 0);

							//L_middle
							DrawBone2(ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, ents[i]->boneptr1->boneptr2->b57_X, ents[i]->boneptr1->boneptr2->b57_Y, ents[i]->boneptr1->boneptr2->b57_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b57_X, ents[i]->boneptr1->boneptr2->b57_Y, ents[i]->boneptr1->boneptr2->b57_Z, ents[i]->boneptr1->boneptr2->b58_X, ents[i]->boneptr1->boneptr2->b58_Y, ents[i]->boneptr1->boneptr2->b58_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b58_X, ents[i]->boneptr1->boneptr2->b58_Y, ents[i]->boneptr1->boneptr2->b58_Z, ents[i]->boneptr1->boneptr2->b59_X, ents[i]->boneptr1->boneptr2->b59_Y, ents[i]->boneptr1->boneptr2->b59_Z, 255, 0, 255, 0);

							//L_ring
							DrawBone2(ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, ents[i]->boneptr1->boneptr2->b60_X, ents[i]->boneptr1->boneptr2->b60_Y, ents[i]->boneptr1->boneptr2->b60_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b60_X, ents[i]->boneptr1->boneptr2->b60_Y, ents[i]->boneptr1->boneptr2->b60_Z, ents[i]->boneptr1->boneptr2->b61_X, ents[i]->boneptr1->boneptr2->b61_Y, ents[i]->boneptr1->boneptr2->b61_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b61_X, ents[i]->boneptr1->boneptr2->b61_Y, ents[i]->boneptr1->boneptr2->b61_Z, ents[i]->boneptr1->boneptr2->b62_X, ents[i]->boneptr1->boneptr2->b62_Y, ents[i]->boneptr1->boneptr2->b62_Z, 255, 0, 255, 0);

							//L_little
							DrawBone2(ents[i]->boneptr1->boneptr2->b50_X, ents[i]->boneptr1->boneptr2->b50_Y, ents[i]->boneptr1->boneptr2->b50_Z, ents[i]->boneptr1->boneptr2->b63_X, ents[i]->boneptr1->boneptr2->b63_Y, ents[i]->boneptr1->boneptr2->b63_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b63_X, ents[i]->boneptr1->boneptr2->b63_Y, ents[i]->boneptr1->boneptr2->b63_Z, ents[i]->boneptr1->boneptr2->b64_X, ents[i]->boneptr1->boneptr2->b64_Y, ents[i]->boneptr1->boneptr2->b64_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b64_X, ents[i]->boneptr1->boneptr2->b64_Y, ents[i]->boneptr1->boneptr2->b64_Z, ents[i]->boneptr1->boneptr2->b65_X, ents[i]->boneptr1->boneptr2->b65_Y, ents[i]->boneptr1->boneptr2->b65_Z, 255, 0, 255, 0);


							//r_arm
							DrawBone2(ents[i]->boneptr1->boneptr2->b44_X, ents[i]->boneptr1->boneptr2->b44_Y, ents[i]->boneptr1->boneptr2->b44_Z, ents[i]->boneptr1->boneptr2->b71_X, ents[i]->boneptr1->boneptr2->b71_Y, ents[i]->boneptr1->boneptr2->b71_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b71_X, ents[i]->boneptr1->boneptr2->b71_Y, ents[i]->boneptr1->boneptr2->b71_Z, ents[i]->boneptr1->boneptr2->b74_X, ents[i]->boneptr1->boneptr2->b74_Y, ents[i]->boneptr1->boneptr2->b74_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b74_X, ents[i]->boneptr1->boneptr2->b74_Y, ents[i]->boneptr1->boneptr2->b74_Z, ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, 255, 0, 255, 0);

							//r_thumb
							DrawBone2(ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, ents[i]->boneptr1->boneptr2->b77_X, ents[i]->boneptr1->boneptr2->b77_Y, ents[i]->boneptr1->boneptr2->b77_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b77_X, ents[i]->boneptr1->boneptr2->b77_Y, ents[i]->boneptr1->boneptr2->b77_Z, ents[i]->boneptr1->boneptr2->b78_X, ents[i]->boneptr1->boneptr2->b78_Y, ents[i]->boneptr1->boneptr2->b78_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b78_X, ents[i]->boneptr1->boneptr2->b78_Y, ents[i]->boneptr1->boneptr2->b78_Z, ents[i]->boneptr1->boneptr2->b79_X, ents[i]->boneptr1->boneptr2->b79_Y, ents[i]->boneptr1->boneptr2->b79_Z, 255, 0, 255, 0);

							//r_finger
							DrawBone2(ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, ents[i]->boneptr1->boneptr2->b80_X, ents[i]->boneptr1->boneptr2->b80_Y, ents[i]->boneptr1->boneptr2->b80_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b80_X, ents[i]->boneptr1->boneptr2->b80_Y, ents[i]->boneptr1->boneptr2->b80_Z, ents[i]->boneptr1->boneptr2->b81_X, ents[i]->boneptr1->boneptr2->b81_Y, ents[i]->boneptr1->boneptr2->b81_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b81_X, ents[i]->boneptr1->boneptr2->b81_Y, ents[i]->boneptr1->boneptr2->b81_Z, ents[i]->boneptr1->boneptr2->b82_X, ents[i]->boneptr1->boneptr2->b82_Y, ents[i]->boneptr1->boneptr2->b82_Z, 255, 0, 255, 0);

							//r_middle
							DrawBone2(ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, ents[i]->boneptr1->boneptr2->b83_X, ents[i]->boneptr1->boneptr2->b83_Y, ents[i]->boneptr1->boneptr2->b83_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b83_X, ents[i]->boneptr1->boneptr2->b83_Y, ents[i]->boneptr1->boneptr2->b83_Z, ents[i]->boneptr1->boneptr2->b84_X, ents[i]->boneptr1->boneptr2->b84_Y, ents[i]->boneptr1->boneptr2->b84_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b84_X, ents[i]->boneptr1->boneptr2->b84_Y, ents[i]->boneptr1->boneptr2->b84_Z, ents[i]->boneptr1->boneptr2->b85_X, ents[i]->boneptr1->boneptr2->b85_Y, ents[i]->boneptr1->boneptr2->b85_Z, 255, 0, 255, 0);

							//r_rring
							DrawBone2(ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, ents[i]->boneptr1->boneptr2->b86_X, ents[i]->boneptr1->boneptr2->b86_Y, ents[i]->boneptr1->boneptr2->b86_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b86_X, ents[i]->boneptr1->boneptr2->b86_Y, ents[i]->boneptr1->boneptr2->b86_Z, ents[i]->boneptr1->boneptr2->b87_X, ents[i]->boneptr1->boneptr2->b87_Y, ents[i]->boneptr1->boneptr2->b87_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b87_X, ents[i]->boneptr1->boneptr2->b87_Y, ents[i]->boneptr1->boneptr2->b87_Z, ents[i]->boneptr1->boneptr2->b88_X, ents[i]->boneptr1->boneptr2->b88_Y, ents[i]->boneptr1->boneptr2->b88_Z, 255, 0, 255, 0);

							//r_little
							DrawBone2(ents[i]->boneptr1->boneptr2->b76_X, ents[i]->boneptr1->boneptr2->b76_Y, ents[i]->boneptr1->boneptr2->b76_Z, ents[i]->boneptr1->boneptr2->b89_X, ents[i]->boneptr1->boneptr2->b89_Y, ents[i]->boneptr1->boneptr2->b89_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b89_X, ents[i]->boneptr1->boneptr2->b89_Y, ents[i]->boneptr1->boneptr2->b89_Z, ents[i]->boneptr1->boneptr2->b90_X, ents[i]->boneptr1->boneptr2->b90_Y, ents[i]->boneptr1->boneptr2->b90_Z, 255, 0, 255, 0);
							DrawBone2(ents[i]->boneptr1->boneptr2->b90_X, ents[i]->boneptr1->boneptr2->b90_Y, ents[i]->boneptr1->boneptr2->b90_Z, ents[i]->boneptr1->boneptr2->b91_X, ents[i]->boneptr1->boneptr2->b91_Y, ents[i]->boneptr1->boneptr2->b91_Z, 255, 0, 255, 0);

						}
					}

					ImGui::Begin("##stfuffdfs", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
					auto draw = ImGui::GetBackgroundDrawList();
					if (distance < drawRange)
					{
						if (snapline)
						{

							string entIDstr = wstring_to_utf8(ents[i]->boneptr1->healthptr1->healthptr2->entID);
							const char* entID = entIDstr.c_str();

							draw->AddLine(ImVec2(vScreen.x, vScreen.y), ImVec2(rwidth / 2, rhight), IM_COL32(255, 255, 255, 255), 0.2f);
							draw->AddText(ImVec2(vScreen.x - 70, vScreen.y + 40), IM_COL32(255, 255, 0, 255), "ID: ");
							draw->AddText(ImVec2(vScreen.x, vScreen.y + 40), IM_COL32(255, 255, 0, 255), entID);
							if (i != 0) {
								draw->AddText(ImVec2(vScreen.x - 70, vScreen.y + 20), IM_COL32(255, 255, 0, 255), "Distance: ");
								draw->AddText(ImVec2(vScreen.x, vScreen.y + 20), IM_COL32(255, 255, 0, 255), array2);
							}
						}
						if (healthesp)
						{
							draw->AddText(ImVec2(vScreen.x - 70, vScreen.y - 70), IM_COL32(0, 255, 0, 255), "Health: ");
							draw->AddText(ImVec2(vScreen.x, vScreen.y - 70), IM_COL32(0, 255, 0, 255), array);
						}
						if (coordsesp)
						{
							float x = ents[i]->coords.x;
							float y = ents[i]->coords.y;
							float z = ents[i]->coords.z;
							sprintf(array, "%.2f", x);
							draw->AddText(ImVec2(vScreen.x - 70, vScreen.y), IM_COL32(0, 0, 255, 255), "Position: ");
							draw->AddText(ImVec2(vScreen.x, vScreen.y), IM_COL32(255, 0, 0, 255), array);
							sprintf(array, "%.2f", y);
							draw->AddText(ImVec2(vScreen.x + 45, vScreen.y), IM_COL32(0, 255, 0, 255), array);
							sprintf(array, "%.2f", z);
							draw->AddText(ImVec2(vScreen.x + 95, vScreen.y), IM_COL32(0, 0, 255, 255), array);
						}

						if (dirESP)
						{
							float radiantRotation = ents[i]->rotation;
							radiantRotation += 1.5707963268;
							float deg = radiantRotation * 180.0f / M_PI;
							sprintf(array2, "%.10f", deg);
							ImGui::Begin("tet");
							ImGui::Text(array2);
							ImGui::End();


							float angleX = cosf(radiantRotation);
							float angleY = sinf(radiantRotation);

							Vector3 pointOrigin;
							vec2 pointangle;

							pointOrigin.x = ents[i]->coords.x + angleX;
							pointOrigin.y = ents[i]->coords.y;
							pointOrigin.z = ents[i]->coords.z - angleY;


							if (WorldToScreen(pointOrigin, pointangle, Matrix, rwidth, rhight))
							{
								vec2 screenPos;
								screenPos = pointangle;
								screenPos.x = (pointangle.x - vScreen.x);
								screenPos.y = (pointangle.y - vScreen.y);

								float lengthVec = sqrtf(powf(screenPos.x, 2) + powf(screenPos.y, 2));
								screenPos.x = screenPos.x / lengthVec;
								screenPos.y = screenPos.y / lengthVec;

								float viewDirBarrelLength = 100;
								draw->AddLine(ImVec2(vScreen.x, vScreen.y), ImVec2(vScreen.x + screenPos.x * viewDirBarrelLength, vScreen.y + screenPos.y * viewDirBarrelLength), IM_COL32(255, 255, 255, 255), 0.2f);
								draw->AddCircleFilled(ImVec2(vScreen.x, vScreen.y), 10, IM_COL32(0, 200, 50, 255));

							}

						}


					}
					ImGui::End();

				}

			}
		}
	}
}
