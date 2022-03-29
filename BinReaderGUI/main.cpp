//author https://github.com/autergame

#pragma comment(lib, "mimalloc-static")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glfw3")

#pragma warning(push, 0)

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

#include <new>
#include <mimalloc.h>
#include <Windows.h>

#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>

#pragma warning(pop)

#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>

#include "MemoryWrapper.h"
#include "Myassert.h"

#include "Hashtable.h"
#include "TernaryTree.h"
#include "BinReader.h"
#include "BinReaderLib.h"

#include <algorithm>


void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
	scanf_s("Press enter to exit.");
	_Exit(EXIT_FAILURE);
}

void strip_filepath(char* fname)
{
	char* end = fname + strlen(fname);
	while (end > fname && *end != '\\')
		--end;
	if (end > fname)
		*end = '\0';
}

int main()
{
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC, "C");

	#ifdef TRACY_ENABLE_ZONES
		ZoneNamedN(mz, "main", true);
	#endif

	printf("Loading hashes\n");

	HashTable hashT;
	hashT.LoadFromFile("hashes.bintypes.txt");
	hashT.LoadFromFile("hashes.binfields.txt");
	hashT.LoadFromFile("hashes.binhashes.txt");
	hashT.LoadFromFile("hashes.binentries.txt");
#ifdef NDEBUG
	hashT.LoadFromFile("hashes.game.txt");
	hashT.LoadFromFile("hashes.lcu.txt");
