#include "mem.h"

#include <windows.h>
#include <iostream>
#include <vector>

uintptr_t mem::FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets) {
    uintptr_t addr = ptr;

    for (unsigned int i = 0; i < offsets.size(); i++) {
        addr = *(uintptr_t*)addr;
        addr += offsets[i];
    }
    return addr;
}

bool mem::Hook(BYTE* src, BYTE* dst, size_t length) {
    if (length < 13) return false;

    DWORD oldProtect;
    VirtualProtect(src, length, PAGE_EXECUTE_READWRITE, &oldProtect);

    memset(src, 0x90, length);

    src[0] = 0xFF;
    src[1] = 0x25;
    *(uint32_t*)(src + 2) = 0x00;
    *(uint64_t*)(src + 6) = (uintptr_t)dst;

    VirtualProtect(src, length, oldProtect, &oldProtect);

    return true;
}

BYTE* mem::TrampHook(BYTE* src, BYTE* dst, size_t length) {
    if (length < 14) return 0;

    BYTE* gateway = (BYTE*)VirtualAlloc(0, length + 12, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    memcpy_s(gateway, length, src, length);

    *(gateway + length) = 0xFF;
    *(gateway + length + 1) = 0x25;
    *(uint32_t*)(gateway + length + 2) = 0x00;
    *(uint64_t*)(gateway + length + 6) = (uintptr_t)src + length;

    mem::Hook(src, dst, length);

    return gateway;
}

void mem::Patch(BYTE* dst, BYTE* newBytes, unsigned int length) {
	DWORD oldProtect;
	VirtualProtect(dst, length, PAGE_EXECUTE_READWRITE, &oldProtect);

	memcpy(dst, newBytes, length);
	VirtualProtect(dst, length, oldProtect, &oldProtect);
}

void mem::Nop(BYTE* dst, unsigned int length) {
	DWORD oldProtect;
	VirtualProtect(dst, length, PAGE_EXECUTE_READWRITE, &oldProtect);

	memset(dst, 0x90, length);
	VirtualProtect(dst, length, oldProtect, &oldProtect);
}