#include <windows.h>

#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"

struct FVertexSimple
{
	float x, y, z;
	float r, g, b, a;
};

struct FVector3
{
	float x, y, z;
	FVector3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

	FVector3 operator-(FVector3 other)
	{
		FVector3 result;
		result = { x - other.x, y - other.y, z - other.z };
		return result;
	}

	FVector3 operator+(FVector3 other)
	{
		FVector3 result;
		result = { x + other.x, y + other.y, z + other.z };
		return result;
	}

	FVector3 operator*(float tmp) const {
		FVector3 result;
		result = { x * tmp, y * tmp, z * tmp };
		return result;
	}

	float Dot(FVector3 other)
	{
		return x * other.x + y * other.y + z * other.z;
	}

	float Length()
	{
		return sqrt(x * x + y * y + z * z);
	}

	FVector3 Normalize()
	{
		FVector3 normal;
		float len = Length();
		if (len > 0.0f)
		{
			normal = { x / len, y / len, z / len };
		}
		else
		{
			normal = { 0, 0, 0 };
		}

		return normal;
	}
};

class URender
{
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* Devicecontext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	ID3D11Texture2D* FrameBuffer = nullptr; 
	ID3D11RasterizerState* RasterizerState = nullptr; 
	ID3D11RenderTargetView* FrameBufferRTV = nullptr; 
	ID3D11Buffer* ConstantBuffer = nullptr;

	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	D3D11_VIEWPORT ViewportInfo; 

	ID3D11VertexShader* SimpleVertexShader;
	ID3D11PixelShader* SimplePixelShader;
	ID3D11InputLayout* SimpleInputLayout;
	unsigned int Stride;

public:
	//초기화 함수
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);

		CreateFrameBuffer();

		CreateRasterizerState();
	}

	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };

		//스왑 체인 설정
		DXGI_SWAP_CHAIN_DESC swapinfo = {};
		swapinfo.BufferDesc.Width = 0;
		swapinfo.BufferDesc.Height = 0;
		swapinfo.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapinfo.SampleDesc.Count = 1;
		swapinfo.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapinfo.BufferCount = 2;
		swapinfo.OutputWindow = hWindow;
		swapinfo.Windowed = TRUE;
		swapinfo.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		//장치와 스왑체인 생성
		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			levels, ARRAYSIZE(levels), D3D11_SDK_VERSION,
			&swapinfo, &SwapChain, &Device, nullptr, &Devicecontext);

		//스왑체인 정보 가져오기
		SwapChain->GetDesc(&swapinfo);
		//뷰포트 설정
		ViewportInfo = { 0.0f, 0.0f, (float)swapinfo.BufferDesc.Width, (float)swapinfo.BufferDesc.Height, 0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (Devicecontext)
		{
			Devicecontext->Flush();
		}

		if (SwapChain)
		{
			SwapChain->Release();
			SwapChain = nullptr;
		}

		if (Device)
		{
			Device->Release();
			Device = nullptr;
		}

		if (Devicecontext)
		{
			Devicecontext->Release();
			Devicecontext = nullptr;
		}
	}

	void CreateFrameBuffer()
	{
		//다음 이미지 업데이트를 위해 스왑체인으로부터 백버퍼 가져오기
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		//랜더 타겟 뷰 생성
		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVinfo = {};
		framebufferRTVinfo.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		framebufferRTVinfo.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVinfo, &FrameBufferRTV);
	}

	void ReleaseFrameBuffer()
	{
		if (FrameBuffer)
		{
			FrameBuffer->Release();
			FrameBuffer = nullptr;
		}

		if (FrameBufferRTV)
		{
			FrameBufferRTV->Release();
			FrameBufferRTV = nullptr;
		}
	}

	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerinfo = {};
		rasterizerinfo.FillMode = D3D11_FILL_SOLID;
		rasterizerinfo.CullMode = D3D11_CULL_BACK;

		Device->CreateRasterizerState(&rasterizerinfo, &RasterizerState);
	}

	//모든 리소스 해제
	void Release()
	{
		RasterizerState->Release();

		Devicecontext->OMSetRenderTargets(0, nullptr, nullptr);

		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0);
	}

	void CreateShader()
	{
		ID3DBlob* vertexshaderCompiled;
		ID3DBlob* pixelshaderCompiled;

		D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "mainVS", "vs_5_0", 0, 0, &vertexshaderCompiled, nullptr);
		Device->CreateVertexShader(vertexshaderCompiled->GetBufferPointer(), vertexshaderCompiled->GetBufferSize(), nullptr, &SimpleVertexShader); 

		D3DCompileFromFile(L"Shader.hlsl", nullptr, nullptr, "mainPS", "vs_5_0", 0, 0, &pixelshaderCompiled, nullptr);
		Device->CreatePixelShader(pixelshaderCompiled->GetBufferPointer(), pixelshaderCompiled->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		};

		Device->CreateInputLayout(layout, ARRAYSIZE(layout), vertexshaderCompiled->GetBufferPointer(), vertexshaderCompiled->GetBufferSize(), &SimpleInputLayout);
		Stride = sizeof(FVertexSimple);

		vertexshaderCompiled->Release();
		pixelshaderCompiled->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout)
		{
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader)
		{
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader)
		{
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
	{
		D3D11_BUFFER_DESC vertexbufferinfo = {};
		vertexbufferinfo.ByteWidth = byteWidth;
		vertexbufferinfo.Usage = D3D11_USAGE_IMMUTABLE;
		vertexbufferinfo.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexbufferSubData = { vertices };
		ID3D11Buffer* vertexBuffer;
		Device->CreateBuffer(&vertexbufferinfo, &vertexbufferSubData, &vertexBuffer);
		
		return vertexBuffer;
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		vertexBuffer->Release();
	}

	struct FConstants
	{
		FVector3 Offset;
		float ScaleMod;
		FVector3 Color;
	};

	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferinfo = {};
		constantbufferinfo.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0; 
		constantbufferinfo.Usage = D3D11_USAGE_DYNAMIC; 
		constantbufferinfo.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferinfo.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		Device->CreateBuffer(&constantbufferinfo, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer)
		{
			ConstantBuffer->Release();
			ConstantBuffer = nullptr;
		}
	}

	void UpdateConstant(FVector3 Offset, float ScaleMod, FVector3 Color)
	{
		if (ConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

			Devicecontext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); //매프레임마다 상수버퍼 업데이트
			FConstants* constants = (FConstants*)constantbufferMSR.pData;
			{
				constants->Offset = Offset;
				constants->ScaleMod = ScaleMod;
				constants->Color = Color;
			}
			Devicecontext->Unmap(ConstantBuffer, 0);
		}
	}

	void Prepare()
	{
		Devicecontext->ClearRenderTargetView(FrameBufferRTV, ClearColor);
		Devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		Devicecontext->RSSetViewports(1, &ViewportInfo);
		Devicecontext->RSSetState(RasterizerState);
		Devicecontext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
		Devicecontext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void PrepareShader()
	{
		Devicecontext->VSSetShader(SimpleVertexShader, nullptr, 0);
		Devicecontext->PSSetShader(SimplePixelShader, nullptr, 0);
		Devicecontext->IASetInputLayout(SimpleInputLayout);

		//버텍스 셰이더에 상수 버퍼 설정
		if (ConstantBuffer)
		{
			Devicecontext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		Devicecontext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);

		Devicecontext->Draw(numVertices, 0);
	}
};