#endif

	printf("Loaded total of hashes: %zd\n", hashT.table.size());

	hashT.Insert(0xf9100aa9, "patch");
	hashT.Insert(0x84874d36, "path");
	hashT.Insert(0x425ed3ca, "value");

	printf("Finised loading hashes\n\n");

	printf("Loading ternary\n");

	TernaryTree ternaryT;
	ternaryT.LoadFromHashTable(hashT);

	printf("Finised loading ternary\n\n");

	char openPath[MAX_PATH] = { '\0' };
	char savePath[MAX_PATH] = { '\0' };
	char openFile[MAX_PATH] = { '\0' };
	char saveFile[MAX_PATH] = { '\0' };

	char currentPath[MAX_PATH] = { '\0' };
	GetCurrentDirectoryA(MAX_PATH, currentPath);

	LSTATUS regStatus = 0;
	HKEY regkeyresult = nullptr;
	regStatus = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", 0, KEY_ALL_ACCESS, &regkeyresult);
	if (regStatus == ERROR_PATH_NOT_FOUND)
	{
		regStatus = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", 0, nullptr,
			REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, nullptr, &regkeyresult, nullptr);
		if (regStatus != ERROR_SUCCESS)
			printf("Creating key failed: %d %d\n", regStatus, GetLastError());
	}
	else if (regStatus != ERROR_SUCCESS)
		printf("Setting key value failed: %d %d\n", regStatus, GetLastError());

	DWORD pathSize = MAX_PATH;
	regStatus = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "openpath",
		RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, openPath, &pathSize);
	if (regStatus == ERROR_FILE_NOT_FOUND)
	{
		regStatus = RegSetValueExA(regkeyresult, "openpath", 0, REG_EXPAND_SZ, (LPCBYTE)currentPath, MAX_PATH);
		if (regStatus != ERROR_SUCCESS)
			printf("Setting key value failed: %d %d\n", regStatus, GetLastError());
	}
	else if (regStatus != ERROR_SUCCESS)
		printf("Getting key value failed: %d %d\n", regStatus, GetLastError());

	pathSize = MAX_PATH;
	regStatus = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "savepath",
		RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, savePath, &pathSize);
	if (regStatus == ERROR_FILE_NOT_FOUND)
	{
		regStatus = RegSetValueExA(regkeyresult, "savepath", 0, REG_EXPAND_SZ, (LPCBYTE)currentPath, MAX_PATH);
		if (regStatus != ERROR_SUCCESS)
			printf("Setting key value failed: %d %d\n", regStatus, GetLastError());
	}
	else if (regStatus != ERROR_SUCCESS)
		printf("Getting key value failed: %d %d\n", regStatus, GetLastError());

	pathSize = 4;
	DWORD vSync = 0;
	regStatus = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "vsync",
		RRF_RT_REG_DWORD, nullptr, &vSync, &pathSize);
	if (regStatus == ERROR_FILE_NOT_FOUND)
	{
		vSync = 0;
		regStatus = RegSetValueExA(regkeyresult, "vsync", 0, REG_DWORD, (LPCBYTE)&vSync, 4);
		if (regStatus != ERROR_SUCCESS)
			printf("Setting key value failed: %d %d\n", regStatus, GetLastError());
	}
	else if (regStatus != ERROR_SUCCESS)
		printf("Getting key value failed: %d %d\n", regStatus, GetLastError());

	RECT rectScreen;
	HWND hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);
	int windowWidth = (int)((float)rectScreen.right * 0.75f);
	int windowHeight = (int)((float)rectScreen.bottom * 0.75f);
	int windowPosX = ((rectScreen.right - rectScreen.left) / 2 - windowWidth / 2);
	int windowPosY = ((rectScreen.bottom - rectScreen.top) / 2 - windowHeight / 2);

	glfwSetErrorCallback(glfw_error_callback);
	myassert(glfwInit() == GLFW_FALSE)

	glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *glfwWindow = glfwCreateWindow(windowWidth, windowHeight, "BinReaderGUI", nullptr, nullptr);
	myassert(glfwWindow == nullptr)

	glfwMakeContextCurrent(glfwWindow);
	glfwSwapInterval(vSync);

	glfwSetWindowPos(glfwWindow, windowPosX, windowPosY);
	myassert(gladLoadGL() == 0)

	static ImGuiMemAllocFunc allocFunc = MallocbWrapper;
	static ImGuiMemFreeFunc freeFunc = FreebWrapper;
	ImGui::SetAllocatorFunctions(allocFunc, freeFunc);
	ImGui::CreateContext();
	GImGui->IO.IniFilename = nullptr;
	ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330");
	ImVec4* colors = GImGui->Style.Colors;
	colors[ImGuiCol_FrameBg] = ImColor(76, 76, 76, 255);
	colors[ImGuiCol_Header] = ImColor(51, 51, 51, 255);
	colors[ImGuiCol_Button] = ImColor(51, 51, 51, 255);
	colors[ImGuiCol_Border] = ImColor(76, 76, 76, 255);
	colors[ImGuiCol_FrameBgHovered] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_HeaderHovered] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_ButtonHovered] = ImColor(76, 76, 76, 255);
	colors[ImGuiCol_FrameBgActive] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_HeaderActive] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_ButtonActive] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_TextSelectedBg] = ImColor(102, 102, 102, 255);
	colors[ImGuiCol_MenuBarBg] = ImColor(51, 51, 51, 255);
	colors[ImGuiCol_CheckMark] = ImColor(190, 190, 190, 255);
	GImGui->Style.WindowRounding = 0.f;
	GImGui->Style.FrameBorderSize = 1.f;
	GImGui->Style.WindowBorderSize = 1.f;
	GImGui->Style.IndentSpacing = GImGui->Style.FramePadding.x * 3.f - 2.f;
	GImGui->IO.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13);
	int FULL_SCREEN_FLAGS = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

	char packetStrVersion[32] = { '\0' };
	char windowTitle[64] = { '\0' };
	mi_string stringBuf(2, '\0');
	
	bool rainbow = false;
	bool openTree = false;
	bool openChoose = false;

	int treeId = 0;
	PacketBin packet;

#define TESTBIN
#if defined TESTBIN && defined _DEBUG
	const char* teststr = "C:\\Users\\autergame\\Documents\\Visual Studio 2019\\Projects\\BinReader\\BinReader\\DebugWR\\test.bin";
	if (packet.DecodeBin((char*)teststr, hashT, ternaryT))
	{
		treeId = 2;
		myassert(memcpy(openFile, teststr, strlen(teststr)) != openFile)
		myassert(sprintf_s(packetStrVersion, 32, "%" PRIu32, packet.m_Version) <= 0)
		if (packet.m_Version >= 2)
		{
			for (uint32_t i = 0; i < packet.m_linkedList.size(); i++)
			{
				packet.m_linkedList[i].first = treeId;
				treeId += 2;
			}
		}
		ContructIdForBinField(packet.m_entriesBin, treeId);
		ContructIdForBinField(packet.m_patchesBin, treeId);
	}
