#include <dwmapi.h>
#include <TlHelp32.h>
#include <string>
#include <Psapi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <iostream>
#include "../Structs/structs.h"
#include "../Hack/hack.h"


playerent* ents[500];
playerent* entsptr;

DWORD64 EntlistJmpBack = 0x0;
DWORD64 fovJmpBack = 0x0;
DWORD64 frameLockJmpBack = 0x0;
DWORD64 EntityObjStart = 0x0;

hookData enthookData;
hookData fovhookData;
hookData frameLockhookData;
BYTE* originalEntAddress;

int ix;
bool alreadyThere = false;
float fovasm = 0.03f;


__declspec(naked) void frameLockhook()
{
	__asm
	{
		mov[rax - 0x76B3C378], ecx
		stosd
		jo DWORD PTR ds : 0x1411AB55B
		add[rax], al
		call DWORD PTR ds : 0x140977E70


		jmp[fovJmpBack]
	}
}

__declspec(naked) void fovhook()
{
	__asm
	{
		mulss  xmm1, DWORD PTR[fovasm]
		subss xmm1, [rsi + 0x50]
		jmp[fovJmpBack]
	}
}



__declspec(naked) void entityhook()
{
	__asm
	{
		movaps[rsp + 0x50], xmm1
		mov r15b, 01
		movss xmm2, [rdi + 0x84]
		mov EntityObjStart, rdi

	}

	__asm
	{
		push rax
		mov rax, EntityObjStart
		mov[entsptr], rax
		pop rax
	}

	if (entsptr == nullptr)
	{
		goto GIVE_UP;
	}

	alreadyThere = false;

	for (ix = 0; ix < 254; ix++)
	{
		if (ents[ix] == entsptr)
		{
			alreadyThere = true;
			break;
		}
	}

	if (alreadyThere)
	{
		goto GIVE_UP;
	}
	else
	{
		for (ix = 0; ix < 254; ix++)
		{
			if (ents[ix] == 0)
			{
				ents[ix] = entsptr;
				break;
			}
		}
	}


GIVE_UP:
	__asm {
		jmp[EntlistJmpBack]
	}
}


uintptr_t ComboFindPattern(char* szModule, char* pattern)
{
	MODULEINFO modInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandleA(szModule), &modInfo, sizeof(MODULEINFO));

	uintptr_t startAddress = (uintptr_t)GetModuleHandleA(szModule);
	uintptr_t base = (uintptr_t)modInfo.lpBaseOfDll;
	uintptr_t size = (startAddress + (uintptr_t)modInfo.SizeOfImage);

	size_t patternLength = strlen(pattern);
	for (size_t i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (int j = 0; j < patternLength; j += 3)
		{
			//convert string literal to byte 
			if (pattern[j] == ' ')
			{
				j -= 2; //makes the j+=3 work properly in this case
				continue;
			}

			//if a wildcard or space, just continue
			if (pattern[j] == '?')
			{
				continue;
			}

			long int  lower = strtol(&pattern[j], 0, 16);

			//if byte does not match the byte from memory
			if ((char)lower != *(char*)(base + i + j / 3))
			{
				found = false; break;
			}
		}
		if (found)
		{
			return (uintptr_t)base + i;
		}
	}
	return 0;
}


BYTE originalJumpCode[16];

void RestoreJMP() {
	DWORD dwOldProtect, dwBkup, dwRelAddr;

	VirtualProtect(originalEntAddress, sizeof(originalJumpCode), PAGE_EXECUTE_READWRITE, &dwOldProtect);

	memcpy(originalEntAddress, &originalJumpCode, sizeof(originalJumpCode));

	VirtualProtect(originalEntAddress, sizeof(originalJumpCode), dwOldProtect, &dwBkup);
}

void RestoreJMPNew(hookData* data) {
	DWORD dwOldProtect, dwBkup, dwRelAddr;

	VirtualProtect(data->originalAddress, data->overwrittenBytes, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	memcpy(data->originalAddress, &data->originalJumpCode, data->overwrittenBytes);

	VirtualProtect(data->originalAddress, data->overwrittenBytes, dwOldProtect, &dwBkup);
}

void RestoreJumps() {
	RestoreJMPNew(&enthookData);
	RestoreJMPNew(&fovhookData);
}


void PlaceJMPNew(hookData* data) {
	DWORD dwOldProtect, dwBkup, dwRelAddr;

	VirtualProtect(data->originalAddress, data->overwrittenBytes, PAGE_EXECUTE_READWRITE, &dwOldProtect);


	uintptr_t* jumpToAdress = data->hookAdress;

	memcpy(&data->originalJumpCode, data->originalAddress, data->overwrittenBytes);
	BYTE jumpCode[] = "\x49\xBA\x00\x00\x00\x00\x00\x00\x00\x00\x41\xFF\xE2";
	memcpy(&jumpCode[2], &jumpToAdress, sizeof(jumpToAdress));
	memcpy(data->originalAddress, &jumpCode, sizeof(jumpCode) - 1);


	for (DWORD x = 13; x < data->overwrittenBytes; x++)
	{
		BYTE* curraddr2 = (BYTE*)data->originalAddress + x;
		*curraddr2 = 0x90;
	}

	VirtualProtect(data->originalAddress, data->overwrittenBytes, dwOldProtect, &dwBkup);
}


DWORD_PTR WINAPI InitiateHooks(LPVOID param) {
	char* module = _strdup("sekiro.exe");


	uintptr_t* entityhookaddr = (uintptr_t*)entityhook;
	uintptr_t* fovhookaddr = (uintptr_t*)fovhook;
	uintptr_t* frameLockhookaddr = (uintptr_t*)frameLockhook;



	enthookData.hookAdress = entityhookaddr;
	enthookData.originalAddress = (uintptr_t*)ComboFindPattern(module, (char*)"0F 29 4C 24 50 41 B7 01 F3 0F 10 97 84 00 00 00");;
	enthookData.overwrittenBytes = 16;
	fovhookData.hookAdress = fovhookaddr;
	fovhookData.originalAddress = (uintptr_t*)ComboFindPattern(module, (char*)"F3 0F 59 0D ?? ?? ?? ?? F3 0F 5C 4E 50");
	fovhookData.overwrittenBytes = 13;

	frameLockhookData.hookAdress = frameLockhookaddr;
	frameLockhookData.originalAddress = (uintptr_t*)ComboFindPattern(module, (char*)"89 88 88 3C 4C 89 AB");
	frameLockhookData.overwrittenBytes = 16;

	EntlistJmpBack = (uintptr_t)(BYTE*)enthookData.originalAddress + enthookData.overwrittenBytes;
	fovJmpBack = (uintptr_t)(BYTE*)fovhookData.originalAddress + fovhookData.overwrittenBytes;
	frameLockJmpBack = (uintptr_t)(BYTE*)frameLockhookData.originalAddress + frameLockhookData.overwrittenBytes;

	PlaceJMPNew(&enthookData);
	PlaceJMPNew(&fovhookData);
	//PlaceJMPNew(&frameLockhookData);


	return NULL;
}

