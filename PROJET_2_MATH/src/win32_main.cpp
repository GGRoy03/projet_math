#include "utility/inputs.hpp"
#include "utility/types.h"

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"

#include "asset_table.cpp"
#include "directx/dx11_main.cpp"
#include "entities.cpp"
#include "space.cpp"
#include "ui/main_ui.cpp"

static inline LARGE_INTEGER GetWallClock(void)
{
	LARGE_INTEGER Counter;
	QueryPerformanceCounter(&Counter);
	return Counter;
}



struct window_context
{
	bool      Valid;
	u32       Width;
	u32       Height;
	HINSTANCE Instance;
	HWND      Handle;
};

static window_context Window;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK AppWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
	{
		return TRUE;
	}


	switch (msg)
	{
	case WM_SIZE:
	{
		static u64 LastResizeTime;
		u64 Now = GetCurrentTime();
		if (wParam != SIZE_MINIMIZED && Now - LastResizeTime > 100)
		{
			i32 Width = LOWORD(lParam);
			i32 Height = HIWORD(lParam);

			RECT Client = {};
			GetClientRect(Window.Handle, &Client);
			u32 RealWidth = Client.right - Client.left;
			u32 RealHeight = Client.bottom - Client.top;

			BackendWindowResize(Window.Handle, RealWidth, RealHeight);

			if (ImGui::GetCurrentContext())
			{
				UIWindowResize(RealWidth, RealHeight);
			}

			LastResizeTime = Now;
		}
	}
	case WM_INPUT:
	{
		ImGuiContext* ImGuiContext = ImGui::GetCurrentContext();
		if (ImGuiContext && !ImGui::GetIO().WantCaptureMouse && !ImGui::GetIO().WantCaptureKeyboard)
		{
			UINT dwSize;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
			static LPBYTE lpb[sizeof(dwSize)];

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
			{
			}

			RAWINPUT* Raw = (RAWINPUT*)lpb;

			if (Raw->header.dwType == RIM_TYPEMOUSE)
			{
				RAWMOUSE& RawMouse = Raw->data.mouse;
				i32 OffsetX = RawMouse.lLastX;
				i32 OffsetY = RawMouse.lLastY;

				if (OffsetX || OffsetY)
				{
					Inputs.XPos += OffsetX;
					Inputs.YPos += OffsetY;
				}

				if (RawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
				{
					Inputs.LeftClickHeld = true;
				}

				if (RawMouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
				{
					Inputs.LeftClickHeld = false;
				}

				if (RawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
				{
					Inputs.RightClickHeld = true;
				}

				if (RawMouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
				{
					Inputs.RightClickHeld = false;
				}

				if (RawMouse.usButtonFlags & RI_MOUSE_WHEEL)
				{
					SHORT ScrollDelta = (SHORT)RawMouse.usButtonData;
					Inputs.ScrollDelta = ScrollDelta;
				}
			}
			else if (Raw->header.dwType == RIM_TYPEKEYBOARD)
			{
				RAWKEYBOARD KB = Raw->data.keyboard;

				USHORT Key = KB.VKey;
				USHORT Flags = KB.Flags;

				bool IsKeyDown = !(Flags & RI_KEY_BREAK);

				if (Key < 256)
				{
					Inputs.KeysDown[Key] = IsKeyDown;
				}
			}
		}

		break;
	}
	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}


static window_context InitializeWin32Window(u32 Width, u32 Height)
{
	window_context Context = {};

	WNDCLASSEX wc     = { 0 };
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc    = AppWndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = GetModuleHandle(NULL);
	wc.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName   = NULL;
	wc.lpszClassName  = "Main Window";
	wc.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Error registering class", "Error", MB_OK | MB_ICONERROR);
		Context.Valid = false;
		return Context;;
	}

	Context.Width = Width;
	Context.Height = Height;
	Context.Instance = wc.hInstance;
	Context.Handle = CreateWindowEx(
		0, "Main Window", "Project Math 2", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL,
		NULL, wc.hInstance, NULL
	);

	if (!Context.Handle)
	{
		MessageBox(NULL, "Error registering class", "Error", MB_OK | MB_ICONERROR);
		Context.Valid = false;
		return Context;;
	}

	i32 CmdShow             = SW_SHOWDEFAULT;
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb          = sizeof(STARTUPINFO);
	
	GetStartupInfo(&startupInfo);
	if (startupInfo.dwFlags & STARTF_USESHOWWINDOW)
	{
		CmdShow = startupInfo.wShowWindow;
	}
	ShowWindow(Context.Handle, CmdShow);

	Context.Valid = true;
	return Context;
}

// TODO: 
// 1) Fix the input bug
// 2) Finish the document
// 3) CBA Tests (Maybe?)
// 4) Send the project.
int main()
{
	const u32 Width       = 1920;
	const u32 Height      = 1080;
	const u32 RefreshRate = 120;

	Window = InitializeWin32Window(Width, Height);
	bool Running          = true;

	if (Window.Valid)
	{
		f32 TargetSecondsPerFrame = 1.0f / RefreshRate;

		LARGE_INTEGER LastOSCounter = GetWallClock();
		LARGE_INTEGER OSFrequency;
		QueryPerformanceFrequency(&OSFrequency);

		RAWINPUTDEVICE Rid[2] = { 0 };
		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage = 0x02;
		Rid[0].dwFlags = 0;
		Rid[0].hwndTarget = 0;

		Rid[1].usUsagePage = 0x01;
		Rid[1].usUsage = 0x06;
		Rid[1].dwFlags = 0;
		Rid[1].hwndTarget = 0;

		if (RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])) == FALSE)
		{
			printf("Failed to resigter for raw inputs.\n");
			return 0;
		}

		RECT Client = {};
		GetClientRect(Window.Handle, &Client);
		u32 RealWidth  = Client.right - Client.left;
		u32 RealHeight = Client.bottom - Client.top;

		InitializeRendererBackend(RealWidth, RealHeight, RefreshRate, Window.Handle);
		Backend.DrawList  = InitializeDrawList();

		f32 AspectRatio             = (f32)Width / Height;
		f32 FieldOfView             = 90.0f;
		vec_3 CameraDefaultPosition = vec_3(0.0f, 0.0f, -1.0f);
		InitializeCamera(AspectRatio, FieldOfView, CameraDefaultPosition);

		InitializeEntityManager();

		Initialize3DSpace();
		InitializeSimulationUI(Window.Handle, Backend.Device, Backend.ImmediateContext);

		LoopStart:
		while (Running)
		{
			MSG Message;
			while (PeekMessage(&Message, NULL, 0, 0, PM_REMOVE))
			{
				if (Message.message == WM_QUIT)
				{
					Running = false;
					goto LoopStart;
				}

				TranslateMessage(&Message);
				DispatchMessage(&Message);
			}

			RenderSimulationUI();

			UpdateEntities();
			UpdateSpace();

			RenderAppFrame();

			Inputs.LastX = Inputs.XPos;
			Inputs.LastY = Inputs.YPos;
			Inputs.ScrollDelta = 0;		

			LARGE_INTEGER EndOSCounter = GetWallClock();
			f32 SecondsElapsedForFrame = (f32)(EndOSCounter.QuadPart - LastOSCounter.QuadPart) / OSFrequency.QuadPart;
			f32 S = (TargetSecondsPerFrame - SecondsElapsedForFrame) * 1000;
			if (S > 1.0f)
			{
				Sleep((DWORD)S);
			}

			LastOSCounter = GetWallClock();
		}	

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	else
	{
		printf("Failed to initialize the window.\n");
		return 0;
	}
}