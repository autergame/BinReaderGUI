//author https://github.com/autergame
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "opengl32")
#include "libs/glad/glad.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_win32.h"
#include "libs/imgui/imgui_impl_opengl3.h"
#include "libs/imgui/imgui_internal.h"
#include "BinReaderLib.h"
#include <windows.h>


typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext,
	const int* attribList);
wglCreateContextAttribsARB_type* wglCreateContextAttribsARB;

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001

typedef bool WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int* piAttribIList,
	const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
wglChoosePixelFormatARB_type* wglChoosePixelFormatARB;

#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_SAMPLE_BUFFERS_ARB                    0x2041
#define WGL_SAMPLES_ARB                           0x2042

#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

void* GetAnyGLFuncAddress(const char* name)
{
	void* p = (void*)wglGetProcAddress(name);
	if (p == 0 || (p == (void*)0x1) || (p == (void*)0x2) || (p == (void*)0x3) || (p == (void*)-1))
	{
		HMODULE module = LoadLibraryA("opengl32.dll");
		p = (void*)GetProcAddress(module, name);
	}
	return p;
}

bool touch[256];
int width, height;
bool active = true;
LARGE_INTEGER Frequencye, Starte;

double GetTimeSinceStart()
{
	LARGE_INTEGER t_End;
	QueryPerformanceCounter(&t_End);
	return (float)(t_End.QuadPart - Starte.QuadPart) / Frequencye.QuadPart;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return true;

	switch (uMsg)
	{
		case WM_SIZE:
			width = LOWORD(lParam);
			height = HIWORD(lParam);
			glViewport(0, 0, width, height);
			PostMessage(hWnd, WM_PAINT, 0, 0);
			break;

		case WM_CLOSE:
			active = FALSE;
			break;

		case WM_KEYDOWN:
			touch[wParam] = TRUE;
			break;

		case WM_KEYUP:
			touch[wParam] = FALSE;
			break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int main()
{
	QueryPerformanceFrequency(&Frequencye);
	QueryPerformanceCounter(&Starte);

	WNDCLASSA window_class;
	window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_class.lpfnWndProc = (WNDPROC)WindowProc;
	window_class.cbClsExtra = 0;
	window_class.cbWndExtra = 0;
	window_class.hInstance = GetModuleHandle(0);
	window_class.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	window_class.hCursor = LoadCursor(0, IDC_ARROW);
	window_class.hbrBackground = 0;
	window_class.hbrBackground = NULL;
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = "binreadergui";

	if (!RegisterClassA(&window_class)) {
		printf("Failed to RegisterClassA: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	RECT rectScreen;
	width = 1024, height = 600;
	HWND hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);
	int PosX = ((rectScreen.right - rectScreen.left) / 2 - width / 2);
	int PosY = ((rectScreen.bottom - rectScreen.top) / 2 - height / 2);

	HWND window = CreateWindowExA(
		0,
		window_class.lpszClassName,
		"OpenGL Window",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		PosX,
		PosY,
		width,
		height,
		0,
		0,
		window_class.hInstance,
		0);

	if (!window) {
		printf("Failed to CreateWindowExA: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	WNDCLASSA window_classe;
	window_classe.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	window_classe.lpfnWndProc = DefWindowProcA;
	window_classe.cbClsExtra = 0;
	window_classe.cbWndExtra = 0;
	window_classe.hInstance = GetModuleHandle(0);
	window_classe.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	window_classe.hCursor = LoadCursor(0, IDC_ARROW);
	window_classe.hbrBackground = 0;
	window_classe.hbrBackground = NULL;
	window_classe.lpszMenuName = NULL;
	window_classe.lpszClassName = "Dummy_binreadergui";

	if (!RegisterClassA(&window_classe)) {
		printf("Failed to RegisterClassA: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	HWND dummy_window = CreateWindowExA(
		0,
		window_classe.lpszClassName,
		"Dummy OpenGL Window",
		0,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		window_classe.hInstance,
		0);

	if (!dummy_window) {
		printf("Failed to CreateWindowExA dummy: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 8;

	HDC dummy_dc = GetDC(dummy_window);
	int pixel_formate = ChoosePixelFormat(dummy_dc, &pfd);
	if (!pixel_formate) {
		printf("Failed to ChoosePixelFormat: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}
	if (!SetPixelFormat(dummy_dc, pixel_formate, &pfd)) {
		printf("Failed to SetPixelFormat: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	HGLRC dummy_context = wglCreateContext(dummy_dc);
	if (!dummy_context) {
		printf("Failed to wglCreateContext dummy: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		printf("Failed to wglMakeCurrent dummy: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_type*)wglGetProcAddress(
		"wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_type*)wglGetProcAddress(
		"wglChoosePixelFormatARB");

	int pixel_format_attribs[] = {
	  WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
	  WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
	  WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
	  WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
	  WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
	  WGL_COLOR_BITS_ARB,         32,
	  WGL_DEPTH_BITS_ARB,         8,
	  WGL_STENCIL_BITS_ARB,       0,
	  WGL_SAMPLE_BUFFERS_ARB,     GL_FALSE,
	  WGL_SAMPLES_ARB,			  0,
	  0
	};

	int pixel_format;
	UINT num_formats;
	HDC real_dc = GetDC(window);
	wglChoosePixelFormatARB(real_dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
	if (!num_formats) {
		printf("Failed to wglChoosePixelFormatARB: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	PIXELFORMATDESCRIPTOR pfde;
	DescribePixelFormat(real_dc, pixel_format, sizeof(pfde), &pfde);
	if (!SetPixelFormat(real_dc, pixel_format, &pfde)) {
		printf("Failed to SetPixelFormat: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	int gl33_attribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
		WGL_CONTEXT_MINOR_VERSION_ARB, 3,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0,
	};

	HGLRC gl33_context = wglCreateContextAttribsARB(real_dc, 0, gl33_attribs);
	if (!gl33_context) {
		printf("Failed to wglCreateContextAttribsARB: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	if (!wglMakeCurrent(real_dc, gl33_context)) {
		printf("Failed to wglMakeCurrent: %d.\n", GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	if (!gladLoadGLLoader((GLADloadproc)GetAnyGLFuncAddress)) {
		printf("Failed to gladLoadGLLoader.\n");
		scanf("press enter to exit.");
		return 1;
	}

	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = NULL;
	ImGui_ImplWin32_Init(window);
	ImGui_ImplOpenGL3_Init("#version 130");
	ImVec4* colors = GImGui->Style.Colors;
	colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.f);
	colors[ImGuiCol_Header] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4f, 0.4f, 0.4f, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.f);
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, GImGui->Style.FramePadding.x * 3.f - 2.5f);
	int FULL_SCREEN_FLAGS = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

	printf("loading hashes.\n");
	HashTable* hasht = createHashTable(1000000);
	addhash(hasht, "hashes.bintypes.txt");
	addhash(hasht, "hashes.binfields.txt");
	addhash(hasht, "hashes.binhashes.txt");
	addhash(hasht, "hashes.binentries.txt");
	printf("finised loading hashes.\n");

	MSG msg = { 0 };
	uintptr_t treebefore = 0;
	char szFile[260] = { 0 };
	PacketBin* packet = NULL;
	ShowWindow(window, TRUE);
	UpdateWindow(window);
	HDC gldc = GetDC(window);
	float Deltatime = 0, Lastedtime = 0;
	char* tmp = (char*)calloc(1, 64);
	myassert(tmp == NULL);
	while (active)
	{
		if (PeekMessageA(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			continue;
		}

		if (touch[VK_ESCAPE])
			active = FALSE;

		Deltatime = float(GetTimeSinceStart() - Lastedtime);
		Lastedtime = (float)GetTimeSinceStart();

		sprintf(tmp, "BinReaderGUI - FPS: %1.0f", 1 / Deltatime);
		SetWindowTextA(window, tmp);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
		ImGui::Begin("Main", 0, FULL_SCREEN_FLAGS);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem("Open Bin"))
			{
				OPENFILENAMEA ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = window;
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = sizeof(szFile);
				ofn.lpstrFilter = "*.bin\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = NULL;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = NULL;
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
				if (GetOpenFileNameA(&ofn) == TRUE)
				{
					treebefore = 0;
					if (packet != NULL)
						cleanbin(packet->entriesMap);
					packet = decode(szFile, hasht);
					myassert(packet == NULL);
					ImGui::GetStateStorage()->Clear();
					getstructidbin(packet->entriesMap, &treebefore);
				}
			}
			if (szFile[0] != 0 && packet != NULL)
			{
				if (ImGui::MenuItem("Save Bin"))
				{
					OPENFILENAMEA ofn;
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = window;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "*.bin\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
					if (GetSaveFileNameA(&ofn) == TRUE)
					{
						encode(szFile, packet);
					}
				}
			}
			ImGui::EndMenuBar();
		}
		if (szFile[0] != 0 && packet != NULL)
		{
			ImGui::AlignTextToFramePadding();
			bool treeope = ImGui::TreeNodeEx((void*)0, ImGuiTreeNodeFlags_SpanFullWidth, "");
			ImGui::SameLine(); ImGui::Text(szFile);
			if (treeope)
			{
				getvaluefromtype(packet->entriesMap, hasht);
				ImGui::TreePop();
			}
		}
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SwapBuffers(gldc);
	}

	if (packet != NULL)
		cleanbin(packet->entriesMap);

	ImGui_ImplOpenGL3_Shutdown();
	wglDeleteContext(gl33_context);
	ImGui::DestroyContext();
	ImGui_ImplWin32_Shutdown();

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(gl33_context);
	ReleaseDC(window, gldc);
	DestroyWindow(window);
	UnregisterClassA("binreadergui", window_class.hInstance);

	return (int)msg.wParam;
}