class UBall
{
public:
	FVector3 Location;
	FVector3 Velocity;
	float Radius;
	float Mass;
	UBall* Next;
	FVector3 Color;
	FVector3 Acceleration;

public:
	UBall(FVector3 inLocation, FVector3 inVelocity, float inRadius)
	{
		Location = inLocation;
		Velocity = inVelocity;
		Radius = inRadius;
		CalculateMass();
		Next = nullptr;
		Color = FVector3(1.0f, 1.0f, 1.0f);
		Acceleration = FVector3(0, 0, 0);
	}

	void CalculateMass()
	{
		float p = 3.141592f;
		float density = 1.0f;
		Mass = Radius * Radius * p * density;
	}

	double Distance(UBall* Other)
	{
		double distance;
		distance = sqrt(pow(Location.x - Other->Location.x, 2) + pow(Location.y - Other->Location.y, 2));
		return distance;
	}

	bool Intersects(UBall* Other)
	{
		double distance;
		distance = Distance(Other);

		if (distance <= Radius + Other->Radius)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

FVector3 RandomLocation()
{
	FVector3 location;
	location.x = (float)(rand() % 200 - 100) / 100.0f;
	location.y = (float)(rand() % 200 - 100) / 100.0f;
	location.z = 0.0f;
	return location;
}

FVector3 RandomVelocity()
{
	FVector3 velocity;
	velocity.x = (float)(rand() % 20 - 10) / 100.0f;
	velocity.y = (float)(rand() % 20 - 10) / 100.0f;
	velocity.z = 0.0f;
	return velocity;
}

float RandomRadius()
{
	float radius;
	radius = (float)(rand() % 10 + 10) / 100.0f;
	return radius;
}

class UBallManager
{
public:
	UBall* Head;
	int BallCount;

public:
	UBallManager()
	{
		Head = nullptr;
		BallCount = 0;
	}

	void AddBall()
	{
		FVector3 location = RandomLocation();
		FVector3 velocity = RandomVelocity();
		float radius = RandomRadius();

		UBall* newBall = new UBall(location, velocity, radius);
		newBall->Next = Head;
		Head = newBall;
		BallCount++;
	}

	void RemoveBall()
	{
		if (Head == nullptr || BallCount == 0)
		{
			return;
		}

		UINT randomIndex = rand() % BallCount;
		UBall* pre = nullptr;
		UBall* current = Head;
		for (int i = 0; i < randomIndex; ++i)
		{
			if (current == nullptr) return;
			pre = current;
			current = current->Next;
		}

		if (pre)
		{
			pre->Next = current->Next;
		}
		else
		{
			Head = current->Next;
		}

		delete current;
		current = nullptr;
		BallCount--;
		if (BallCount == 0)
		{
			Head = nullptr;
		}
	}
};

extern LRESULT ImGui_ImplWin32_WindowProcHandler(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//메시지 처리 함수
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WindowProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

int WINAPI MainWindow(HINSTANCE hInstance, HINSTANCE hPreInstance, LPSTR IpCmdLine, int nShowCmd)
{
	WCHAR WindowClass[] = L"MainWindow"; //윈도우 이름
	WCHAR Title[] = L"PopingBall"; //타이틀바 이름
	//WNDCLASSW: 윈도우를 생성할 때 필용한 정보를 담는 구조체
	WNDCLASSW WndClass = { 0, WindowProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

	//윈도우 클래스 등록
	RegisterClassW(&WndClass);

	const int ScreenWidth = 1024;
	const int ScreenHeight = 1024;
	//윈도우 생성
	HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, ScreenWidth, ScreenHeight,
		nullptr, nullptr, hInstance, nullptr);
	UBallManager BallManager;
	//render.Create(hWnd);

}