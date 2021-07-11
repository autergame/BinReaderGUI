//author https://github.com/autergame
#include "BinReaderLib.h"

void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
	scanf("press enter to exit.");
	exit(1);
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
	#ifdef TRACY_ENABLE
		ZoneNamedN(mz, "main", true);
	#endif

	printf("loading hashes.\n");
	HashTable* hasht = CreateHashTable(1000);
	AddToHashTable(hasht, "hashes.bintypes.txt");
	AddToHashTable(hasht, "hashes.binfields.txt");
	AddToHashTable(hasht, "hashes.binhashes.txt");
	AddToHashTable(hasht, "hashes.binentries.txt");
#ifdef NDEBUG
	AddToHashTable(hasht, "hashes.game.txt", true);
	AddToHashTable(hasht, "hashes.lcu.txt", true);
#endif
	printf("finised loading hashes.\n");

	RECT rectScreen;
	HWND hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);
	int width = rectScreen.right * 0.75f, height = rectScreen.bottom * 0.75f;
	int PosX = ((rectScreen.right - rectScreen.left) / 2 - width / 2);
	int PosY = ((rectScreen.bottom - rectScreen.top) / 2 - height / 2);

	glfwSetErrorCallback(glfw_error_callback);
	myassert(glfwInit() == GLFW_FALSE)

	glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(width, height, "BinReaderGUI", NULL, NULL);
	myassert(window == NULL)

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glfwSetWindowPos(window, PosX, PosY);
	myassert(gladLoadGL() == 0)

	static ImGuiMemAllocFunc allocfunc = MallocbWrapper;
	static ImGuiMemFreeFunc freefunc = FreebWrapper;
	ImGui::SetAllocatorFunctions(allocfunc, freefunc);
	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = NULL;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	ImVec4* colors = GImGui->Style.Colors;
	colors[ImGuiCol_FrameBg] = ImColor(76, 76, 76, 255);
	colors[ImGuiCol_Header] = ImColor(51, 51, 51, 191);
	colors[ImGuiCol_Button] = ImColor(51, 51, 51, 255);
	colors[ImGuiCol_Border] = ImColor(76, 76, 76, 128);
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
	ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13);
	int FULL_SCREEN_FLAGS = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

	HKEY testkey = nullptr;
	LSTATUS testresult = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", 0, KEY_QUERY_VALUE, &testkey);
	if (testresult != ERROR_SUCCESS) 
	{
		HKEY default_key;
		char currentpath[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, currentpath);
		DWORD pathsize = (DWORD)strlen(currentpath);
		LSTATUS status = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", 0, NULL,
			REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &default_key, NULL);
		if (status != ERROR_SUCCESS) {
			printf("Creating key failed: %d %d.\n", status, GetLastError());
			scanf("press enter to exit.");
			return 1;
		}
		status = RegSetValueExA(default_key, "openpath", 0, REG_EXPAND_SZ, (LPCBYTE)currentpath, pathsize);
		if (status != ERROR_SUCCESS) {
			printf("Setting key value failed: %d %d.\n", status, GetLastError());
			scanf("press enter to exit.");
			return 1;
		}
		status = RegSetValueExA(default_key, "savepath", 0, REG_EXPAND_SZ, (LPCBYTE)currentpath, pathsize);
		if (status != ERROR_SUCCESS) {
			printf("Setting key value failed: %d %d.\n", status, GetLastError());
			scanf("press enter to exit.");
			return 1;
		}
		RegCloseKey(default_key);
	}
	RegCloseKey(testkey);

	HKEY subKey = nullptr;
	LSTATUS status = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", 0, KEY_SET_VALUE, &subKey);
	if (status != ERROR_SUCCESS)
	{
		printf("Opening key failed: %d %d.\n", status, GetLastError());
		scanf("press enter to exit.");
		return 1;
	}

	char tmp[64] = { 0 };
	char buf[32] = { 0 };
	char opencopy[MAX_PATH] = { 0 };
	char openfile[MAX_PATH] = { 0 };
	char savefile[MAX_PATH] = { 0 };
	char openpath[MAX_PATH] = { 0 };
	char savepath[MAX_PATH] = { 0 };
	
	bool rainbow = false;
	bool openchoose = false;
	bool hasbeopened = false;

	uintptr_t treeid = 0;
	PacketBin* packet = NULL;

