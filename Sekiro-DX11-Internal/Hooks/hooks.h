#include "../Structs/structs.h"

extern DWORD64 EntlistJmpBack;
extern DWORD64 fovJmpBack;
extern DWORD64 frameLockJmpBack;
extern DWORD64 EntityObjStart;

extern playerent* ents[];
extern playerent* entsptr;
extern float fovasm;

__declspec(naked) void frameLockhook();
__declspec(naked) void fovhook();
__declspec(naked) void entityhook();


void RestoreJumps();