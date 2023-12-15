// ScreenDrawDesktop.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "ScreenDrawDesktop.h"

#include <d3d11.h>
#include <D3Dcompiler.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>

int HZ = 144;
const float drawSize0 = 4.0f;
const float drawSize1 = 4.0f;
const float eraseSize = 200.0f;
const float keyEraseSize = 100.0f; // For when holding W

float downThreshold = 0.45f;
float upThreshold = 0.25f;
float ctrlDownThreshold = -1.0f;
float ctrlUpThreshold = -1.0f;

//#define CLICK_MOUSE

#define MOUSE_MIN_DIST 5.0f
#define BATCH 8

#define DRAW_COLOR_COUNT 3
DirectX::XMFLOAT4 drawColors[DRAW_COLOR_COUNT]
{
	{1.0f,0.0f,0.0f,1.0f},
	{0.0f,1.0f,0.0f,1.0f},
	{0.0f,0.0f,1.0f,1.0f},
};
int drawColorID = 0;

int WIDTH = 420; // 3440;
int HEIGHT = 69; // 1440;

struct Vertex
{
	DirectX::XMFLOAT4 position;
	DirectX::XMFLOAT4 color;
};

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <cstdio>

SOCKET listenSocket = NULL;
SOCKET clientSocket = NULL;

ID3D11Device* pDevice = nullptr;
ID3D11DeviceContext* pDeviceContext = nullptr;
IDXGISwapChain* pSwapChain = nullptr;
ID3D11RenderTargetView* pRenderTargetView = nullptr;
//ID3D11RenderTargetView* pTempRenderTargetView = nullptr;

void InitDirectX(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = WIDTH;
	swapChainDesc.BufferDesc.Height = HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = HZ;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	//swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &swapChainDesc, &pSwapChain, &pDevice, nullptr, &pDeviceContext);

	ID3D11Texture2D* pBackBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRenderTargetView);
	pBackBuffer->Release();

	//ID3D11Texture2D* pRenderTargetTexture;
	//D3D11_TEXTURE2D_DESC textureDesc = {};
	//textureDesc.Width = WIDTH;
	//textureDesc.Height = HEIGHT;
	//textureDesc.MipLevels = 1;
	//textureDesc.ArraySize = 1;
	//textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//textureDesc.SampleDesc.Count = 1;
	//textureDesc.SampleDesc.Quality = 0;
	//textureDesc.Usage = D3D11_USAGE_DEFAULT;
	//textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	//textureDesc.CPUAccessFlags = 0;
	//textureDesc.MiscFlags = 0;
	//pDevice->CreateTexture2D(&textureDesc, nullptr, &pRenderTargetTexture);
	//pDevice->CreateRenderTargetView(pRenderTargetTexture, nullptr, &pTempRenderTargetView);

	pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr);

	D3D11_VIEWPORT vp;
	vp.Width = WIDTH;
	vp.Height = HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pDeviceContext->RSSetViewports(1, &vp);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	pDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
}

const char* vertexShaderCode =
"struct VertexInput {"
"   float4 position : POSITION;"
"   float4 color : COLOR;"
"};"
"struct PixelInput {"
"   float4 position : SV_POSITION;"
"   float4 color : COLOR;"
"};"
"PixelInput main(VertexInput input) {"
"   PixelInput output;"
"   output.position = input.position;"
"   output.color = input.color;"
"   return output;"
"}";
const char* pixelShaderCode =
"struct PixelInput {"
"   float4 position : SV_POSITION;"
"   float4 color : COLOR;"
"};"
"float4 main(PixelInput input) : SV_TARGET {"
"   return input.color;"
"}";
ID3D11VertexShader* pVertexShader = NULL;
ID3D11PixelShader* pPixelShader = NULL;
ID3D11InputLayout* pInputLayout = NULL;
ID3D11RasterizerState* pRasterizerState = NULL;
ID3D11DepthStencilState* pDepthStencilState = NULL;

#define INPUT_UP 0
#define INPUT_DRAW 1
#define INPUT_ERASE 2
#define CONTINUE 0
#define DOWN 1
#define UP 2

