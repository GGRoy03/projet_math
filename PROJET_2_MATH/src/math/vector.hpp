#pragma once

#include <cmath>

#include "utility/types.h"

struct vec_2
{
	union
	{
		struct { f32 x, y; };
		f32 AsArray[2];
	};

	vec_2() : x(0), y(0) {}
	vec_2(f32 X, f32 Y) : x(X), y(Y) {}
};


struct vec_3
{
	union
	{
		struct { f32 x, y, z; };
		f32 AsArray[3];
	};

	vec_3() : x(0), y(0), z(0) {}
	vec_3(f32 X, f32 Y, f32 Z) : x(X), y(Y), z(Z) {}
};

struct vec_4
{
	union
	{
		struct { f32 x, y, z, w; };
		f32 AsArray[4];
	};

	vec_4() : x(0), y(0), z(0) {}
	vec_4(f32 X, f32 Y, f32 Z, f32 W) : x(X), y(Y), z(Z), w(W) {}

	f32& operator[](size_t Index) { return (&x)[Index]; }
};

constexpr u32 VEC_4_LENGTH = 4;
constexpr u32 VEC_3_LENGTH = 3;

inline static vec_3 operator+(vec_3 vl, vec_3 vr)
{
	vec_3 Result = vec_3(vl.x + vr.x, vl.y + vr.y, vl.z + vr.z);
	return Result;
}

inline static vec_3 operator-(vec_3 vl, vec_3 vr)
{
	vec_3 Result = vec_3(vl.x - vr.x, vl.y - vr.y, vl.z - vr.z);
	return Result;
}

inline static vec_3 operator*(vec_3 v, f32 Scalar)
{
	vec_3 Result = vec_3(v.x * Scalar, v.y * Scalar, v.z * Scalar);
	return Result;
}

inline static vec_3 operator/(vec_3 v, f32 Divider)
{
	vec_3 Result = vec_3();
	Result.x = v.x / Divider;
	Result.y = v.y / Divider;
	Result.z = v.z / Divider;
	return Result;
}

inline static bool AreEqual(vec_3 vl, vec_3 vr)
{
	bool Result = (vl.x == vr.x) && (vl.y == vr.y) && (vl.z == vr.z);
	return Result;
}

inline static bool IsZeroVector(vec_3 v)
{
	bool Result = (v.x == 0) && (v.y == 0) && (v.z == 0);
	return Result;
}

inline static vec_3 ScaleVector(vec_3 v, f32 Scalar)
{
	vec_3 Result = vec_3(v.x * Scalar, v.y * Scalar, v.z * Scalar);
	return Result;
}

inline static f32 Dot(vec_3 vl, vec_3 vr)
{
	f32 Result = 0;
	Result += vl.x * vr.x;
	Result += vl.y * vr.x;
	Result += vl.z * vr.z;
	return Result;
}

inline static f32 Dot(vec_4 vl, vec_4 vr)
{
	f32 Result = 0;
	for (u32 VectorIndex = 0; VectorIndex < VEC_4_LENGTH; VectorIndex++)
	{
		Result += vl.AsArray[VectorIndex] * vr.AsArray[VectorIndex];
	}
	return Result;
}

inline static vec_3 VectorProduct(vec_3 vl, vec_3 vr)
{
	vec_3 Result = vec_3(
		(vl.y * vr.z) - (vl.z * vr.y),
		(vl.z * vr.x) - (vl.x * vr.z),
		(vl.x * vr.y) - (vl.y * vr.x)
	);
	return Result;
}

inline static f32 VectorLength(vec_3 v)
{
	f32 SquaredSum = 0;
	SquaredSum += v.x * v.x;
	SquaredSum += v.y * v.y;
	SquaredSum += v.z * v.z;

	if (SquaredSum >= 0)
	{
		f32 Result = sqrtf(SquaredSum);
		return Result;
	}
	
	return 0;
}

static vec_3 Normalize(vec_3 v)
{
	f32 Length   = VectorLength(v);
	vec_3 Result = vec_3(
		v.x / Length,
		v.y / Length,
		v.z / Length
	);
	return Result;
}

static f32 GetAngleBetween(vec_3 vl, vec_3 vr)
{
	f32 LengthLeft  = VectorLength(vl);
	f32 LengthRight = VectorLength(vr);
	
	f32 Denom = LengthLeft * LengthRight;
	if (Denom != 0)
	{
		f32 DotValue     = Dot(vl, vr);
		f32 LeftHandSide = DotValue / Denom;
		f32 Result       = acos(LeftHandSide);
		return Result;
	}
	
	return 0.0f;
}

static vec_3 ProjectVectorOnVector(vec_3 pv, vec_3 tv)
{
	f32 Denom = Dot(tv, tv);
	if (Denom != 0)
	{
		f32 Numer = Dot(pv, tv);
		f32 Scale = Numer / Denom;

		vec_3 Projection = tv * Scale;
		return Projection;
	}

	return vec_3();
}

static vec_3 ProjectVectorOnPlane(vec_3 v, vec_3 pn)
{
	vec_3 ProjectionOnNormal = pn * Dot(v, pn);
	vec_3 ProjectionOnPlane  = v - ProjectionOnNormal;
	return ProjectionOnPlane;
}

inline static void PrintVector(vec_3 v)
{
	printf("Vector: (%f,%f,%f)\n", v.x , v.y, v.z);
}

inline static void PrintVector(vec_3 v, const char* Message)
{
	printf("%s: | Vector: (%f,%f,%f)\n", Message, v.x, v.y, v.z);
}

inline static void PrintVector(vec_4 v)
{
	printf("Vector: (%f,%f,%f,%f)\n", v.x, v.y, v.z, v.w);
}

inline static void PrintVector(vec_4 v, const char* Message)
{
	printf("%s: | Vector: (%f,%f,%f,%f)\n", Message, v.x, v.y, v.z, v.w);
}

// ==================================================================================
