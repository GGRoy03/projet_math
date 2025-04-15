#pragma once

#include "types.h"

enum key_code
{
	A_KEY = 0x41,
	B_KEY = 0x42,
	C_KEY = 0x43,
	D_KEY = 0x44,
	E_KEY = 0x45,
	F_KEY = 0x46,
	G_KEY = 0x47,
	H_KEY = 0x48,
	I_KEY = 0x49,
	J_KEY = 0x4A,
	K_KEY = 0x4B,
	L_KEY = 0x4C,
	M_KEY = 0x4D,
	N_KEY = 0x4E,
	O_KEY = 0x4F,
	P_KEY = 0x50,
	Q_KEY = 0x51,
	R_KEY = 0x52,
	S_KEY = 0x53,
	T_KEY = 0x54,
	U_KEY = 0x55,
	V_KEY = 0x56,
	W_KEY = 0x57,
	X_KEY = 0x58,
	Y_KEY = 0x59,
	Z_KEY = 0x5A,
};

struct inputs_state
{
	f32 XPos;
	f32 YPos;

	f32 LastX;
	f32 LastY;

	bool LeftClickHeld;
	bool RightClickHeld;;
	f32  ScrollDelta;

	bool KeysDown[256];
};

static inputs_state Inputs;