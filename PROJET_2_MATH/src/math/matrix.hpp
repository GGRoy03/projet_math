#pragma once

#include "vector.hpp"

constexpr auto D_PI = 3.141592653589793;
constexpr auto F_PI = 3.14159265f;
constexpr float DegToRad(float degrees) { return degrees * (F_PI / 180.0f); }

constexpr auto MAT_4_ROW_COUNT = 4;

struct mat_4
{
	union
	{
		struct { 
			vec_4 r1;
			vec_4 r2;
			vec_4 r3;
			vec_4 r4;
		};
		f32 AsArray[16];
	};

	mat_4():
		r1(vec_4(1, 0, 0, 0)),
		r2(vec_4(0, 1, 0, 0)),
		r3(vec_4(0, 0, 1, 0)),
		r4(vec_4(0, 0, 0, 1))
	{}

	mat_4(const vec_4& R1, const vec_4& R2, const vec_4& R3, const vec_4& R4)
		  : r1(R1), r2(R2), r3(R3), r4(R4)
	{}

	vec_4& operator[](u32 Index) { return (&r1)[Index]; }
};

inline static mat_4 operator*(mat_4 Transform, mat_4 Base)
{
	vec_4 FirstColumn = vec_4(
		Base.AsArray[0],
		Base.AsArray[4],
		Base.AsArray[8],
		Base.AsArray[12]
	);

	vec_4 SecondColumn = vec_4(
		Base.AsArray[1],
		Base.AsArray[5],
		Base.AsArray[9],
		Base.AsArray[13]
	);

	vec_4 ThirdColumn = vec_4(
		Base.AsArray[2],
		Base.AsArray[6],
		Base.AsArray[10],
		Base.AsArray[14]
	);

	vec_4 FourthColumn = vec_4(
		Base.AsArray[3],
		Base.AsArray[7],
		Base.AsArray[11],
		Base.AsArray[15]
	);

	mat_4 Result;
	for (u32 RowIndex = 0; RowIndex < MAT_4_ROW_COUNT; RowIndex++)
	{
		vec_4 Row          = Transform[RowIndex];

		Result[RowIndex].x = Dot(Row, FirstColumn);
		Result[RowIndex].y = Dot(Row, SecondColumn);
		Result[RowIndex].z = Dot(Row, ThirdColumn);
		Result[RowIndex].w = Dot(Row, FourthColumn);
	}

	return Result;
}

static void PrintMatrix(mat_4 m)
{
	PrintVector(m.r1, "Row 1: ");
	PrintVector(m.r2, "Row 2: ");
	PrintVector(m.r3, "Row 3: ");
	PrintVector(m.r4, "Row 4: ");
}

static void PrintMatrix(mat_4 m, const char* Message)
{
	printf("%s: \n", Message);
	PrintVector(m.r1, "Row 1: ");
	PrintVector(m.r2, "Row 2: ");
	PrintVector(m.r3, "Row 3: ");
	PrintVector(m.r4, "Row 4: ");
}

static mat_4 ProjectionMatrix(f32 FOV, f32 AspectRatio, f32 Near, f32 Far)
{
	f32 FOVRadians = FOV * (F_PI / 180.0f);
	
	f32 XProj        = 1 / (AspectRatio * tanf(FOVRadians / 2));
	f32 YProj        = 1 / tanf(FOVRadians / 2);
	f32 Depth        = Far - Near;

	mat_4 ProjectionMatrix = mat_4(
		vec_4(XProj, 0    ,                     0,                         0),
		vec_4(0    , YProj,                     0,                         0),
		vec_4(0    , 0    , (Far + Near) / Depth , (-2 * Far * Near) / Depth),
		vec_4(0    , 0    ,                     1,                         0)
	);
	
	return ProjectionMatrix;
}

static mat_4 FocusMatrix(vec_3 DefaultUp, vec_3 CameraPosition, vec_3 Focus)
{
	vec_3 Forward = Normalize(Focus - CameraPosition);
	vec_3 Right   = Normalize(VectorProduct(DefaultUp, Forward));
	vec_3 Up      = Normalize(VectorProduct(Forward, Right));

	mat_4 Rotation = mat_4(
		vec_4(Right.x  , Right.y  , Right.z  , 0),
		vec_4(Up.x     , Up.y     , Up.z     , 0),
		vec_4(Forward.x, Forward.y, Forward.z, 0),
		vec_4(0        , 0        , 0        , 1)
	);

	mat_4 Translation = mat_4(
		vec_4(1, 0, 0, -CameraPosition.x),
		vec_4(0, 1, 0, -CameraPosition.y),
		vec_4(0, 0, 1, -CameraPosition.z),
		vec_4(0, 0, 0,                 1)
	);

	return Rotation * Translation;
}

static mat_4 IdentityMatrix()
{
	mat_4 Result = mat_4(
		vec_4(1, 0, 0, 0),
		vec_4(0, 1, 0, 0),
		vec_4(0, 0, 1, 0),
		vec_4(0, 0, 0, 1)
	);

	return Result;
}

static mat_4 ScalingMatrix(vec_3 ScaleVector)
{
	f32 XScale = ScaleVector.x;
	f32 YScale = ScaleVector.y;
	f32 ZScale = ScaleVector.z;

	mat_4 Result = mat_4(
		vec_4(XScale, 0     , 0     , 0),
		vec_4(0     , YScale, 0     , 0),
		vec_4(0     , 0     , ZScale, 0),
		vec_4(0     , 0     , 0     , 1)
	);

	return Result;
}

static mat_4 TranslationMatrix(vec_3 TranslationVector)
{
	f32 XTrans = TranslationVector.x;
	f32 YTrans = TranslationVector.y;
	f32 ZTrans = TranslationVector.z;

	mat_4 Result = mat_4(
		vec_4(1, 0, 0, XTrans),
		vec_4(0, 1, 0, YTrans),
		vec_4(0, 0, 1, ZTrans),
		vec_4(0, 0, 0, 1)
	);

	return Result;
}

static mat_4 RotationMatrixFromEulerAngles(vec_3 Angles)
{
	f32 Pitch = DegToRad(Angles.x);
	f32 Yaw   = DegToRad(Angles.y);
	f32 Roll  = DegToRad(Angles.z);

	mat_4 Rx = mat_4(
		vec_4(1, 0, 0, 0),
		vec_4(0, cosf(Pitch), -sinf(Pitch), 0),
		vec_4(0, sinf(Pitch), cosf(Pitch), 0),
		vec_4(0, 0, 0, 1)
	);

	mat_4 Ry = mat_4(
		vec_4(cosf(Yaw), 0, sinf(Yaw), 0),
		vec_4(0, 1, 0, 0),
		vec_4(-sinf(Yaw), 0, cosf(Yaw), 0),
		vec_4(0, 0, 0, 1)
	);

	mat_4 Rz = mat_4(
		vec_4(cosf(Roll), -sinf(Roll), 0, 0),
		vec_4(sinf(Roll), cosf(Roll), 0, 0),
		vec_4(0, 0, 1, 0),
		vec_4(0, 0, 0, 1)
	);

	mat_4 Combined = Rz * Ry * Rx;
	return Combined;
}