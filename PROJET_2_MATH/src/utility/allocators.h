#pragma once

#include "types.h"

#define WIN32_MEAN_AND_LEAN
#include <Windows.h>

enum BUMP_ALLOCATOR_MODE
{
	BUMP_FIXED,
	BUMP_RESIZABLE,
};

struct bump_allocator
{
	char Tag[16]             = {};
	char* Memory             = nullptr;
	u32 GrowthFactor         = 2;
	size_t At                = 0;
	size_t Size              = 0;
	size_t Capacity          = 0;
	BUMP_ALLOCATOR_MODE Mode = BUMP_FIXED;
};

inline bump_allocator CreateBumpAllocator(size_t Size, BUMP_ALLOCATOR_MODE Mode = BUMP_FIXED, const char* Tag = "NONE", u32 GrowthFactor = 2)
{
	bump_allocator Allocator = {};
	Allocator.Capacity       = Size;
	Allocator.Mode           = Mode;
	Allocator.GrowthFactor   = GrowthFactor;
	Allocator.Memory         = (char*)VirtualAlloc(NULL, Size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	memcpy(Allocator.Tag, Tag, 16);

	ASSERT(Allocator.Memory, "Failed to allocate memory for allocator. Memory corruption?");

	return Allocator;
}

inline void* PushSize(size_t Size, bump_allocator* Allocator)
{
	if (Allocator->At + Size > Allocator->Capacity)
	{
		if (Allocator->Mode == BUMP_RESIZABLE)
		{
			size_t NewCapacity = Allocator->Capacity * Allocator->GrowthFactor;
			if (NewCapacity < Allocator->At + Size)
			{
				NewCapacity = Allocator->At + Size;
			}

			void* NewMemory = VirtualAlloc(NULL, NewCapacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			ASSERT(NewMemory, "Possible memory corruption? | Out of memory?");
			memcpy(NewMemory, Allocator->Memory, Allocator->At);
			VirtualFree(Allocator->Memory, 0, MEM_RELEASE);

			Allocator->Memory   = (char*)NewMemory;
			Allocator->Capacity = NewCapacity;
		}
		else
		{
			// TODO: Fatal Error - Out of memory
			return nullptr;
		}
	}

	void* Block      = Allocator->Memory + Allocator->At;
	Allocator->At   += Size;
	Allocator->Size += 1;
	return Block;
}

inline void* PushAndCopy(size_t Size, void* Data, bump_allocator* Allocator)
{
	void* Block = PushSize(Size, Allocator);
	if (Block)
	{
		memcpy(Block, Data, Size);
	}
	return Block;
}

inline void ClearAllocator(bump_allocator* Allocator)
{
	memset(Allocator->Memory, 0, Allocator->At);
	Allocator->At = 0;
	Allocator->Size = 0;
}

inline void FreeAllocator(bump_allocator* Allocator)
{
	VirtualFree(Allocator->Memory, 0, MEM_RELEASE);
}

inline size_t GetElementsCount(bump_allocator* Allocator, size_t ElementsSize)
{
	size_t ElementsCount = Allocator->At / ElementsSize;
	return ElementsCount;
}