#define TESTBIN
#if defined TESTBIN && defined _DEBUG
	const char* test = "C:\\Users\\autergame\\Documents\\Visual Studio 2019\\Projects\\BinReader\\Release\\test.bin";
	packet = DecodeBin(_strdup(test), hasht);
	if (packet != NULL)
	{
		treeid = 3;
		memcpy(opencopy, test, strlen(test));
		myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
		if (packet->Version >= 2)
		{
			for (uint32_t i = 0; i < packet->linkedsize; i++)
			{
				packet->LinkedList[i]->id = treeid;
				treeid += 1;
			}
		}
		GetStructIdBin(packet->entriesMap, &treeid);
	}
#endif 

	glClearColor(0.f, 0.f, 0.f, 1.f);
	HWND windowa = glfwGetWin32Window(window);
	while (!glfwWindowShouldClose(window))
	{
		#ifdef TRACY_ENABLE
			FrameMark;
		#endif 	

		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);

		myassert(sprintf(tmp, "BinReaderGUI - FPS: %1.0f", GImGui->IO.Framerate) < 0);
		glfwSetWindowTitle(window, tmp);

		if (GImGui->IO.Framerate < 45.f)
			GImGui->Style.FrameRounding = 0.f;
		else
			GImGui->Style.FrameRounding = 4.f;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));
		ImGui::Begin("Main", 0, FULL_SCREEN_FLAGS);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::MenuItem("Open Bin"))
			{
				DWORD size = 260;
				OPENFILENAMEA ofn;
				memset(&ofn, 0, sizeof(ofn));
				memset(openfile, 0, MAX_PATH);
				memset(openpath, 0, MAX_PATH);
				LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "openpath",
					RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, NULL, openpath, &size);
				if (status != ERROR_SUCCESS) {
					printf("Getting key value failed: %d %d.\n", status, GetLastError());
					scanf("press enter to exit.");
					return 1;
				}
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = windowa;
				ofn.lpstrFile = openfile;
				ofn.nMaxFile = MAX_PATH;
				ofn.lpstrFilter = "*.bin\0";
				ofn.nFilterIndex = 1;
				ofn.lpstrFileTitle = NULL;
				ofn.nMaxFileTitle = 0;
				ofn.lpstrInitialDir = openpath;
				ofn.lpstrTitle = "Open Bin File";
				ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
				if (GetOpenFileNameA(&ofn) == TRUE)
				{
					memset(openpath, 0, MAX_PATH);
					memset(opencopy, 0, MAX_PATH);
					memcpy(openpath, openfile, strlen(openfile));
					memcpy(opencopy, openfile, strlen(openfile));
					strip_filepath(openpath);
					status = RegSetValueExA(subKey, "openpath", 0, REG_EXPAND_SZ, (LPCBYTE)openpath, (DWORD)strlen(openpath));
					if (status != ERROR_SUCCESS) {
						printf("Setting key value failed: %d %d.\n", status, GetLastError());
						scanf("press enter to exit.");
						return 1;
					}
					if (packet != NULL)
						ClearBin(packet->entriesMap);
					packet = DecodeBin(openfile, hasht);
					if (packet != NULL)
					{
						treeid = 3;
						ImGui::GetStateStorage()->Clear();
						myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
						if (packet->Version >= 2)
						{
							for (uint32_t i = 0; i < packet->linkedsize; i++)
							{
								packet->LinkedList[i]->id = treeid;
								treeid += 1;
							}
						}
						GetStructIdBin(packet->entriesMap, &treeid);
					}
				}
			}
			if (opencopy[0] != 0 && packet != NULL)
			{
				if (ImGui::MenuItem("Save Bin"))
				{
					DWORD size = 260;
					OPENFILENAMEA ofn;
					memset(&ofn, 0, sizeof(ofn));
					memset(savefile, 0, MAX_PATH);
					memset(savepath, 0, MAX_PATH);
					LSTATUS status = RegGetValueA(HKEY_CURRENT_USER, "SOFTWARE\\BinReaderGUI", "savepath",
						RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND, NULL, savepath, &size);
					if (status != ERROR_SUCCESS) {
						printf("Getting key value failed: %d %d.\n", status, GetLastError());
						scanf("press enter to exit.");
						return 1;
					}
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = windowa;
					ofn.lpstrFile = savefile;
					ofn.nMaxFile = MAX_PATH;
					ofn.lpstrFilter = "*.bin\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrDefExt = "bin";
					ofn.lpstrInitialDir = savepath;
					ofn.lpstrTitle = "Save Bin File";
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
					if (GetSaveFileNameA(&ofn) == TRUE)
					{
						memset(savepath, 0, MAX_PATH);
						memcpy(savepath, savefile, strlen(savefile));
						strip_filepath(savepath);
						status = RegSetValueExA(subKey, "savepath", 0, REG_EXPAND_SZ, (LPCBYTE)savepath, (DWORD)strlen(savepath));
						if (status != ERROR_SUCCESS) {
							printf("Setting key value failed: %d %d.\n", status, GetLastError());
							scanf("press enter to exit.");
							return 1;
						}
						EncodeBin(savefile, packet);
					}
				}
				if (packet != NULL)
				{
					ImGui::SameLine();
					ImGui::AlignTextToFramePadding();
					if (ImGui::Checkbox("Open/Close All Tree Nodes", &openchoose))
					{
						if (openchoose)
							hasbeopened = true;
						else
							SetTreeCloseState(packet->entriesMap, ImGui::GetCurrentWindow());
					}
				}
				if (packet != NULL)
				{
					ImGui::SameLine();
					ImGui::AlignTextToFramePadding();
					ImGui::Checkbox("RainBow Mode", &rainbow);
				}
			}
			ImGui::EndMenuBar();
		}
		if (opencopy[0] != 0 && packet != NULL)
		{
			ImGui::AlignTextToFramePadding();
			if (hasbeopened)
				ImGui::SetNextItemOpen(true);
			if (ImGui::TreeNodeEx((void*)0, ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen, opencopy))
			{
				uint32_t treesize = 0;
				GetTreeSize(packet->entriesMap, &treesize);

				ImGuiWindow* windowi = ImGui::GetCurrentWindow();
				ImDrawList* drawlist = windowi->DrawList;
				ImDrawListSplitter splitter;
				splitter.Split(drawlist, 2);
				splitter.SetCurrentChannel(drawlist, 1);

				ImGui::AlignTextToFramePadding();
				ImGui::Indent(); ImGui::Text("Tree Size"); ImGui::SameLine();
				ImGui::Text("="); ImGui::SameLine(); ImGui::Text("%d", treesize);
				ImGui::AlignTextToFramePadding();
				ImGui::Text("Version"); ImGui::SameLine();
				ImGui::Text("="); ImGui::SameLine();
				char* string = InputText(buf, 1);
				if (string != NULL)
				{
					sscanf(string, "%" PRIu32, &packet->Version);
					myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
					freeb(string);
				}
				if (packet->Version >= 2)
				{
					ImVec2 cursor = windowi->DC.CursorPos;
					ImGui::AlignTextToFramePadding();
					if (ImGui::TreeNodeEx((void*)2, ImGuiTreeNodeFlags_Framed, "LinkedList"))
					{
						ImGui::Indent();
						for (uint32_t i = 0; i < packet->linkedsize; i++)
						{
							char* str = InputText(packet->LinkedList[i]->str, packet->LinkedList[i]->id);
							if (str != NULL)
							{
								freeb(packet->LinkedList[i]->str);
								packet->LinkedList[i]->str = str;
							}
						}
						if (ImGui::Button("Add new item"))
						{
							treeid += 1;
							packet->linkedsize += 1;
							packet->LinkedList = (PacketId**)reallocb(packet->LinkedList, packet->linkedsize * sizeof(PacketId*));
							packet->LinkedList[packet->linkedsize-1] = (PacketId*)callocb(1, sizeof(PacketId));
							packet->LinkedList[packet->linkedsize-1]->str = (char*)callocb(1, 1);
							packet->LinkedList[packet->linkedsize-1]->id = treeid;
						}
						ImGui::Unindent();
						ImGui::TreePop();

						cursor.x -= 3.f;
						ImVec2 cursormax = ImVec2(windowi->WorkRect.Max.x,
							windowi->DC.CursorMaxPos.y);
						ImGui::ItemAdd(ImRect(cursor, cursormax), 0);
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
						splitter.SetCurrentChannel(drawlist, 0);
						drawlist->AddRectFilled(cursor, cursormax, col);
						drawlist->AddRect(cursor, cursormax, rainbow ? IM_COL32(255, 0, 0, 255) : IM_COL32_GREY);
						splitter.SetCurrentChannel(drawlist, 1);
					}
				}
				Map* mp = (Map*)packet->entriesMap->data;
				#ifdef TRACY_ENABLE
					ZoneNamedN(mpz, "EntryMap", true);
				#endif
				for (uint32_t i = 0; i < mp->itemsize; i++)
				{
					if (mp->items[i]->key != NULL)
					{
						if (hasbeopened)
							ImGui::SetNextItemOpen(true);
						mp->items[i]->cursormin = windowi->DC.CursorPos;
						ImGui::AlignTextToFramePadding();
						mp->items[i]->expanded = ImGui::TreeNodeEx((void*)mp->items[i]->id, ImGuiTreeNodeFlags_Framed |
							ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth, "");
						mp->items[i]->idim = windowi->IDStack.back();
						ImGui::AlignTextToFramePadding();
						if (IsItemVisible(windowi))
						{
							ImVec2 cursor, cursore;
							#ifdef _DEBUG
							if (ImGui::IsItemHovered())
								ImGui::SetTooltip("%d", mp->items[i]->id);
							#endif
							ImGui::SameLine();
							InputTextHash(hasht, (uint32_t*)mp->items[i]->key->data, mp->items[i]->key->id);
							ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
							GetValueFromType(mp->items[i]->value, hasht, &treeid, windowi,
								hasbeopened, &mp->items[i]->expanded, &cursor);
							cursore = ImGui::GetCursorPos();
							ImGui::SetCursorPos(cursor);
							if (BinFieldDelete(mp->items[i]->id+1))
							{
								ClearBin(mp->items[i]->key);
								ClearBin(mp->items[i]->value);
								mp->items[i]->key = NULL;
								mp->items[i]->value = NULL;
							}
							ImGui::SetCursorPos(cursore);
						} 
						else {		
							ImGui::SameLine();
							GetValueFromType(mp->items[i]->value, hasht, &treeid, windowi,
								hasbeopened, &mp->items[i]->expanded);
						}
						if (mp->items[i]->expanded)
						{
							ImGui::TreePop();
							if (((PointerOrEmbed*)mp->items[i]->value->data)->name == NULL)
							{
								mp->items[i]->cursormax.x = 0.f;
								continue;
							}
							mp->items[i]->cursormax = ImVec2(windowi->WorkRect.Max.x,
								windowi->DC.CursorMaxPos.y);
						} 
						else
							mp->items[i]->isover = false;
					}
				}
				if (IsItemVisible(windowi))
				{
					if (ImGui::Button("Add new item"))
					{
						mp->itemsize += 1;
						mp->items = (Pair**)reallocb(mp->items, mp->itemsize * sizeof(Pair*));
						mp->items[mp->itemsize - 1] = (Pair*)callocb(1, sizeof(Pair));
						mp->items[mp->itemsize - 1]->key = BinFieldClean(mp->keyType, (Type)mp->current2, (Type)mp->current3);
						mp->items[mp->itemsize - 1]->value = BinFieldClean(mp->valueType, (Type)mp->current4, (Type)mp->current5);
						GetStructIdBin(packet->entriesMap, &treeid);
					}
				} 
				else {
					NewLine(windowi);
				}
				ImGui::Unindent();
				ImGui::TreePop();
				if (hasbeopened)
					hasbeopened = false;
				splitter.SetCurrentChannel(drawlist, 0);
				if (rainbow)
					DrawRectRainBow(packet->entriesMap, windowi, 0);
				else 
					DrawRectNormal(packet->entriesMap, windowi, 0.09f);
				splitter.SetCurrentChannel(drawlist, 1);
				splitter.Merge(drawlist);
			}		
		}
		ImGui::End();

		#ifdef _DEBUG
			ImGui::ShowMetricsWindow();
			ImGui::Begin("Dear ImGui Style Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::ShowStyleEditor();
			ImGui::End();
			ImGui::SetWindowCollapsed("Dear ImGui Style Editor", true, ImGuiCond_Once);
			ImGui::SetWindowCollapsed("Dear ImGui Metrics/Debugger", true, ImGuiCond_Once);
			ImGui::SetWindowPos("Dear ImGui Style Editor", ImVec2(width/1.75f, 73), ImGuiCond_Once);
			ImGui::SetWindowPos("Dear ImGui Metrics/Debugger", ImVec2(width/1.75f, 50), ImGuiCond_Once);
		#endif

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	if (packet != NULL)
		ClearBin(packet->entriesMap);

	return 0;
}