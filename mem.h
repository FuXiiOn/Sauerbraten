#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>

namespace mem {
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);
	bool Hook(BYTE* src, BYTE* dst, size_t length);
	BYTE* TrampHook(BYTE* src, BYTE* dst, size_t length);
	void Patch(BYTE* dst, BYTE* newBytes, unsigned int length);
	void Nop(BYTE* dst, unsigned int length);
}