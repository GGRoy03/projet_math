#include <d3d11.h> 
#include <dxgi.h> 
#include <DirectXMath.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

#include "math/matrix.hpp"
#include "utility/types.h"
#include "utility/allocators.h"

enum UPDATE_RESOURCE_TYPE : u16
{
	UPDATE_RESOURCE_NONE = 1 << 0,

	UPDATE_RESOURCE_RECREATE = 1 << 1,
	UPDATE_RESOURCE_DISCARD = 1 << 2,
	UPDATE_RESOURCE_NO_DISCARD = 1 << 3,
};

struct mesh_info
{
	bump_allocator VertexBuffer;
	bump_allocator IndexBuffer;
};

struct render_pipeline
{
	size_t                   AttributesStride;
	D3D11_PRIMITIVE_TOPOLOGY Topology;
	ID3D11InputLayout*       Layout;
	ID3D11VertexShader*      VertexShader;
	ID3D11PixelShader*       PixelShader;
};

struct rendering_context
{
	bool Valid;
	ID3D11Device* Device;
	ID3D11DeviceContext* ImmediateContext;
	IDXGISwapChain* SwapChain;
	D3D11_VIEWPORT Viewport;
	ID3D11RenderTargetView* RenderTargetView;
	ID3D11DepthStencilView* DepthAndStencil;
	render_pipeline* LastState;
};

struct dx11_vertex_buffer
{
	u32 Stride;
	ID3D11Buffer* Buffer;
};

struct object_data
{
	mat_4 Transform;
};

struct shared_object_data
{
	mat_4 View;
	mat_4 Projection;
};

struct draw_vertex
{
	vec_3 Pos;
	vec_2 Uv;
	vec_3 Normal;
};

struct draw_command
{
	u32 VertexOffset;
	u32 IndexOffset;
	u32 ElementCount;

	u32 ObjectDataKey;
	u32 InstanceDataKey;

	render_pipeline* Pipeline;
};

struct draw_list
{
	bump_allocator VertexBuffer;
	bump_allocator IndexBuffer;
	bump_allocator CommandBuffer;

	u64 FrameVertexCount;
	u64 FrameIndexCount;
};

struct instance_buffer
{
	ID3D11Buffer* Buffer;
	ID3D11ShaderResourceView* SRV;
	u32 Stride;
	u32 Count;
};

// TODO: Define max for these
struct resource_manager
{
	ID3D11Buffer*   ObjectDataBuffers[100];
	instance_buffer InstanceDataBuffers[100];
	render_pipeline Pipelines[100];
	mesh_info       Meshes[100];

	i32 ObjectResourceCount;
	i32 InstanceResourceCount;
	i32 PipelinesCount;
	i32 MeshesCount;
};

struct backend_state
{
	ID3D11Device* Device;
	ID3D11DeviceContext* ImmediateContext;
	IDXGISwapChain* SwapChain;
	D3D11_VIEWPORT Viewport;
	ID3D11RenderTargetView* RenderTargetView;
	ID3D11DepthStencilView* DepthAndStencil;
	render_pipeline* LastState;

	ID3D11Buffer* VertexBuffer;
	ID3D11Buffer* IndexBuffer;

	resource_manager Resources;
	draw_list DrawList;

	u64 IndexBufferSize;
	u64 VertexBufferSize;
};



static backend_state Backend;

#include "directx/dx11_camera.cpp"
#include "directx/dx11_shaders.cpp"

