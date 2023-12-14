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

int WIDTH = 3440;
int HEIGHT = 1440;
const float drawSize0 = 4.0f;
const float drawSize1 = 4.0f;
const float eraseSize = 200.0f;
//#define CLICK_MOUSE
float downThreshold = 0.45f;
float upThreshold = 0.25f;

int drawColorID = 0;
#define DRAW_COLOR_COUNT 3
DirectX::XMFLOAT4 drawColors[DRAW_COLOR_COUNT]
{
	{1.0f,0.0f,0.0f,1.0f},
	{0.0f,1.0f,0.0f,1.0f},
	{0.0f,0.0f,1.0f,1.0f},
};

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

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

void InitDirectX(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferDesc.Width = WIDTH;
	swapChainDesc.BufferDesc.Height = HEIGHT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 1;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
		D3D11_SDK_VERSION, &swapChainDesc, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pd3dDeviceContext);

	// Get the back buffer
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();

	// Set render target
	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

	// Setup viewport
	D3D11_VIEWPORT vp;
	vp.Width = WIDTH;
	vp.Height = HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pd3dDeviceContext->RSSetViewports(1, &vp);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
}

// Shader code for the vertex and pixel shaders
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
void RenderLines(std::vector<DirectX::XMFLOAT4>& positions)
{
	if (positions.size() == 0)
		return;
	ID3D11DeviceContext* pContext = g_pd3dDeviceContext;
	ID3D11Device* pDevice = g_pd3dDevice;

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
			if (erasing)
			{
				s = eraseSize;
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

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(Vertex) * static_cast<UINT>(vertices.size());
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices.data();

	ID3D11Buffer* pVertexBuffer = nullptr;
	HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &initData, &pVertexBuffer);
	if (FAILED(hr))
	{
		std::cerr << hr << std::endl;
	}
	if (!pVertexBuffer)
	{
		std::cerr << "!pVertexBuffer" << std::endl;
		return;
	}
	//std::cout << "Yes" << std::endl;

	g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr); //?

	g_pd3dDeviceContext->RSSetState(pRasterizerState);
	g_pd3dDeviceContext->OMSetDepthStencilState(pDepthStencilState, 1); // 1 is the stencil reference value

	pContext->IASetInputLayout(pInputLayout);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	pContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &stride, &offset);

	pContext->VSSetShader(pVertexShader, nullptr, 0);
	pContext->PSSetShader(pPixelShader, nullptr, 0);

	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
	pContext->Draw(static_cast<UINT>(vertices.size()), 0);

	pVertexBuffer->Release();
}

std::mutex mutex;
std::vector<char> data;

bool clear = false;
void Update()
{
	if (clear)
	{
		clear = false;
		float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		g_pd3dDeviceContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
		g_pSwapChain->Present(1, 0);
	}
	//std::cerr << "Update" << std::endl;

	if (data.size() > 0)
	{
		//std::cout << "Received: " << std::string(recvBuffer, recvResult) << std::endl;

		std::vector<DirectX::XMFLOAT4> points;

		mutex.lock();
		int loc = 0;
		while (loc < ((int)data.size() - 3) && (*((int*)(data.data() + loc)) != 478934687))
			loc++;
		while (loc < (int)data.size() - 5 * 4 + 1)
		{
			char* b = data.data() + loc;
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
					if (pressure < upThreshold)
					{
						down = false;
						type = UP;
					}
				}
				else
				{
					if (pressure > downThreshold)
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

			const float minDist = 3.0f;
			if (dist > minDist || loc >= data.size() || type != 0) // 
			{
				prevx = x;
				prevy = y;
				x *= 65535;
				y *= 65535;

				int xx = (int)x;
				int yy = (int)y;

				UINT flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
#ifdef CLICK_MOUSE
				if (type == 1)
					flags |= MOUSEEVENTF_LEFTDOWN;
				else if (type == 2)
					flags |= MOUSEEVENTF_LEFTUP;
#endif
				mouse_event(flags, xx, yy, 0, 0);

				//System.Threading.Thread.Sleep(TimeSpan.FromTicks(1000));
			}
		}
		data.clear();
		mutex.unlock();

		if (points.size() > 0)
		{
			RenderLines(points);
		}

		g_pSwapChain->Present(1, 0);
	}
}

