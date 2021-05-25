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
	while (end > fname && *end != '\\') {
		--end;
	}
	if (end > fname) {
		*end = '\0';
	}
}

int main()
{
	#ifdef TRACY_ENABLE
		ZoneScopedN("main");
	#endif
	RECT rectScreen;
	int width = 1024, height = 576;
	HWND hwndScreen = GetDesktopWindow();
	GetWindowRect(hwndScreen, &rectScreen);
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

	ImGui::CreateContext();
	ImGui::GetIO().IniFilename = NULL;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
	ImVec4* colors = GImGui->Style.Colors;
	colors[ImGuiCol_FrameBg] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_Header] = ImVec4(0.2f, 0.2f, 0.2f, 0.75f);
	colors[ImGuiCol_Button] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	colors[ImGuiCol_Border] = ImVec4(0.3f, 0.3f, 0.3f, 0.5f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, GImGui->Style.FramePadding.x * 3.0f - 2.0f);
	ImGui::GetIO().Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13);
	int FULL_SCREEN_FLAGS = ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | 
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_HorizontalScrollbar;

	printf("loading hashes.\n");
	HashTable* hasht = createHashTable(10000);
	addhash(hasht, "hashes.bintypes.txt");
	addhash(hasht, "hashes.binfields.txt");
	addhash(hasht, "hashes.binhashes.txt");
	addhash(hasht, "hashes.binentries.txt");
	addhash(hasht, "hashes.game.txt", true);
	printf("finised loading hashes.\n");

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

	MSG msg = { 0 };
	char tmp[64] = { 0 };
	char buf[32] = { 0 };
	char opencopy[MAX_PATH] = { 0 };
	char openfile[MAX_PATH] = { 0 };
	char savefile[MAX_PATH] = { 0 };
	char openpath[MAX_PATH] = { 0 };
	char savepath[MAX_PATH] = { 0 };
	
	bool openchoose = false;
	bool hasbeopened = false;
	uintptr_t treebefore = 0;
	PacketBin* packet = NULL;

#ifdef  _DEBUG
	const char* test = "C:\\Users\\autergame\\Documents\\Visual Studio 2019\\Projects\\BinReader\\Release\\test.bin";
	packet = decode(_strdup(test), hasht);
	if (packet != NULL)
	{
		treebefore = 3;
		memcpy(opencopy, test, strlen(test));
		myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
		if (packet->Version >= 2)
		{
			for (uint32_t i = 0; i < packet->linkedsize; i++)
			{
				packet->LinkedList[i]->id = treebefore;
				treebefore += 1;
			}
		}
		getstructidbin(packet->entriesMap, &treebefore);
	}
