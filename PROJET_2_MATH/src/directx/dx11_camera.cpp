#include "utility/inputs.hpp"

#include "math/vector.hpp"
#include "math/matrix.hpp"

struct dx11_projection_camera
{
	u8 UpdateFlag;

	f32 NearPlane;
	f32 FarPlane;
	f32 AspectRatio;
	f32 FOV;

	vec_3 Pos;
	vec_3 Direction;
	vec_3 Right;
	vec_3 Up;

	f32 Yaw;
	f32 Pitch;
	f32 Roll;

	mat_4 ViewMatrix;
	mat_4 Projection;

	ID3D11Buffer* Buffer;
};

static dx11_projection_camera Camera;

struct camera_frame_data
{
	vec_3 Movement;
	f32   ZoomDelta;
	f32   YawDelta;
	f32   PitchDelta;
};


static vec_3 ComputeCameraDirection(f32 Yaw, f32 Pitch)
{
	vec_3 Direction;
	Direction.x = cosf(Yaw) * cosf(Pitch);
	Direction.y = sinf(Pitch);
	Direction.z = sinf(Yaw) * cosf(Pitch);
	return Normalize(Direction);
}

static vec_3 ComputeCameraRight(vec_3 Direction, vec_3 Up)
{
	vec_3 Right = Normalize(VectorProduct(Up, Direction));
	return Right;
}

static vec_3 ComputeCameraUp(vec_3 Right, vec_3 Direction)
{
	vec_3 Up = Normalize(VectorProduct(Direction, Right));
	return Up;
}

static void InitializeCamera(f32 AspectRatio, f32 FovDegree, vec_3 CameraPosition)
{
	Camera.Yaw       = F_PI / 2;
	Camera.Pitch     = 0.0f;
	Camera.Direction = ComputeCameraDirection(Camera.Yaw, Camera.Pitch);

	vec_3 Up         = vec_3(0.0f, 1.0f, 0.0f);
	vec_3 FocusPoint = CameraPosition + Camera.Direction;

	Camera.Right = ComputeCameraRight(Camera.Direction, Up);
	Camera.Up    = ComputeCameraUp(Camera.Right, Camera.Direction);

	D3D11_BUFFER_DESC CameraBufferDesc = { 0 };
	CameraBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	CameraBufferDesc.ByteWidth = sizeof(shared_object_data);
	CameraBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	CameraBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT Status = Backend.Device->CreateBuffer(&CameraBufferDesc, NULL, &Camera.Buffer);
	if (FAILED(Status))
	{
		return;
	}

	f32 NearPlane = 0.1f;
	f32 FarPlane = 100.0f;

	mat_4 View       = FocusMatrix(Up, CameraPosition, FocusPoint);
	mat_4 Projection = ProjectionMatrix(FovDegree, AspectRatio, 0.1f, 100.0f);

	D3D11_MAPPED_SUBRESOURCE MappedData;
	Status = Backend.ImmediateContext->Map(Camera.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedData);
	if (Status == S_OK)
	{
		shared_object_data SharedData;
		SharedData.View = View;
		SharedData.Projection = Projection;

		memcpy(MappedData.pData, &SharedData, sizeof(shared_object_data));
		Backend.ImmediateContext->Unmap(Camera.Buffer, 0);
	}

	Camera.NearPlane   = NearPlane;
	Camera.FarPlane    = FarPlane;
	Camera.FOV         = FovDegree;
	Camera.AspectRatio = AspectRatio;
	Camera.ViewMatrix  = View;
	Camera.Projection  = Projection;
	Camera.Pos         = CameraPosition;
}

static camera_frame_data BuildCameraFrameData()
{
	camera_frame_data Frame = {};
	ImGuiIO& IO             = ImGui::GetIO();
	f32 CameraSpeed         = 0.05f;
	f32 ZoomStrength        = 0.05f;
	f32 GrabForce           = 0.005f;
	f32 RotationSpeed       = 0.0025f;
	f32 ScrollDelta         = Inputs.ScrollDelta;
	f32 MouseXOffset        = Inputs.LastX - Inputs.XPos;
	f32 MouseYOffset        = Inputs.YPos - Inputs.LastY;

	if (Inputs.KeysDown[W_KEY])
	{
		Frame.Movement = ScaleVector(Camera.Direction, CameraSpeed);
	}

	if (Inputs.KeysDown[S_KEY])
	{
		Frame.Movement = Frame.Movement - ScaleVector(Camera.Direction, CameraSpeed);
	}

	if (Inputs.KeysDown[A_KEY])
	{
		Frame.Movement = Frame.Movement - ScaleVector(Camera.Right, CameraSpeed);
	}

	if (Inputs.KeysDown[D_KEY])
	{
		Frame.Movement = Frame.Movement + ScaleVector(Camera.Right, CameraSpeed);
	}

	if (Inputs.LeftClickHeld)
	{
		vec_3 MovementX = ScaleVector(Camera.Right, (GrabForce * MouseXOffset));
		vec_3 MovementY = ScaleVector(Camera.Up   , (GrabForce * MouseYOffset));
		Frame.Movement  = Frame.Movement + MovementX + MovementY;
	}

	if (Inputs.RightClickHeld)
	{
		Frame.YawDelta   = MouseXOffset * RotationSpeed;
		Frame.PitchDelta = MouseYOffset * RotationSpeed;
	}

	Frame.ZoomDelta = ScrollDelta * ZoomStrength;

	return Frame;
}

static bool UpdateProjectionCamera()
{
	camera_frame_data Frame = BuildCameraFrameData();
	bool ShouldUpdate       = false;

	if (Frame.YawDelta != 0 || Frame.PitchDelta != 0)
	{
		Camera.Yaw   += Frame.YawDelta;
		Camera.Pitch += Frame.PitchDelta;

		if (Camera.Pitch > 1)
		{
			Camera.Pitch = 1;
		}
		else if (Camera.Pitch < -1)
		{
			Camera.Pitch = -1;
		}

		Camera.Direction = ComputeCameraDirection(Camera.Yaw, Camera.Pitch);
		Camera.Right     = ComputeCameraRight(Camera.Direction, vec_3(0.0f, 1.0f, 0.0f));
		Camera.Up        = ComputeCameraUp(Camera.Right, Camera.Direction);

		ShouldUpdate     = true;
	}


	if (!IsZeroVector(Frame.Movement))
	{
		Camera.Pos   = Camera.Pos + Frame.Movement;

		ShouldUpdate = true;
	}

	if (Frame.ZoomDelta != 0)
	{
		Camera.FOV  -= Frame.ZoomDelta;
		if (Camera.FOV < 15.0f)
		{
			Camera.FOV = 15.0f;
		}
		else if (Camera.FOV > 130.0f)
		{
			Camera.FOV = 130.0f;
		}
		
		ShouldUpdate = true;
	}

	if (ShouldUpdate)
	{
		vec_3 FocusPoint  = Camera.Pos + Camera.Direction;
		Camera.ViewMatrix = FocusMatrix(Camera.Up, Camera.Pos, FocusPoint);
		Camera.Projection = ProjectionMatrix(Camera.FOV, Camera.AspectRatio, Camera.NearPlane, Camera.FarPlane);
	}

	return ShouldUpdate;
}