static mesh_info* LoadMesh(const char* MeshPath)
{
	char CwdPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, CwdPath);

	char AssetRoot[MAX_PATH] = {};
	snprintf(AssetRoot, MAX_PATH, "%s\\assets\\", CwdPath);

	u32 RootLength = StringLength(AssetRoot);

	char PathBuffer[256] = {};
	snprintf(PathBuffer, 256, "%s%s", AssetRoot, MeshPath);

	FILE* File;
	fopen_s(&File, PathBuffer, "rb");

	ASSERT(File != nullptr, "Invalid path in the asset table? | Path: %s", PathBuffer);

	tka_header Header = {};
	fread(&Header, sizeof(tka_header), 1, File);

	i32 MeshKey = Backend.Resources.MeshesCount;
	mesh_info* Mesh = &Backend.Resources.Meshes[MeshKey];
	Backend.Resources.MeshesCount++;

	u32 AllocationSize = Header.AllocationSize;
	u32 VertexDataSize = Header.DataSize;
	u32 IndexBufferSize = AllocationSize - VertexDataSize;

	Mesh->VertexBuffer = CreateBumpAllocator(VertexDataSize);
	Mesh->VertexBuffer.At = VertexDataSize;

	Mesh->IndexBuffer = CreateBumpAllocator(IndexBufferSize);
	Mesh->IndexBuffer.At = IndexBufferSize;

	fread(Mesh->VertexBuffer.Memory, 1, VertexDataSize, File);
	fread(Mesh->IndexBuffer.Memory, 1, IndexBufferSize, File);

	return Mesh;
}

static size_t FindBufferStride(SHADER_IN_DATA_TYPE InputType)
{
	switch (InputType)
	{
	case SHADER_IN_NONE:
		return 0;
	case SHADER_IN_POS_UV_NORM_INSTANCE_POS:
	case SHADER_IN_POS_UV_NORM:
		return 8 * sizeof(f32);
	default:
		return 0;
	}
}

static void BackendWindowResize(HWND hwnd, i32 Width, i32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	if (Backend.RenderTargetView) 
	{
		Backend.RenderTargetView->Release();
		Backend.RenderTargetView = nullptr;
	}

	if (Backend.DepthAndStencil) 
	{
		Backend.DepthAndStencil->Release();
		Backend.DepthAndStencil = nullptr;
	}

	HRESULT hr = Backend.SwapChain->ResizeBuffers(
		0,
		Width,
		Height,
		DXGI_FORMAT_UNKNOWN,
		0
	);

	if (FAILED(hr)) 
	{
		return;
	}

	ID3D11Texture2D* BackBuffer = nullptr;
	hr = Backend.SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	if (SUCCEEDED(hr)) 
	{
		hr = Backend.Device->CreateRenderTargetView(BackBuffer, nullptr, &Backend.RenderTargetView);
		BackBuffer->Release();
	}

	D3D11_TEXTURE2D_DESC DepthDesc = {};
	DepthDesc.Width = Width;
	DepthDesc.Height = Height;
	DepthDesc.MipLevels = 1;
	DepthDesc.ArraySize = 1;
	DepthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthDesc.SampleDesc.Count = 1;
	DepthDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ID3D11Texture2D* depthStencil = nullptr;
	hr = Backend.Device->CreateTexture2D(&DepthDesc, nullptr, &depthStencil);
	if (SUCCEEDED(hr)) {
		hr = Backend.Device->CreateDepthStencilView(depthStencil, nullptr, &Backend.DepthAndStencil);
		depthStencil->Release();
	}

	Backend.Viewport.Width  = (f32)Width;
	Backend.Viewport.Height = (f32)Height;
	Backend.ImmediateContext->RSSetViewports(1, &Backend.Viewport);

	Backend.ImmediateContext->OMSetRenderTargets(1, &Backend.RenderTargetView, Backend.DepthAndStencil);
}

static void BindRenderingPipeline(render_pipeline* Pipeline)
{
	Backend.ImmediateContext->IASetPrimitiveTopology(Pipeline->Topology);
	Backend.ImmediateContext->IASetInputLayout(Pipeline->Layout);
	Backend.ImmediateContext->VSSetShader(Pipeline->VertexShader, nullptr, 0);
	Backend.ImmediateContext->PSSetShader(Pipeline->PixelShader, nullptr, 0);
}