float prevx, prevy;
bool erasing;
bool keyErase = false;
ID3D11Buffer* pVertexBuffer = nullptr;
int pVertexBufferSize = -1;
void RenderLines(std::vector<DirectX::XMFLOAT4>& positions)
{
	if (positions.size() == 0)
		return;

	float IW = 1.0f / WIDTH;
	float IH = 1.0f / HEIGHT;

	std::vector<Vertex> vertices;
	for (const auto& pos : positions)
	{
		float x = pos.x * 2.0f - 1.0f;
		float y = -(pos.y * 2.0f - 1.0f);

		if (pos.z != (float)DOWN)
		{
			float s;
			DirectX::XMFLOAT4 color{ 0.0f, 0.0f, 0.0f, 0.0f };
			if (erasing || keyErase)
			{
				s = keyErase ? keyEraseSize : eraseSize;
			}
			else
			{
				float t = pos.w;
				s = drawSize0 * (1.0f - t) + drawSize1 * t;
				color = drawColors[drawColorID];
			}

			float dirx = x - prevx;
			float diry = y - prevy;
			float dirm = sqrt(dirx * dirx + diry * diry);
			if (dirm > 0)
			{
				float m = s / dirm;
				dirx *= m;
				diry *= m;
				float perpx = -diry;
				float perpy = dirx;
				dirx *= IW;
				diry *= IH;
				perpx *= IW;
				perpy *= IH;

				vertices.push_back({ { x + perpx, y + perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { x + dirx, y + diry, 0.0f, 1.0f }, color });
				vertices.push_back({ { x - perpx, y - perpy, 0.0f, 1.0f }, color });

				vertices.push_back({ { prevx + perpx, prevy + perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { prevx - dirx, prevy - diry, 0.0f, 1.0f }, color });
				vertices.push_back({ { prevx - perpx, prevy - perpy, 0.0f, 1.0f }, color });

				vertices.push_back({ { prevx + perpx, prevy + perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { x + perpx, y + perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { x - perpx, y - perpy, 0.0f, 1.0f }, color });

				vertices.push_back({ { prevx + perpx, prevy + perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { x - perpx, y - perpy, 0.0f, 1.0f }, color });
				vertices.push_back({ { prevx - perpx, prevy - perpy, 0.0f, 1.0f }, color });
			}
		}

		prevx = x;
		prevy = y;
	}

	int s = sizeof(Vertex) * static_cast<UINT>(vertices.size());
	if (pVertexBuffer == NULL || pVertexBufferSize < s)
	{
		if (pVertexBuffer)
			pVertexBuffer->Release();
		pVertexBuffer = NULL;

		pVertexBufferSize = max(2048, s * 2);

		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bufferDesc.ByteWidth = pVertexBufferSize;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		HRESULT hr = pDevice->CreateBuffer(&bufferDesc, NULL, &pVertexBuffer);
		if (FAILED(hr) || !pVertexBuffer)
		{
			pVertexBuffer = NULL;
			std::cerr << "!pVertexBuffer. HR: " << hr << std::endl;
			return;
		}
	}

	D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT hr = pDeviceContext->Map(pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (SUCCEEDED(hr))
	{
		memcpy(mappedResource.pData, vertices.data(), s);
		pDeviceContext->Unmap(pVertexBuffer, 0);
	}
	else
	{
		std::cerr << "Couldn't map. HR: " << hr << std::endl;
		return;
	}

	//pDeviceContext->OMSetRenderTargets(1, &pRenderTargetView, nullptr); //?
	//pDeviceContext->RSSetState(pRasterizerState);
	//pDeviceContext->OMSetDepthStencilState(pDepthStencilState, 1); // 1 is the stencil reference value
	//pDeviceContext->IASetInputLayout(pInputLayout);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pDeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

	pDeviceContext->Draw(static_cast<UINT>(vertices.size()), 0);
}

std::mutex mutex; // Lock for data0
std::vector<char> data0;
std::vector<char> data1;

bool paused = true;

bool ctrling = false;
bool running = true;
bool clear = false;
void Update()
{
	//std::cerr << "Update" << std::endl;

	mutex.lock();
	data1.resize(data0.size());
	memcpy(data1.data(), data0.data(), data0.size());
	data0.clear();
	mutex.unlock();

	if (data1.size() > 0)
	{
		//std::cout << "Received: " << std::string(recvBuffer, recvResult) << std::endl;

		std::vector<DirectX::XMFLOAT4> points;

		int loc = 0;
		while (loc < ((int)data1.size() - 3) && (*((int*)(data1.data() + loc)) != 478934687))
			loc++;
		while (loc < (int)data1.size() - 5 * 4 + 1)
		{
			char* b = data1.data() + loc;
			int inputType = *((int*)b + 1);
			float x = *((float*)b + 2);
			float y = *((float*)b + 3);
			float pressure = *((float*)b + 4);

			int type = CONTINUE;
			static bool down = false;
			if (inputType == INPUT_UP)
			{
				if (down)
				{
					down = false;
					type = UP;
				}
			}
			else
			{
				if (down)
				{
					if (pressure < (ctrling ? ctrlUpThreshold : upThreshold))
					{
						down = false;
						type = UP;
					}
				}
				else
				{
					if (pressure > (ctrling ? ctrlDownThreshold : downThreshold))
					{
						down = true;
						type = DOWN;
					}
				}
			}

			if (inputType == INPUT_ERASE)
				erasing = true;
			else if (inputType == INPUT_DRAW)
				erasing = false;

			if (type != CONTINUE || down)
				points.push_back({ x, y, (float)type, pressure });

			//std::cerr << type << " - Point: " << x << ", " << y << ", pressure: " << pressure << std::endl;

			static float prevx, prevy;
			float dx = (x - prevx) * WIDTH;
			float dy = (y - prevy) * HEIGHT;
			float dist = std::sqrt(dx * dx + dy * dy);

			loc += 5 * 4;

			if (dist > MOUSE_MIN_DIST || loc >= data1.size() || type != 0)
			{
				prevx = x;
				prevy = y;
				x *= 65535;
				y *= 65535;

				int xx = (int)x;
				int yy = (int)y;

				UINT flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
#if 0
				SetCursorPos(prevx * WIDTH, prevy * HEIGHT);
#else
#ifdef CLICK_MOUSE
				if (type == 1)
					flags |= MOUSEEVENTF_LEFTDOWN;
				else if (type == 2)
					flags |= MOUSEEVENTF_LEFTUP;
#endif
				mouse_event(flags, xx, yy, 0, 0); // TODO: Should this be called so often?
#endif
			}
		}

		if (points.size() > 0)
		{
			RenderLines(points);

			pSwapChain->Present(0, 0);
		}
	}
}

void ReceiveThread()
{
	while (running)
	{
		if (paused)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}

		if (clientSocket == NULL)
		{
			std::cout << "Waiting for incoming connections..." << std::endl;

			clientSocket = accept(listenSocket, NULL, NULL);
			if (clientSocket == INVALID_SOCKET)
			{
				std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
				clientSocket = NULL;
				continue;
			}

			std::cout << "Client connected!" << std::endl;
		}

		constexpr int S = (5 * 4) * BATCH; //16
		char recvBuffer[S];
		int recvResult;
		recvResult = recv(clientSocket, recvBuffer, S, 0);
		if (recvResult > 0)
		{
			mutex.lock();
			int c = data0.size();
			data0.resize(c + recvResult);
			memcpy(data0.data() + c, recvBuffer, recvResult);
			mutex.unlock();
		}
		else if (recvResult == 0)
		{
			//closesocket(clientSocket);
			clientSocket = NULL;
			std::cout << "Connection closed by peer." << std::endl;
			continue;
		}
		else
		{
			std::cerr << "Receive failed with error: " << WSAGetLastError() << std::endl;
			continue;
		}
	}
}

HWND hWnd = NULL;
HWND previousFocusedWindow = NULL;

static bool qPressed = false;
static bool wPressed = false;
static bool ePressed = false;

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

void UpdateIcon()
{
	HICON hNewIcon = LoadIcon(hInst, MAKEINTRESOURCE(paused ? IDI_SCREENDRAWINACTIVE : IDI_SCREENDRAWACTIVE));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hNewIcon);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hNewIcon);
}

void UpdateThread()
{
	while (running)
	{
		HWND fw = GetForegroundWindow();
		if (fw == hWnd)
		{
			if (previousFocusedWindow)
				SetForegroundWindow(previousFocusedWindow);
		}
		else
			previousFocusedWindow = fw;

		if (!paused && qPressed && wPressed && ePressed)
		{
			running = false;
			clear = false;

			float ClearColor[4] = { 0.0f, 0.1f, 0.1f, 1.0f };
			pDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
			pSwapChain->Present(0, 0);

			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			float ClearColor2[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor2);
			pSwapChain->Present(0, 0);

			PostQuitMessage(0);
		}

		if (clear)
		{
			clear = false;
			float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			pDeviceContext->ClearRenderTargetView(pRenderTargetView, ClearColor);
			pSwapChain->Present(0, 0);
		}

		try
		{
			if (running && !paused)
				Update();
		}
		catch (...)
		{
			std::cerr << "Caught an exception!" << std::endl;
		}

		if (paused)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HHOOK keyboardHook;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (paused || nCode < 0)
		return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT* pKeyInfo = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		if (pKeyInfo->vkCode == VK_CONTROL || pKeyInfo->vkCode == VK_LCONTROL || pKeyInfo->vkCode == VK_RCONTROL)
		{
			if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
				ctrling = true;
			else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
				ctrling = false;
		}
		if (pKeyInfo->vkCode == 'Q' && wParam == WM_KEYDOWN)
		{
			qPressed = true;
			clear = true;
			return 1;
		}
		if (pKeyInfo->vkCode == 'Q' && wParam == WM_KEYUP)
		{
			qPressed = false;
			return 1;
		}
		if (pKeyInfo->vkCode == 'W' && wParam == WM_KEYDOWN)
		{
			wPressed = keyErase = true;
			return 1;
		}
		if (pKeyInfo->vkCode == 'W' && wParam == WM_KEYUP)
		{
			wPressed = keyErase = false;
			return 1;
		}
		if (pKeyInfo->vkCode == 'E' && wParam == WM_KEYDOWN)
		{
			ePressed = true;
			drawColorID++;
			drawColorID = drawColorID % DRAW_COLOR_COUNT;
			return 1;
		}
		if (pKeyInfo->vkCode == 'E' && wParam == WM_KEYUP)
		{
			ePressed = false;
			return 1;
		}
	}
	return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetProcessDPIAware(); //SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SCREENDRAWDESKTOP, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}


	std::ofstream file("ScreenDraw.log");
	if (!file.is_open())
		return 1;
	std::cerr.rdbuf(file.rdbuf());
	std::cout.rdbuf(file.rdbuf());
	std::clog.rdbuf(file.rdbuf());

	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed with error: " << result << std::endl;
		return 1;
	}

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(8987);

	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCREENDRAWDESKTOP));

	MSG msg;

	pSwapChain->Present(0, 0);


	std::thread receiveThread(ReceiveThread);
	if (!SetThreadPriority(receiveThread.native_handle(), THREAD_PRIORITY_HIGHEST))
		std::cerr << "Failed to change thread priority.\n";


	ID3DBlob* pErrorBlob = nullptr;
	ID3DBlob* pVertexShaderBlob = nullptr;
	HRESULT hr = D3DCompile(vertexShaderCode, strlen(vertexShaderCode), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &pVertexShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			char* errorMsg = static_cast<char*>(pErrorBlob->GetBufferPointer());
			std::cerr << errorMsg << hr << std::endl;
			pErrorBlob->Release();
		}
		return 1;
	}
	hr = pDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &pVertexShader);
	if (FAILED(hr))
	{
		std::cerr << "No VS: " << hr << std::endl;
		return 1;
	}
	ID3DBlob* pPixelShaderBlob = nullptr;
	hr = D3DCompile(pixelShaderCode, strlen(pixelShaderCode), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &pPixelShaderBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			char* errorMsg = static_cast<char*>(pErrorBlob->GetBufferPointer());
			std::cerr << errorMsg << hr << std::endl;
			pErrorBlob->Release();
		}
		return 1;
	}
	hr = pDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &pPixelShader);
	if (FAILED(hr))
	{
		std::cerr << "No PS: " << hr << std::endl;
		return 1;
	}

	D3D11_INPUT_ELEMENT_DESC layout[2] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(float) * 4, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	pDevice->CreateInputLayout(layout, 2, pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), &pInputLayout);

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.ScissorEnable = false;
	pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE; // Enable depth testing
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Enable writing to depth buffer
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS; // Set depth comparison function
	pDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);

	// Set one time here
	pDeviceContext->RSSetState(pRasterizerState);
	pDeviceContext->OMSetDepthStencilState(pDepthStencilState, 1); // 1 is the stencil reference value
	pDeviceContext->IASetInputLayout(pInputLayout);
	pDeviceContext->VSSetShader(pVertexShader, nullptr, 0);
	pDeviceContext->PSSetShader(pPixelShader, nullptr, 0);
	pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	std::thread updateThread(UpdateThread);
	if (!SetThreadPriority(updateThread.native_handle(), THREAD_PRIORITY_HIGHEST))
		std::cerr << "Failed to change update thread priority.\n";

	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);

	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0) && running)
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	running = false;

	std::cerr << "Ending?" << std::endl;

	if (pVertexBuffer)
		pVertexBuffer->Release();
	pVertexBuffer = NULL;

	if (clientSocket != NULL)
		closesocket(clientSocket);
	closesocket(listenSocket);
	WSACleanup();

	return !running ? 1 : (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCREENDRAWACTIVE));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SCREENDRAWDESKTOP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SCREENDRAWACTIVE));

	return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	WIDTH = GetSystemMetrics(SM_CXSCREEN);
	HEIGHT = GetSystemMetrics(SM_CYSCREEN);

	hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP,
		0, 0, WIDTH, HEIGHT, nullptr, nullptr, hInstance, nullptr); //CW_USEDEFAULT

	if (!hWnd)
	{
		return FALSE;
	}

	SetMenu(hWnd, NULL);

	InitDirectX(hWnd);

	previousFocusedWindow = GetForegroundWindow();

	ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT); // | WS_EX_NOACTIVATE

	//ShowWindow(hWnd, SW_SHOWNA);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_ACTIVATE:
			if (LOWORD(wParam) == WA_ACTIVE)
			{
				paused = !paused;
				UpdateIcon();
				if (paused)
					clear = true;
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
