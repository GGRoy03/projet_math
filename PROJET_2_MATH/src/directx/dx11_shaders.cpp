#include "utility/string.h"

struct dx11_shader_chain
{
	ID3D11InputLayout* Layout;
	ID3D11VertexShader* Vertex;
	ID3D11PixelShader* Pixel;
};

static dx11_shader_chain CreateShaderChain(shader_info* ShaderInfos, SHADER_IN_DATA_TYPE ShaderInputType)
{
	dx11_shader_chain ShaderChain = {};
	HRESULT Status = S_OK;
	ID3DBlob* Blob = NULL;
	wchar_t WidePath[512] = {};

	char CwdPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, CwdPath);

	char ShadersRoot[MAX_PATH] = {};
	snprintf(ShadersRoot, MAX_PATH, "%s\\shaders\\", CwdPath);
	u32 RootLength = StringLength(ShadersRoot);

	char ShaderPath[256] = {};
	char* WriteStart = ShaderPath + RootLength;
	memcpy(ShaderPath, ShadersRoot, RootLength);
	
	u32 PathLength = 0;
	u32 ShaderIndex = 0;
	shader_info ShaderInfo = ShaderInfos[ShaderIndex];
	while (ShaderInfo.Type && ShaderIndex < SHADER_TYPE_COUNT && SUCCEEDED(Status))
	{
		PathLength = StringLength(ShaderInfo.Path);
		ASSERT((PathLength + RootLength) < 256, "Shader path in asset table too long?");

		memcpy(WriteStart, ShaderInfo.Path, PathLength);
		ConvertToWide(ShaderPath, WidePath, 512);

		Status = D3DReadFileToBlob(WidePath, &Blob);

		if (FAILED(Status))
		{
			return ShaderChain;
		}

		switch (ShaderInfo.Type)
		{
		case SHADER_TYPE_VERTEX:
		{
			Status = Backend.Device->CreateVertexShader(Blob->GetBufferPointer(), Blob->GetBufferSize(),
				                                                 nullptr, &ShaderChain.Vertex);
			if (FAILED(Status)) return ShaderChain;

			D3D11_INPUT_ELEMENT_DESC Layout[10] = {};
			u32 LayoutElementCount = 0;
			switch (ShaderInputType)
			{
			case SHADER_IN_POS_UV_NORM:
				Layout[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0                           , D3D11_INPUT_PER_VERTEX_DATA, 0 };
				Layout[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				Layout[2] = { "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				LayoutElementCount = 3;
				break;
			case SHADER_IN_POS_UV_NORM_INSTANCE_POS:
				Layout[0] = { "POSITION"    , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0                           , D3D11_INPUT_PER_VERTEX_DATA, 0 };
				Layout[1] = { "TEXCOORD"    , 0, DXGI_FORMAT_R32G32_FLOAT   , 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				Layout[2] = { "NORMAL"      , 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
				Layout[3] = { "INSTANCE_POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1 };
				LayoutElementCount = 4;
				break;
			}

			Status = Backend.Device->CreateInputLayout(Layout, LayoutElementCount,
				                                                Blob->GetBufferPointer(), Blob->GetBufferSize(),
				                                                &ShaderChain.Layout);

			Status = Backend.Device->CreateVertexShader(Blob->GetBufferPointer(), Blob->GetBufferSize(),
				                                                 nullptr, &ShaderChain.Vertex);
			break;
		}
		case SHADER_TYPE_PIXEL:
			Status = Backend.Device->CreatePixelShader(Blob->GetBufferPointer(), Blob->GetBufferSize(),
				                                                nullptr, &ShaderChain.Pixel);
			break;
		}

		ShaderIndex++;
		ShaderInfo = ShaderInfos[ShaderIndex];

		memset(WriteStart, 0, PathLength);
		memset(WidePath, 0, 512);
		SAFE_RELEASE(Blob);
	}

	return ShaderChain;
}