static u32 CreateObjectResource(void* Resource, size_t ResourceSize)
{
	u32 Key                       = Backend.Resources.ObjectResourceCount;
	ID3D11Buffer** ResourceBuffer = &Backend.Resources.ObjectDataBuffers[Key];

	D3D11_BUFFER_DESC Desc = {};
	Desc.Usage             = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth         = ResourceSize;
	Desc.BindFlags         = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

	ASSERT(SUCCEEDED(Backend.Device->CreateBuffer(&Desc, nullptr, ResourceBuffer)),
		   "Failed to create a resource. Memory corruption?");

	D3D11_MAPPED_SUBRESOURCE InitialData = {};
	ASSERT(SUCCEEDED(Backend.ImmediateContext->Map(*ResourceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &InitialData)),
		   "Failed to map the buffer. Memory corruption?");

	memcpy(InitialData.pData, Resource, ResourceSize);
	Backend.ImmediateContext->Unmap(*ResourceBuffer, 0);

	Backend.Resources.ObjectResourceCount++;

	return Key;
}

static u32 CreateInstancedResource(u32 InstanceCount, void* Resource, size_t SizePerInstance)
{
	u32              ResourceSize   = InstanceCount * SizePerInstance;;
	u32              Key            = Backend.Resources.InstanceResourceCount;
	instance_buffer* InstanceBuffer = &Backend.Resources.InstanceDataBuffers[Key];

	D3D11_BUFFER_DESC Desc   = {};
	Desc.Usage               = D3D11_USAGE_DYNAMIC;
	Desc.ByteWidth           = ResourceSize;
	Desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	Desc.StructureByteStride = SizePerInstance;

	ASSERT(SUCCEEDED(Backend.Device->CreateBuffer(&Desc, nullptr, &InstanceBuffer->Buffer)),
		   "Failed to create a resource. Memory corruption?");

	D3D11_MAPPED_SUBRESOURCE InitialData = {};
	ASSERT(SUCCEEDED(Backend.ImmediateContext->Map(InstanceBuffer->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &InitialData)),
		  "Failed to map the buffer. Memory corruption?");

	memcpy(InitialData.pData, Resource, ResourceSize);
	Backend.ImmediateContext->Unmap(InstanceBuffer->Buffer, 0);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format                          = DXGI_FORMAT_UNKNOWN;
	SRVDesc.ViewDimension                   = D3D11_SRV_DIMENSION_BUFFER;
	SRVDesc.Buffer.FirstElement             = 0;
	SRVDesc.Buffer.NumElements              = InstanceCount;

	ASSERT(SUCCEEDED(Backend.Device->CreateShaderResourceView(InstanceBuffer->Buffer, &SRVDesc, &InstanceBuffer->SRV)),
		   "Failed to create a SRV for an instance buffer.");

	InstanceBuffer->Stride = SizePerInstance; 
	InstanceBuffer->Count  = InstanceCount;

	Backend.Resources.InstanceResourceCount++;

	return Key;
}