#endif 

	glClearColor(0.f, 0.f, 0.f, 1.f);
	HWND glfwWindowNative = glfwGetWin32Window(glfwWindow);
	while (!glfwWindowShouldClose(glfwWindow))
	{
		#ifdef TRACY_ENABLE_ZONES
			FrameMark;
		#endif 	

		glfwPollEvents();
		if (glfwGetKey(glfwWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(glfwWindow, true);

		glfwGetFramebufferSize(glfwWindow, &windowWidth, &windowHeight);
		glViewport(0, 0, windowWidth, windowHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		myassert(sprintf_s(windowTitle, 64,
			"BinReaderGUI - Fps: %1.0f / Ms: %.3f", GImGui->IO.Framerate, 1000.0f / GImGui->IO.Framerate) <= 0)
		glfwSetWindowTitle(glfwWindow, windowTitle);

		if (GImGui->IO.Framerate < 45.f)
			GImGui->Style.FrameRounding = 0.f;
		else
			GImGui->Style.FrameRounding = 4.f;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));
		if (ImGui::Begin("Main", 0, FULL_SCREEN_FLAGS))
		{
			ImGuiWindow& imguiWindow = *ImGui::GetCurrentWindow();
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::MenuItem("Open Bin"))
				{				
					OPENFILENAMEA ofn;
					myassert(memset(&ofn, 0, sizeof(ofn)) != &ofn)
					myassert(memset(openFile, 0, MAX_PATH) != openFile)
					myassert(memset(openPath, 0, MAX_PATH) != openPath)

					pathSize = MAX_PATH;
					regStatus = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "openpath",
						RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, openPath, &pathSize);
					if (regStatus != ERROR_SUCCESS)
						printf("Getting key value failed: %d %d\n", regStatus, GetLastError());

					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = glfwWindowNative;
					ofn.lpstrFile = openFile;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrFilter = "*.bin";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = nullptr;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = openPath;
					ofn.lpstrTitle = "Open Bin File";
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

					if (GetOpenFileNameA(&ofn) == TRUE)
					{
						if (openFile[0] != '\0')
						{
							size_t openFileLen = strlen(openFile);
							myassert(memset(openPath, 0, MAX_PATH) != openPath)
							myassert(memcpy(openPath, openFile, openFileLen) != openPath)
							strip_filepath(openPath);

							regStatus = RegSetValueExA(regkeyresult, "openpath", 0, REG_EXPAND_SZ,
								(LPCBYTE)openPath, (DWORD)openFileLen);
							if (regStatus != ERROR_SUCCESS)
								printf("Setting key value failed: %d %d\n", regStatus, GetLastError());

							PacketBin packetNew; 
							if (packetNew.DecodeBin(openFile, hashT, ternaryT))
							{
								if (packet.m_entriesBin)
									ClearBinField(packet.m_entriesBin);
								if (packet.m_patchesBin)
									ClearBinField(packet.m_patchesBin);

								treeId = 2;
								packet = packetNew;
								ImGui::GetStateStorage()->Clear();
								myassert(sprintf_s(packetStrVersion, 32, "%" PRIu32, packet.m_Version) <= 0)
								if (packet.m_Version >= 2)
								{
									for (uint32_t i = 0; i < packet.m_linkedList.size(); i++)
									{
										packet.m_linkedList[i].first = treeId;
										treeId += 2;
									}
								}

								ContructIdForBinField(packet.m_entriesBin, treeId);
								ContructIdForBinField(packet.m_patchesBin, treeId);
							}
						}
					}
				}
				if (packet.m_entriesBin)
				{
					if (ImGui::MenuItem("Save Bin"))
					{
						OPENFILENAMEA ofn;
						myassert(memset(&ofn, 0, sizeof(ofn)) != &ofn)
						myassert(memset(saveFile, 0, MAX_PATH) != saveFile)
						myassert(memset(savePath, 0, MAX_PATH) != savePath)

						pathSize = MAX_PATH;
						regStatus = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "savepath",
							RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, nullptr, savePath, &pathSize);
						if (regStatus != ERROR_SUCCESS)
							printf("Getting key value failed: %d %d\n", regStatus, GetLastError());

						ofn.lStructSize = sizeof(ofn);
						ofn.hwndOwner = glfwWindowNative;
						ofn.lpstrFile = saveFile;
						ofn.nMaxFile = MAX_PATH;
						ofn.lpstrFilter = "*.bin";
						ofn.nFilterIndex = 1;
						ofn.lpstrFileTitle = nullptr;
						ofn.nMaxFileTitle = 0;
						ofn.lpstrDefExt = "bin";
						ofn.lpstrInitialDir = savePath;
						ofn.lpstrTitle = "Save Bin File";
						ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

						if (GetSaveFileNameA(&ofn) == TRUE)
						{
							if (openFile[0] != '\0')
							{
								size_t saveFileLen = strlen(saveFile);
								myassert(memset(savePath, 0, MAX_PATH) != savePath)
								myassert(memcpy(savePath, saveFile, saveFileLen) != savePath)
								strip_filepath(savePath);

								regStatus = RegSetValueExA(regkeyresult, "savepath", 0, REG_EXPAND_SZ,
									(LPCBYTE)savePath, (DWORD)saveFileLen);
								if (regStatus != ERROR_SUCCESS)
									printf("Setting key value failed: %d %d\n", regStatus, GetLastError());

								packet.EncodeBin(saveFile);
							}
						}
					}
					ImGui::SameLine();
					if (ImGui::Checkbox("Open/Close All Tree Nodes", &openChoose))
					{
						if (openChoose)
							openTree = true;
						else
							SetTreeCloseState(packet.m_entriesBin, imguiWindow);
					}
					ImGui::SameLine();
					ImGui::Checkbox("RainBow Mode", &rainbow);
					ImGui::SameLine();
					if (ImGui::Checkbox("Enable Vsync", (bool*)&vSync))
					{
						regStatus = RegSetValueExA(regkeyresult, "vsync", 0, REG_DWORD, (LPCBYTE)&vSync, 4);
						if (regStatus != ERROR_SUCCESS)
							printf("Setting key value failed: %d %d\n", regStatus, GetLastError());
						glfwSwapInterval(vSync);
					}
				}
				ImGui::EndMenuBar();
			}
			if (packet.m_entriesBin)
			{
				ImGui::SetNextItemOpen(true);
				if (ImGui::TreeNodeEx("#filetree", ImGuiTreeNodeFlags_Framed |
					ImGuiTreeNodeFlags_SpanFullWidth, openFile))
				{
					uint32_t treesize = 0;
					GetTotalBinFieldSize(packet.m_entriesBin, treesize);

					ImDrawList* drawList = imguiWindow.DrawList;
					ImDrawListSplitter splitter;
					splitter.Split(drawList, 2);
					splitter.SetCurrentChannel(drawList, 1);
					
					ImGui::Indent();
					{
						ImGui::AlignTextToFramePadding();
						ImGui::Text("Tree Size = %d", treesize);
						ImGui::AlignTextToFramePadding();
						ImGui::Text("Version ="); ImGui::SameLine();
						char* psvString = InputText(packetStrVersion, 1);
						if (psvString)
						{
							myassert(sscanf_s(psvString, "%" PRIu32, &packet.m_Version) <= 0)
							myassert(sprintf_s(packetStrVersion, 32, "%" PRIu32, packet.m_Version) <= 0)
							delete[] psvString;
						}
						ImGui::AlignTextToFramePadding();
						ImGui::Text("Is Patch ="); ImGui::SameLine();
						char* ipString = InputText(packet.m_isPatch ? truestr : falsestr, 2);
						if (ipString)
						{
							for (int i = 0; ipString[i]; i++)
								ipString[i] = tolower(ipString[i]);
							if (strcmp(ipString, truestr) == 0)
								packet.m_isPatch = true;
							else
								packet.m_isPatch = false;
							delete[] ipString;
						}
					}

					if (packet.m_Version >= 2)
					{
						ImVec2 cursor = imguiWindow.DC.CursorPos;
						if (ImGui::TreeNodeEx("#linkedlist", ImGuiTreeNodeFlags_Framed, "LinkedList"))
						{
							ImGui::Indent();
							for (uint32_t i = 0; i < packet.m_linkedList.size(); i++)
							{
								char* str = InputText(packet.m_linkedList[i].second.data(), packet.m_linkedList[i].first);
								if (str)
								{
									packet.m_linkedList[i].second = mi_string(str);
								}
								ImGui::SameLine();
								if (BinFieldDelete(packet.m_linkedList[i].first + 1))
									packet.m_linkedList.erase(packet.m_linkedList.begin() + i);
							}
							if (ImGui::Button("Add new item"))
							{
								treeId += 2;
								packet.m_linkedList.emplace_back(treeId, mi_string(2, '\0'));
							}
							ImGui::Unindent();
							ImGui::TreePop();

							cursor.x -= 3.f;
							ImVec2 cursorMax = ImVec2(imguiWindow.WorkRect.Max.x, imguiWindow.DC.CursorMaxPos.y);
							ImGui::ItemAdd(ImRect(cursor, cursorMax), 0);
							ImColor col;
							if (rainbow)
							{
								if (ImGui::IsItemHovered())
									col = IM_COL32(80, 0, 0, 255);
								else
									col = IM_COL32(64, 0, 0, 255);
							}
							else {
								if (ImGui::IsItemHovered())
									col = IM_COL32(32, 32, 32, 255);
								else
									col = IM_COL32(24, 24, 24, 255);
							}
							splitter.SetCurrentChannel(drawList, 0);
							drawList->AddRectFilled(cursor, cursorMax, col);
							drawList->AddRect(cursor, cursorMax,
								rainbow ? IM_COL32(255, 0, 0, 255) : IM_COL32(128, 128, 128, 255));
							splitter.SetCurrentChannel(drawList, 1);
						}
					}

					ImGui::Separator();

					{
						Map* entryMap = packet.m_entriesBin->data->map;
	#ifdef TRACY_ENABLE_ZONES
						ZoneNamedN(entrymapz, "EntryMap", true);
	#endif
						for (uint32_t i = 0; i < entryMap->items.size(); i++)
						{
							if (openTree)
								ImGui::SetNextItemOpen(true);
							entryMap->items[i].cursorMin = imguiWindow.DC.CursorPos;
							entryMap->items[i].expanded = MyTreeNodeEx(entryMap->items[i].id);
							entryMap->items[i].idim = imguiWindow.IDStack.back();

							if (IsItemVisible(imguiWindow))
							{
								ImVec2 cursor, cursorOld;
	#ifdef _DEBUG
								if (ImGui::IsItemHovered())
									ImGui::SetTooltip("%d", entryMap->items[i].id);
	#endif
								ImGui::SameLine(); InputTextHash(entryMap->items[i].key->data->ui32,
									entryMap->items[i].key->id, hashT, ternaryT);
								ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();

								DrawBinField(entryMap->items[i].value, hashT, ternaryT, treeId,
									imguiWindow, openTree, &entryMap->items[i].expanded, &cursor);

								cursorOld = ImGui::GetCursorPos();
								ImGui::SetCursorPos(cursor);
								if (BinFieldDelete(entryMap->items[i].id + 1))
								{
									ClearBinField(entryMap->items[i].key);
									ClearBinField(entryMap->items[i].value);

									bool open = entryMap->items[i].expanded;
									entryMap->items.erase(entryMap->items.begin() + i);
									if (open)
										ImGui::TreePop();
									continue;
								}
								ImGui::SetCursorPos(cursorOld);
							}
							else {
								ImGui::SameLine();
								DrawBinField(entryMap->items[i].value, hashT, ternaryT,
									treeId, imguiWindow, openTree, &entryMap->items[i].expanded);
							}

							if (entryMap->items[i].expanded)
							{
								ImGui::TreePop();
								if (entryMap->items[i].value->data->pe->name == 0)
								{
									entryMap->items[i].cursorMax.x = 0.f;
									continue;
								}
								entryMap->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x,
									imguiWindow.DC.CursorMaxPos.y);
							}
							else
								entryMap->items[i].isOver = false;
						}

						if (IsItemVisible(imguiWindow))
						{
							if (ImGui::Button("Add new entry item"))
							{
								MapPair mapItem;
								mapItem.key = BinFieldNewClean(entryMap->keyType,
									(BinType)entryMap->current2, (BinType)entryMap->current3);
								mapItem.value = BinFieldNewClean(entryMap->valueType,
									(BinType)entryMap->current4, (BinType)entryMap->current5);

								mapItem.key->parent = packet.m_entriesBin;
								mapItem.value->parent = packet.m_entriesBin;

								entryMap->items.emplace_back(mapItem);
								ContructIdForBinField(packet.m_entriesBin, treeId);
							}
						}
						else {
							MyNewLine(imguiWindow);
						}
					}

					ImGui::Separator();

					if (packet.m_isPatch && packet.m_Version >= 3)
					{
						Map *patchMap = packet.m_patchesBin->data->map;
#ifdef TRACY_ENABLE_ZONES
						ZoneNamedN(patchmapz, "PatchMap", true);
#endif
						for (uint32_t i = 0; i < patchMap->items.size(); i++)
						{
							if (openTree)
								ImGui::SetNextItemOpen(true);
							patchMap->items[i].cursorMin = imguiWindow.DC.CursorPos;
							patchMap->items[i].expanded = MyTreeNodeEx(patchMap->items[i].id);
							patchMap->items[i].idim = imguiWindow.IDStack.back();

							if (IsItemVisible(imguiWindow))
							{
								ImVec2 cursor, cursorOld;
#ifdef _DEBUG
								if (ImGui::IsItemHovered())
									ImGui::SetTooltip("%d", patchMap->items[i].id);
#endif
								ImGui::SameLine(); InputTextHash(patchMap->items[i].key->data->ui32,
									patchMap->items[i].key->id, hashT, ternaryT);
								ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();

								DrawBinField(patchMap->items[i].value, hashT, ternaryT, treeId,
									imguiWindow, openTree, &patchMap->items[i].expanded, &cursor);

								cursorOld = ImGui::GetCursorPos();
								ImGui::SetCursorPos(cursor);
								if (BinFieldDelete(patchMap->items[i].id + 1))
								{
									ClearBinField(patchMap->items[i].key);
									ClearBinField(patchMap->items[i].value);

									bool open = patchMap->items[i].expanded;
									patchMap->items.erase(patchMap->items.begin() + i);
									if (open)
										ImGui::TreePop();
									continue;
								}
								ImGui::SetCursorPos(cursorOld);
							}
							else {
								ImGui::SameLine();
								DrawBinField(patchMap->items[i].value, hashT, ternaryT,
									treeId, imguiWindow, openTree, &patchMap->items[i].expanded);
							}

							if (patchMap->items[i].expanded)
							{
								ImGui::TreePop();
								if (patchMap->items[i].value->data->pe->name == 0)
								{
									patchMap->items[i].cursorMax.x = 0.f;
									continue;
								}
								patchMap->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x,
									imguiWindow.DC.CursorMaxPos.y);
							}
							else
								patchMap->items[i].isOver = false;
						}

						if (IsItemVisible(imguiWindow))
						{
							if (ImGui::Button("Add new patch item"))
							{
								MapPair mapItem;
								mapItem.key = BinFieldNewClean(patchMap->keyType,
									(BinType)patchMap->current2, (BinType)patchMap->current3);
								mapItem.value = BinFieldNewClean(patchMap->valueType,
									(BinType)patchMap->current4, (BinType)patchMap->current5);

								mapItem.key->parent = packet.m_patchesBin;
								mapItem.value->parent = packet.m_patchesBin;

								PointerOrEmbed* pe = mapItem.value->data->pe;
								pe->name = patchFNV;

								char* string = new char[2];
								string[0] = string[1] = '\0';

								BinField* stringBin = new BinField;
								stringBin->parent = mapItem.value;
								stringBin->type = BinType::STRING;
								stringBin->data->string = string;

								EPField firstField;
								firstField.key = pathFNV;
								firstField.value = stringBin;

								pe->items.emplace_back(firstField);

								patchMap->items.emplace_back(mapItem);
								ContructIdForBinField(packet.m_patchesBin, treeId);
							}
						}
						else {
							MyNewLine(imguiWindow);
						}
					}

					ImGui::Unindent();

					ImGui::TreePop();

					openTree = false;
					splitter.SetCurrentChannel(drawList, 0);
					if (rainbow)
					{
						DrawRectRainBow(packet.m_entriesBin, imguiWindow, 0);
						DrawRectRainBow(packet.m_patchesBin, imguiWindow, 0);
					}
					else
					{
						DrawRectNormal(packet.m_entriesBin, imguiWindow, 0.09f);
						DrawRectNormal(packet.m_patchesBin, imguiWindow, 0.09f);
					}
					splitter.SetCurrentChannel(drawList, 1);
					splitter.Merge(drawList);
				}
			}

			ImGui::End();
		}

		#ifdef _DEBUG
			ImGui::SetNextWindowPos(ImVec2(windowWidth / 1.75f, 50), ImGuiCond_Once);
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
			ImGui::ShowMetricsWindow();

			ImGui::SetNextWindowPos(ImVec2(windowWidth / 1.75f, 73), ImGuiCond_Once);
			ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
			ImGui::Begin("Dear ImGui Style Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::ShowStyleEditor();
			ImGui::End();

			ImGui::SetNextWindowCollapsed(true, ImGuiCond_Once);
			ImGui::ShowDemoWindow();
		#endif

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(glfwWindow);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(glfwWindow);
	glfwTerminate();

	if (packet.m_entriesBin)
		ClearBinField(packet.m_entriesBin);

	if (packet.m_patchesBin)
		ClearBinField(packet.m_patchesBin);

	RegCloseKey(regkeyresult);

	return 0;
}