bool running = true;
void Receive()
{
	while (running)
	{
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

		char recvBuffer[5 * 4 * 16];
		int recvResult;
		recvResult = recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		if (recvResult > 0)
		{
			mutex.lock();
			int c = data.size();
			data.resize(c + recvResult);
			memcpy(data.data() + c, recvBuffer, recvResult);
			mutex.unlock();
		}
		else if (recvResult == 0)
		{
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

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

HHOOK keyboardHook;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode < 0)
		return CallNextHookEx(keyboardHook, nCode, wParam, lParam);
	if (nCode == HC_ACTION)
	{
		// Check for key events here
		KBDLLHOOKSTRUCT* pKeyInfo = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		if (pKeyInfo->vkCode == 'Q' && wParam == WM_KEYDOWN)
		{
			clear = true;
		}
		if (pKeyInfo->vkCode == 'E' && wParam == WM_KEYDOWN)
		{
			drawColorID++;
			drawColorID = drawColorID % DRAW_COLOR_COUNT;
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

	// TODO: Place code here.

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

	if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SCREENDRAWDESKTOP));

	MSG msg;

	g_pSwapChain->Present(1, 0);


	std::thread receiveThread(Receive);


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
	hr = g_pd3dDevice->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &pVertexShader);
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
	hr = g_pd3dDevice->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &pPixelShader);
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
	g_pd3dDevice->CreateInputLayout(layout, 2, pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), &pInputLayout);

	D3D11_RASTERIZER_DESC rasterizerDesc = {};
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.ScissorEnable = false;
	g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState);

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	depthStencilDesc.DepthEnable = FALSE; // Enable depth testing
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // Enable writing to depth buffer
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS; // Set depth comparison function
	g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &pDepthStencilState);


	// Main message loop:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		try
		{
			Update();
		}
		catch (...)
		{
			std::cerr << "Caught an exception!" << std::endl;
		}

		//if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		//{
			//TranslateMessage(&msg);
			//DispatchMessage(&msg);
		//}
	}

	running = false;

	std::cerr << "Ending?" << std::endl;

	if (clientSocket != NULL)
		closesocket(clientSocket);
	closesocket(listenSocket);
	WSACleanup();

	return (int)msg.wParam;
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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SCREENDRAWDESKTOP));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SCREENDRAWDESKTOP);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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

	int WIDTH = GetSystemMetrics(SM_CXSCREEN);
	int HEIGHT = GetSystemMetrics(SM_CYSCREEN);

	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP | WS_VISIBLE,
		0, 0, WIDTH, HEIGHT, nullptr, nullptr, hInstance, nullptr); //CW_USEDEFAULT

	if (!hWnd)
	{
		return FALSE;
	}

	InitDirectX(hWnd);

	//SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);

	ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	//SetWindowLongPtr(hWnd, GWL_EXSTYLE, (GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_BORDER & ~WS_SYSMENU);
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	//SetWindowPos(hWnd, HWND_TOP, 0, 0, screenWidth, screenHeight, SWP_SHOWWINDOW);

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
		//case WM_NCHITTEST:
		//    // Return HTTRANSPARENT for the regions you want to be click-through
		//    return HTTRANSPARENT;

		//case WM_KEYDOWN:
		//	if (wParam == VK_LEFT)
		//	{
		//	}
		//	break;

		//case WM_COMMAND:
		//{
		//	int wmId = LOWORD(wParam);
		//	// Parse the menu selections:
		//	switch (wmId)
		//	{
		//		case IDM_ABOUT:
		//			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		//			break;
		//		case IDM_EXIT:
		//			DestroyWindow(hWnd);
		//			break;
		//		default:
		//			return DefWindowProc(hWnd, message, wParam, lParam);
		//	}
		//}
		//break;
		// 
		//case WM_PAINT:
		//{
		//	PAINTSTRUCT ps;
		//	HDC hdc = BeginPaint(hWnd, &ps);
		//	// TODO: Add any drawing code that uses hdc here...
		//	EndPaint(hWnd, &ps);
		//}
		//break;

		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}