static void UpdateInstanceData(u32 InstanceResourceKey, void* Resource, size_t ResourceSize,
	                           u32 ResourceCount, size_t ResourceOffset, u16 UpdateFlags)
{
	instance_buffer* InstanceBuffer = &Backend.Resources.InstanceDataBuffers[InstanceResourceKey];

	if (UpdateFlags & UPDATE_RESOURCE_RECREATE)
	{
		if (InstanceBuffer->Buffer)
		{
			InstanceBuffer->Buffer->Release();
		}
		if (InstanceBuffer->SRV)
		{
			InstanceBuffer->SRV->Release();
		}

		u32 SizePerInstance = ResourceSize;
		u32 ByteWidth       = ResourceCount * SizePerInstance;

		D3D11_BUFFER_DESC Desc = {};
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.ByteWidth = ByteWidth;
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		Desc.StructureByteStride = SizePerInstance;

		ASSERT(SUCCEEDED(Backend.Device->CreateBuffer(&Desc, nullptr, &InstanceBuffer->Buffer)),
			   "Failed to create a resource. Memory corruption?");

		D3D11_MAPPED_SUBRESOURCE InitialData = {};
		ASSERT(SUCCEEDED(Backend.ImmediateContext->Map(InstanceBuffer->Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &InitialData)),
			   "Failed to map the buffer. Memory corruption?");

		memcpy(InitialData.pData, Resource, ByteWidth);
		Backend.ImmediateContext->Unmap(InstanceBuffer->Buffer, 0);

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = ResourceCount;

		ASSERT(SUCCEEDED(Backend.Device->CreateShaderResourceView(InstanceBuffer->Buffer, &SRVDesc, &InstanceBuffer->SRV)),
			   "Failed to create a SRV for an instance buffer.");

		InstanceBuffer->Stride = ResourceSize;
		InstanceBuffer->Count = ResourceCount;
	}
	else if (UpdateFlags & UPDATE_RESOURCE_DISCARD)
	{
		ASSERT(InstanceBuffer->Buffer, "NO BUFFER BOUND FOR UPDATE RESOURCE WITH KEY: %d", InstanceResourceKey);

		D3D11_MAPPED_SUBRESOURCE InstanceData = {};
		HRESULT Status = Backend.ImmediateContext->Map(InstanceBuffer->Buffer, 0, D3D11_MAP_WRITE_DISCARD,
			                                           0, &InstanceData);

		ASSERT(SUCCEEDED(Status), "FAILED TO MAP AN INSTANCE BUFFER WITH KEY: %d", InstanceResourceKey);

		u32 BufferWidth = InstanceBuffer->Count * InstanceBuffer->Stride;
		memcpy((char*)InstanceData.pData, Resource, BufferWidth);
		Backend.ImmediateContext->Unmap(InstanceBuffer->Buffer, 0);
	}
	else if (UpdateFlags & UPDATE_RESOURCE_NO_DISCARD)
	{
		ASSERT(InstanceBuffer->Buffer, "NO BUFFER BOUND FOR UPDATE RESOURCE WITH KEY: %d", InstanceResourceKey);

		D3D11_MAPPED_SUBRESOURCE InstanceData = {};
		HRESULT Status = Backend.ImmediateContext->Map(InstanceBuffer->Buffer, 0, D3D11_MAP_WRITE_NO_OVERWRITE,
			                                           0, &InstanceData);

		ASSERT(SUCCEEDED(Status), "FAILED TO MAP AN INSTANCE BUFFER WITH KEY: %d", InstanceResourceKey);

		memcpy((char*)InstanceData.pData + ResourceOffset, Resource, ResourceSize);
		Backend.ImmediateContext->Unmap(InstanceBuffer->Buffer, 0);
	}
}

static void UpdateObjectData(u32 ResourceKey, void* Resource, size_t ResourceSize, u16 UpdateFlags)
{
	ID3D11Buffer* ObjectBuffer = Backend.Resources.ObjectDataBuffers[ResourceKey];

	if (UpdateFlags & UPDATE_RESOURCE_RECREATE)
	{
		
	}
	else if (UpdateFlags & UPDATE_RESOURCE_DISCARD)
	{
		ASSERT(ObjectBuffer, "NO BUFFER BOUND FOR UPDATE RESOURCE WITH KEY: %d", ResourceKey);

		D3D11_MAPPED_SUBRESOURCE InstanceData = {};
		HRESULT Status = Backend.ImmediateContext->Map(ObjectBuffer, 0, D3D11_MAP_WRITE_DISCARD,
			                                           0, &InstanceData);

		ASSERT(SUCCEEDED(Status), "FAILED TO MAP AN INSTANCE BUFFER WITH KEY: %d", ResourceKey);

		memcpy((char*)InstanceData.pData, Resource, ResourceSize);
		Backend.ImmediateContext->Unmap(ObjectBuffer, 0);
	}
	else if (UpdateFlags & UPDATE_RESOURCE_NO_DISCARD)
	{
		
	}
}