#endif 

	glClearColor(.0f, .0f, .0f, 1.0f);
	HWND windowa = glfwGetWin32Window(window);
	while (!glfwWindowShouldClose(window))
	{
		#ifdef _DEBUG
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

		if (GImGui->IO.Framerate < 30.f)
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		else
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

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
						cleanbin(packet->entriesMap);
					packet = decode(openfile, hasht);
					if (packet != NULL)
					{
						treebefore = 3;
						ImGui::GetStateStorage()->Clear();
						myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
						if (packet->Version >= 2)
						{
							for (uint32_t i = 0; i < packet->linkedsize; i++)
							{
								packet->LinkedList[i]->id = treebefore;
								treebefore += 1;
							}
						}
						getstructidbin(packet->entriesMap, &treebefore);
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
						encode(savefile, packet);
					}
				}
				if (packet != NULL)
				{
					if (ImGui::MenuItem("Open/Close All Tree Nodes"))
					{
						openchoose = !openchoose;
						if (openchoose == false)
							settreeopenstate(packet->entriesMap, ImGui::GetCurrentWindow());
						else
							hasbeopened = true;
					}
				}
			}
			ImGui::EndMenuBar();
		}
		if (opencopy[0] != 0 && packet != NULL)
		{
			ImGui::AlignTextToFramePadding();
			if (ImGui::TreeNodeEx((void*)0, ImGuiTreeNodeFlags_Framed |
				ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DefaultOpen, opencopy))
			{
				ImGui::AlignTextToFramePadding();
				ImGui::Indent(); ImGui::Text("Version"); ImGui::SameLine();
				ImGui::Text("="); ImGui::SameLine();
				char* string = inputtext(buf, 1);
				if (string != NULL)
				{
					sscanf(string, "%" PRIu32, &packet->Version);
					myassert(sprintf(buf, "%" PRIu32, packet->Version) < 0);
					free(string);
				}
				if (packet->Version >= 2)
				{
					ImGui::AlignTextToFramePadding();
					if (ImGui::TreeNodeEx((void*)2, ImGuiTreeNodeFlags_Framed, "LinkedList"))
					{
						ImGui::Indent();
						for (uint32_t i = 0; i < packet->linkedsize; i++)
						{
							char* str = inputtext(packet->LinkedList[i]->str, packet->LinkedList[i]->id);
							if (str != NULL)
							{
								free(packet->LinkedList[i]->str);
								packet->LinkedList[i]->str = str;
							}
						}
						if (ImGui::Button("Add new item"))
						{
							treebefore += 1;
							packet->linkedsize += 1;
							packet->LinkedList = (PacketId**)realloc(packet->LinkedList, packet->linkedsize * sizeof(PacketId*));
							packet->LinkedList[packet->linkedsize-1] = (PacketId*)calloc(1, sizeof(PacketId));
							packet->LinkedList[packet->linkedsize-1]->str = (char*)calloc(1, 1);
							packet->LinkedList[packet->linkedsize-1]->id = treebefore;
						}
						ImGui::Unindent();
						ImGui::TreePop();
					}
				}
				ImGuiWindow* windowi = ImGui::GetCurrentWindow();
				Map* mp = (Map*)packet->entriesMap->data;
				#ifdef TRACY_ENABLE
					ZoneNamedN(mpz, "EntryMap", true);
				#endif
				for (uint32_t i = 0; i < mp->itemsize; i++)
				{
					if (mp->items[i]->key != NULL)
					{
						ImGui::AlignTextToFramePadding();
						if (hasbeopened)
							ImGui::SetNextItemOpen(true);
						bool treeopen = ImGui::TreeNodeEx((void*)mp->items[i]->id1, ImGuiTreeNodeFlags_Framed |
							ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth, "");
						mp->items[i]->idim = windowi->IDStack.back();
						#ifdef _DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%d", mp->items[i]->id1);
						#endif
						ImGui::SameLine();
						if (IsItemVisibleClip(windowi))
						{
							ImVec2 cursor, cursore;
							inputtextmod(hasht, (uint32_t*)mp->items[i]->key->data, mp->items[i]->key->id);
							ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
							getvaluefromtype(mp->items[i]->value, hasht, &treebefore, windowi, hasbeopened, &treeopen, &cursor);
							cursore = ImGui::GetCursorPos();
							ImGui::SetCursorPos(cursor);
							if (binfielddelete(mp->items[i]->id2))
							{
								cleanbin(mp->items[i]->key);
								cleanbin(mp->items[i]->value);
								mp->items[i]->key = NULL;
								mp->items[i]->value = NULL;
							}
							ImGui::SetCursorPos(cursore);
						} else {
							getvaluefromtype(mp->items[i]->value, hasht, &treebefore, windowi, hasbeopened, &treeopen);
						}
					}
				}
				if (IsItemVisible(windowi))
				{
					if (ImGui::Button("Add new item"))
					{
						mp->itemsize += 1;
						mp->items = (Pair**)realloc(mp->items, mp->itemsize * sizeof(Pair*)); myassert(mp->items == NULL);
						mp->items[mp->itemsize - 1] = (Pair*)calloc(1, sizeof(Pair)); myassert(mp->items[mp->itemsize - 1] == NULL);
						mp->items[mp->itemsize - 1]->key = binfieldclean(mp->keyType, (Type)mp->current2, (Type)mp->current3);
						mp->items[mp->itemsize - 1]->value = binfieldclean(mp->valueType, (Type)mp->current4, (Type)mp->current5);
						getstructidbin(packet->entriesMap, &treebefore);
					}
				} else {
					ImGui::NewLine();
				}
				ImGui::Unindent();
				ImGui::TreePop();
				if (hasbeopened)
					hasbeopened = false;
			}		
		}
		ImGui::End();
		ImGui::PopStyleVar();

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

	if (packet != NULL)
		cleanbin(packet->entriesMap);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}