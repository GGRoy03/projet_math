#pragma once

#include "types.h"

static u32 StringLength(const char* String)
{
	u32 CharCount = 0;
	while (*String != '\0')
	{
		String++;
		CharCount++;
	}
	return CharCount;
}

static inline void ConvertToWide(const char* String, wchar_t* WideBuffer, i32 WideBufferSize)
{
	MultiByteToWideChar(CP_ACP, 0, String, -1, WideBuffer, WideBufferSize);
}