static render_pipeline* CreateRenderPipeline(pipeline_info PipelineInfo)
{
	i32 PipelineKey           =  Backend.Resources.PipelinesCount;
	render_pipeline* Pipeline = &Backend.Resources.Pipelines[PipelineKey];
	Backend.Resources.PipelinesCount++;

	Pipeline->Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	dx11_shader_chain Shaders  = CreateShaderChain(PipelineInfo.Shaders, PipelineInfo.ShaderInputType);
	Pipeline->VertexShader     = Shaders.Vertex;
	Pipeline->PixelShader      = Shaders.Pixel;
	Pipeline->Layout           = Shaders.Layout;
	Pipeline->AttributesStride = FindBufferStride(PipelineInfo.ShaderInputType);
	
	return Pipeline;
}

static void InitializeRendererBackend(u32 Width, u32 Height, u32 FrameRate, HWND hwnd)
{
	HRESULT Status = S_OK;

	DXGI_MODE_DESC BufferDesc;
	ZeroMemory(&BufferDesc, sizeof(DXGI_MODE_DESC));
	BufferDesc.Width = Width;
	BufferDesc.Height = Height;
	BufferDesc.RefreshRate.Numerator = FrameRate;
	BufferDesc.RefreshRate.Denominator = 1;
	BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	ZeroMemory(&SwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	SwapChainDesc.BufferDesc = BufferDesc;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.OutputWindow = hwnd;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	Status = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&SwapChainDesc,
		&Backend.SwapChain,
		&Backend.Device,
		NULL,
		&Backend.ImmediateContext
	);

	D3D11_TEXTURE2D_DESC DepthStencilDesc = { 0 };
	DepthStencilDesc.Width = (UINT)Width;
	DepthStencilDesc.Height = (UINT)Height;
	DepthStencilDesc.MipLevels = 1;
	DepthStencilDesc.ArraySize = 1;
	DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DepthStencilDesc.SampleDesc.Count = 1;
	DepthStencilDesc.SampleDesc.Quality = 0;
	DepthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	DepthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DepthStencilDesc.CPUAccessFlags = 0;
	DepthStencilDesc.MiscFlags = 0;

	ID3D11Texture2D* DepthStencilBuffer = { 0 };
	Status = Backend.Device->CreateTexture2D(&DepthStencilDesc, NULL, &DepthStencilBuffer);
	ASSERT(SUCCEEDED(Status), "Failed to initialize backend.");

	Status = Backend.Device->CreateDepthStencilView(DepthStencilBuffer, NULL, &Backend.DepthAndStencil);
	ASSERT(SUCCEEDED(Status), "Failed to initialize backend.");

	D3D11_VIEWPORT Viewport = { 0 };
	ZeroMemory(&Viewport, sizeof(D3D11_VIEWPORT));
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = (FLOAT)Width;
	Viewport.Height = (FLOAT)Height;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	Backend.Viewport = Viewport;

	ID3D11Texture2D* BackBuffer = { 0 };
	Status = Backend.SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	ASSERT(SUCCEEDED(Status), "Failed to initialize backend.");

	Status = Backend.Device->CreateRenderTargetView(BackBuffer, NULL, &Backend.RenderTargetView);
	ASSERT(SUCCEEDED(Status), "Failed to initialize backend.");

	BackBuffer->Release();

	Backend.ImmediateContext->OMSetRenderTargets(1, &Backend.RenderTargetView, Backend.DepthAndStencil);
	Backend.ImmediateContext->RSSetViewports(1, &Backend.Viewport);

	Backend.Resources.ObjectResourceCount   = 1;
	Backend.Resources.InstanceResourceCount = 1;
}

static draw_list InitializeDrawList()
{
	draw_list List      = {};
	List.VertexBuffer   = CreateBumpAllocator(Kilobytes(5), BUMP_RESIZABLE, "Global Vertex Buffer");
	List.IndexBuffer    = CreateBumpAllocator(Kilobytes(5), BUMP_RESIZABLE, "Global Index Buffer");
	List.CommandBuffer  = CreateBumpAllocator(Kilobytes(5), BUMP_RESIZABLE, "Global Command Buffer");

	return List;
}

static void PushDrawCommand(u32 ObjectResourceKey, u32 InstancedDataKey, mesh_info* Info,
	                        render_pipeline* Pipeline)
{
	draw_list* List      = &Backend.DrawList;
	u32 IndexOffset      = List->IndexBuffer.At / sizeof(u32);
	u32 AttributesOffset = List->VertexBuffer.At / sizeof(draw_vertex);
	u32 IndexCount       = GetElementsCount(&Info->IndexBuffer, sizeof(u32));
	u32 VertexCount      = GetElementsCount(&Info->VertexBuffer, sizeof(draw_vertex));

	draw_command Command    = {};
	Command.ElementCount    = IndexCount;
	Command.IndexOffset     = IndexOffset;
	Command.VertexOffset    = AttributesOffset;
	Command.ObjectDataKey   = ObjectResourceKey;
	Command.InstanceDataKey = InstancedDataKey;
	Command.Pipeline        = Pipeline;

	PushAndCopy(sizeof(draw_command), &Command                , &List->CommandBuffer);
	PushAndCopy(Info->IndexBuffer.At , Info->IndexBuffer.Memory , &List->IndexBuffer);
	PushAndCopy(Info->VertexBuffer.At, Info->VertexBuffer.Memory, &List->VertexBuffer);

	List->FrameIndexCount  += IndexCount;
	List->FrameVertexCount += VertexCount;
}

static void RenderAppFrame()
{
	draw_list* List        = &Backend.DrawList;

	if (!Backend.VertexBuffer || Backend.VertexBufferSize < List->FrameVertexCount)
	{
		if (Backend.VertexBuffer)
		{ 
			Backend.VertexBuffer->Release();
			Backend.VertexBuffer = nullptr;
		}

		Backend.VertexBufferSize = List->FrameVertexCount + 500;

		D3D11_BUFFER_DESC Desc = {};
		Desc.Usage             = D3D11_USAGE_DYNAMIC;
		Desc.ByteWidth         = Backend.VertexBufferSize * sizeof(draw_vertex);
		Desc.BindFlags         = D3D11_BIND_VERTEX_BUFFER;
		Desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

		if (Backend.Device->CreateBuffer(&Desc, nullptr, &Backend.VertexBuffer) < 0)
		{
			return;
		}
	}

	if (!Backend.IndexBuffer || Backend.IndexBufferSize < List->FrameIndexCount)
	{
		if (Backend.IndexBuffer)
		{
			Backend.IndexBuffer->Release();
			Backend.IndexBuffer = nullptr;
		}

		Backend.IndexBufferSize = List->FrameIndexCount + 500;

		D3D11_BUFFER_DESC Desc = {};
		Desc.Usage             = D3D11_USAGE_DYNAMIC;
		Desc.ByteWidth         = Backend.IndexBufferSize * sizeof(u32);
		Desc.BindFlags         = D3D11_BIND_INDEX_BUFFER;
		Desc.CPUAccessFlags    = D3D11_CPU_ACCESS_WRITE;

		if (Backend.Device->CreateBuffer(&Desc, nullptr, &Backend.IndexBuffer) < 0)
		{
			return;
		}
	}

	D3D11_MAPPED_SUBRESOURCE VertexResource = {};
	if (Backend.ImmediateContext->Map(Backend.VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &VertexResource) != S_OK)
	{
		return;
	}
	memcpy(VertexResource.pData, List->VertexBuffer.Memory, List->VertexBuffer.At);
	Backend.ImmediateContext->Unmap(Backend.VertexBuffer, 0);

	D3D11_MAPPED_SUBRESOURCE IndexResource = {};
	if (Backend.ImmediateContext->Map(Backend.IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &IndexResource) != S_OK)
	{
		return;
	}
	memcpy(IndexResource.pData, List->IndexBuffer.Memory, List->IndexBuffer.At);
	Backend.ImmediateContext->Unmap(Backend.IndexBuffer, 0);

	u32 Stride = sizeof(draw_vertex);
	u32 Offset = 0;
	Backend.ImmediateContext->IASetVertexBuffers(0, 1, &Backend.VertexBuffer, &Stride, &Offset);
	Backend.ImmediateContext->IASetIndexBuffer(Backend.IndexBuffer, DXGI_FORMAT_R32_UINT, 0);

	if (!ImGui::GetIO().WantCaptureMouse)
	{
		bool ShouldUpdateCamera = UpdateProjectionCamera();
		if (ShouldUpdateCamera)
		{
			// TODO: Use the function for verbosity
			D3D11_MAPPED_SUBRESOURCE MappedData = {};
			HRESULT Status = Backend.ImmediateContext->Map(Camera.Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedData);
			if (SUCCEEDED(Status))
			{
				shared_object_data SharedData = {};
				SharedData.View = Camera.ViewMatrix;
				SharedData.Projection = Camera.Projection;

				memcpy(MappedData.pData, &SharedData, sizeof(shared_object_data));
				Backend.ImmediateContext->Unmap(Camera.Buffer, 0);
			}
		}
	}

	const f32 ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	Backend.ImmediateContext->ClearRenderTargetView(Backend.RenderTargetView, ClearColor);
	Backend.ImmediateContext->ClearDepthStencilView(Backend.DepthAndStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	Backend.ImmediateContext->VSSetConstantBuffers(SHARED_OBJECT_DATA_SLOT, 1, &Camera.Buffer);

	render_pipeline* LastPipeline = Backend.LastState;
	u32              CommandCount = List->CommandBuffer.At / sizeof(draw_command);

	for (u32 CommandIndex = 0; CommandIndex < CommandCount; CommandIndex++)
	{
		draw_command* Command       = (draw_command*)List->CommandBuffer.Memory + CommandIndex;
		u32           InstanceCount = 0;

		if (Command->Pipeline != LastPipeline)
		{
			BindRenderingPipeline(Command->Pipeline);
		}

		u32 ObjectKey = Command->ObjectDataKey;
		if (ObjectKey > 0)
		{
			ID3D11Buffer* ObjectResource = Backend.Resources.ObjectDataBuffers[ObjectKey];
			Backend.ImmediateContext->VSSetConstantBuffers(OBJECT_DATA_SLOT, 1, &ObjectResource);
		}

		u32 InstanceKey = Command->InstanceDataKey;
		if (InstanceKey > 0)
		{
			instance_buffer* InstanceBuffer = &Backend.Resources.InstanceDataBuffers[InstanceKey];
			ID3D11ShaderResourceView* SRV   = InstanceBuffer->SRV;
			InstanceCount                   = InstanceBuffer->Count;

			Backend.ImmediateContext->VSSetShaderResources(INSTANCE_DATA_SLOT, 1, &SRV);
		}
		
		if (InstanceCount > 0)
		{
			Backend.ImmediateContext->DrawIndexedInstanced(Command->ElementCount, InstanceCount, Command->IndexOffset, Command->VertexOffset, 0);
		}
		else
		{
			Backend.ImmediateContext->DrawIndexed(Command->ElementCount, Command->IndexOffset, Command->VertexOffset);
		}

		LastPipeline = Command->Pipeline;
	}

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	Backend.SwapChain->Present(0, 0);
	
	ClearAllocator(&List->VertexBuffer);
	ClearAllocator(&List->IndexBuffer);
	ClearAllocator(&List->CommandBuffer);

	List->FrameIndexCount  = 0;
	List->FrameVertexCount = 0;

	Backend.LastState = LastPipeline;
}