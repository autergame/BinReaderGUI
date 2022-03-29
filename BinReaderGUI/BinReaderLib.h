//author https://github.com/autergame


static char* truestr = _strdup("true");
static char* falsestr = _strdup("false");


#pragma region BinTextsHandlers

char* InputTextWithSusjestion(const int id, char* inner, TernaryTree& ternaryT)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(itwsz, "InputTextWithSusjestion", true);
#endif

	ImGui::PushID(id);

	ImGuiWindow* mainwindow = GImGui->CurrentWindow;

	char inputtextname[16] = { '\0' };
	myassert(sprintf_s(inputtextname, 16, "##it_%08x", id) <= 0)
	ImGuiID idinput = mainwindow->GetIDNoKeepAlive(inputtextname);

	ImGuiID idscroll = 0;
	char comboxwindowname[16] = { '\0' };
	myassert(sprintf_s(comboxwindowname, 16, "##itcb_%08x", id) <= 0)
	ImGuiWindow* comboxwindow = ImGui::FindWindowByName(comboxwindowname);
	if (comboxwindow)
	{
		ImGuiID active_id = ImGui::GetActiveID();
		if (active_id)
		{
			ImGuiID scollidx = ImGui::GetWindowScrollbarID(comboxwindow, ImGuiAxis_X);
			ImGuiID scollidy = ImGui::GetWindowScrollbarID(comboxwindow, ImGuiAxis_Y);

			if (active_id == scollidx)
				idscroll = scollidx;
			else if (active_id == scollidy)
				idscroll = scollidy;

			if (idscroll != 0)
			{				
				ImGui::SetActiveID(idinput, mainwindow);
				ImGui::SetFocusID(idinput, mainwindow);
				ImGui::FocusWindow(mainwindow);
			}
		}
	}

	ImGuiInputTextState* inputstate = ImGui::GetInputTextState(idinput);
	if (inputstate && idscroll != 0)
		inputstate->Stb.cursor = inputstate->Stb.select_end = inputstate->Stb.select_start = inputstate->CurLenW;

	static char userdata[512] = { '\0' };
	myassert(memcpy(userdata, inner, strlen(inner) + 1) != userdata);

	ImGui::SetNextItemWidth(ImMin(ImGui::CalcTextSize(inner).x + GImGui->FontSize * 4.f,
		ImGui::GetContentRegionAvail().x - GImGui->FontSize * 4.f));

	bool ret = ImGui::InputText(inputtextname, userdata, 512);
	bool deactivated = ImGui::IsItemDeactivated();
	bool inputhover = ImGui::IsItemHovered();
	ImGuiID inputid = ImGui::GetItemID();

#ifdef _DEBUG
	if (inputhover)
		ImGui::SetTooltip("%d", id);
#endif

	struct ternarypopup
	{
		bool open = false;

		struct ternarypopupvars
		{	
			int sujestionsize = 0;
			uint32_t userdatahashold = 0;
			const char* biggesttext = nullptr;

			int count = 0;
			int startatold = -1;
			const char** output = nullptr;
		};

		ternarypopupvars* vars = nullptr; 

		~ternarypopup() {
			if (vars != nullptr)
			{
				if (vars->output != nullptr)
					delete vars->output;
				delete vars;
			}
		}
	};

	char* userdatacopied = nullptr;
	static mi_unordered_map<int, ternarypopup> popupmap;

	if (userdata[0] != '\0')
	{
		popupmap[id].open |= ImGui::IsItemActive();
		if (popupmap[id].open)
		{
			for (auto it = popupmap.begin(); it != popupmap.end(); it++)
				it->second.open = false;
			popupmap[id].open = true;

			if (popupmap[id].vars == nullptr)
				popupmap[id].vars = new ternarypopup::ternarypopupvars;

			uint32_t userdatahash = FNV1Hash(userdata, strlen(userdata));
			bool userdatachanged = popupmap[id].vars->userdatahashold != userdatahash;
			if (userdatachanged)
			{
				popupmap[id].vars->sujestionsize = ternaryT.SujestionsSize(userdata, &popupmap[id].vars->biggesttext);
				popupmap[id].vars->userdatahashold = userdatahash;
			}

			if (popupmap[id].vars->sujestionsize > 0)
			{
				int startat = 0;

				if (comboxwindow)
				{
					float scrolly = comboxwindow->Scroll.y;
					if (scrolly > 0.f)
					{
						float scroll_ratio = scrolly / comboxwindow->ScrollMax.y;
						startat = (int)floorf((scroll_ratio * popupmap[id].vars->sujestionsize) - (scroll_ratio * 10.f));
					}
				}

				if (popupmap[id].vars->startatold != startat || userdatachanged)
				{
					if (popupmap[id].vars->output == nullptr)
						popupmap[id].vars->output = new const char*[12]{};

					popupmap[id].vars->count = ternaryT.Sujestions(userdata, popupmap[id].vars->output, startat, 12);
					popupmap[id].vars->startatold = startat;
				}

				if (popupmap[id].vars->count > 0)
				{
					ImRect inputsize = GImGui->LastItemData.Rect;
					ImVec2 popuppos = ImVec2(inputsize.Min.x, 0.f);

					float maxvisible = ImMin(popupmap[id].vars->count, 10) + 1.25f;
					float popupsizey = (GImGui->FontSize + GImGui->Style.FramePadding.y) * maxvisible;

					if (inputsize.Max.y < mainwindow->InnerClipRect.Max.y - popupsizey)
						popuppos.y = inputsize.Max.y;
					else
						popuppos.y = inputsize.Min.y - popupsizey;

					float textmaxsize = popuppos.x + (ImGui::CalcTextSize(popupmap[id].vars->biggesttext).x + GImGui->FontSize * 3.f);
					float maxtextclip = mainwindow->InnerClipRect.Max.x - GImGui->FontSize;
					float textsize = 0.f;
					if (textmaxsize < maxtextclip)
						textsize = textmaxsize - popuppos.x;
					else
						textsize = maxtextclip - popuppos.x;

					ImVec2 popupsize = ImVec2(textsize, popupsizey);

					ImGui::SetNextWindowPos(popuppos);
					ImGui::SetNextWindowSize(popupsize);

					ImGui::PushAllowKeyboardFocus(false);

					if (ImGui::Begin(comboxwindowname, &popupmap[id].open,
						ImGuiWindowFlags_HorizontalScrollbar |
						ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNavFocus |
						ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus))
					{
						ImGui::BringWindowToDisplayFront(GImGui->CurrentWindow);

						for (int i = 0; i < startat; i++)
						{
							ImRect total_bb(GImGui->CurrentWindow->DC.CursorPos,
								ImVec2(GImGui->CurrentWindow->DC.CursorPos.x,
									GImGui->CurrentWindow->DC.CursorPos.y + GImGui->FontSize + GImGui->Style.FramePadding.y * 2.f
								)
							);
							ImGui::ItemSize(total_bb, GImGui->Style.FramePadding.y);
						}

						for (int i = 0; i < popupmap[id].vars->count; i++)
						{
							ImGui::PushID(i);
							if (ImGui::Selectable(popupmap[id].vars->output[i], false,
								ImGuiSelectableFlags_SelectOnClick | ImGuiSelectableFlags_NoHoldingActiveID))
							{
								size_t size = strlen(popupmap[id].vars->output[i]) + 1;
								userdatacopied = new char[size];
								myassert(memcpy(userdatacopied, popupmap[id].vars->output[i], size) != userdatacopied)

								ret = false;
								deactivated = false;
								i = popupmap[id].vars->count;
								popupmap[id].open = false;
							}
							ImGui::PopID();
						}

						for (int i = startat + 10; i < popupmap[id].vars->sujestionsize; i++)
						{
							ImRect total_bb(GImGui->CurrentWindow->DC.CursorPos,
								ImVec2(GImGui->CurrentWindow->DC.CursorPos.x,
									GImGui->CurrentWindow->DC.CursorPos.y + GImGui->FontSize + GImGui->Style.FramePadding.y * 2.f
								)
							);
							ImGui::ItemSize(total_bb, GImGui->Style.FramePadding.y);
						}
					}
					ImGui::End();

					ImGui::PopAllowKeyboardFocus();

					popupmap[id].open &= !(GImGui->HoveredWindow == mainwindow && ImGui::IsMouseDown(ImGuiMouseButton_Left));
					if (popupmap[id].open)
					{
						if (idscroll != 0)
						{
							ImGui::SetActiveID(idscroll, comboxwindow);
							ImGui::SetFocusID(idscroll, comboxwindow);
							ImGui::FocusWindow(comboxwindow);
						}
					}
					else
					{
						if (comboxwindow)
						{
							comboxwindow->Scroll = ImVec2(0.f, 0.f);
						}
					}
				}
			}
		}
	}

	if (deactivated)
	{
		ImGuiInputTextState* state = &GImGui->InputTextState;
		if (state->ID == inputid)
		{
			popupmap[id].open = false;
			userdatacopied = new char[state->CurLenA + 1];
			myassert(memcpy(userdatacopied, state->TextA.Data, state->CurLenA + 1) != userdatacopied)
		}
	}
	else if (ret)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
		   (!inputhover && (GImGui->HoveredWindow == mainwindow && ImGui::IsMouseDown(ImGuiMouseButton_Left))))
		{
			popupmap[id].open = false;
			size_t size = strlen(userdata) + 1;
			userdatacopied = new char[size];
			myassert(memcpy(userdatacopied, userdata, size) != userdatacopied)
		}
	}

	ImGui::PopID();

	return userdatacopied;
}

char* InputText(char* inner, const int id, float sized = 0.f)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(itz, "InputText", true);
#endif

	static char userdata[512] = { '\0' };
	myassert(memcpy(userdata, inner, strlen(inner) + 1) != userdata);

	if (sized == 0)
		sized = ImGui::CalcTextSize(inner).x;

	ImGui::PushID(id);
	ImGui::SetNextItemWidth(ImMin(sized + GImGui->FontSize * 4.f,
		ImGui::GetContentRegionAvail().x - GImGui->FontSize * 4.f));
	bool ret = ImGui::InputText("##it", userdata, 256);
	ImGui::PopID();

	bool deactivated = ImGui::IsItemDeactivated();
	bool inputhover = ImGui::IsItemHovered();
	ImGuiID inputid = ImGui::GetItemID();

#ifdef _DEBUG
	if (inputhover)
		ImGui::SetTooltip("%d", id);
#endif

	char* userdatacopied = nullptr;

	if (deactivated)
	{
		ImGuiInputTextState* state = &GImGui->InputTextState;
		if (state->ID == inputid)
		{
			userdatacopied = new char[state->CurLenA + 1];
			myassert(memcpy(userdatacopied, state->TextA.Data, state->CurLenA + 1) != userdatacopied)
		}
	}
	else if (ret)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
		   (!inputhover && ImGui::IsMouseDown(ImGuiMouseButton_Left)))
		{
			size_t size = strlen(userdata) + 1;
			userdatacopied = new char[size];
			myassert(memcpy(userdatacopied, userdata, size) != userdatacopied)
		}
	}

	return userdatacopied;
}

mi_string HashToString(HashTable& hashT, const uint32_t hashValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(htsz, "HashToString", true);
#endif
	mi_string strvalue = hashT.Lookup(hashValue);
	if (strvalue.size() == 0)
	{
		strvalue.reserve(16);
		myassert(sprintf_s(strvalue.data(), 16, "0x%08" PRIX32, hashValue) <= 0)
	}
	return strvalue;
}

mi_string HashToStringxx(HashTable& hashT, const uint64_t hashValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(htsxz, "HashToStringxx", true);
#endif
	mi_string strvalue = hashT.Lookup(hashValue);
	if (strvalue.size() == 0)
	{
		strvalue.reserve(32);
		myassert(sprintf_s(strvalue.data(), 32, "0x%016" PRIX64, hashValue) <= 0)
	}
	return strvalue;
}

void InputTextHash(uint32_t& hashValue, const int id, HashTable& hashT, TernaryTree& ternaryT)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(itmz, "InputTextHash", true);
#endif
	char* string = InputTextWithSusjestion(id, HashToString(hashT, hashValue).data(), ternaryT);
	if (string != nullptr)
	{
		if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
			hashValue = strtoul(string, nullptr, 16);
		else
		{
			hashValue = FNV1Hash(string, strlen(string));
			hashT.Insert(hashValue, string);
			delete string;
		}
	}
}

void InputTextHashxx(uint64_t& hashValue, const int id, HashTable& hashT, TernaryTree& ternaryT)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(itmxz, "InputTextHashxx", true);
#endif
	char* string = InputTextWithSusjestion(id, HashToStringxx(hashT, hashValue).data(), ternaryT);
	if (string != nullptr)
	{
		if (string[0] == '0' && (string[1] == 'x' || string[1] == 'X'))
			hashValue = strtoull(string, nullptr, 16);
		else
		{
			hashValue = XXHash(string, strlen(string));
			hashT.Insert(hashValue, string);
			delete string;
		}
	}
}

void FloatArray(float* floatArr, size_t size, int id)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(faz, "FloatArray", true);
#endif
	int lengthm = 0;
	size_t arrindex = 0;
	mi_vector<mi_string> buf(size);
	for (size_t i = 0; i < size; i++)
	{
		buf[i] = mi_string(64, '\0');
		int length = sprintf_s(buf[i].data(), 64, "%g", floatArr[i]);
		myassert(length <= 0)
		if (length > lengthm)
		{
			lengthm = length;
			arrindex = i;
		}
	}
	float indent = ImGui::GetCurrentWindow()->DC.CursorPos.x;
	float stringd = ImGui::CalcTextSize(buf[arrindex].c_str()).x;
	for (int i = 0; i < size; i++)
	{
		char* string = InputText(buf[i].data(), id + i, stringd);
		if (string)
		{
			myassert(sscanf_s(string, "%g", &floatArr[i]) <= 0)
			delete[] string;
		}
		if (size == 16)
		{
			if ((i + 1) % 4 == 0 && i != 15)
			{
				ImGui::ItemSize(ImVec2(0.f, GImGui->Style.FramePadding.y));
				ImGui::SameLine(indent);
			}
			else if (i < size - 1)
				ImGui::SameLine();
		}
		else if (i < size - 1)
			ImGui::SameLine();
	}
}

void GetArrayCountFromBinField(BinField *binValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(gacz, "GetArrayCountFromBinField", true);
#endif
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::CONTAINER:
		case BinType::STRUCT:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			ImGui::Text("%d Item(s)", cs->items.size());
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			ImGui::Text("%d Item(s)", pe->items.size());
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			ImGui::Text("%d Item(s)", map->items.size());
			break;
		}
	}
}

void GetArrayTypeFromBinField(BinField *binValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(gatz, "GetArrayTypeFromBinField", true);
#endif
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::CONTAINER:
		case BinType::STRUCT:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			ImGui::Text("[%s]", Type_strings[(uint8_t)cs->valueType]);
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			ImGui::Text("[%s,%s]", Type_strings[(uint8_t)map->keyType], Type_strings[(uint8_t)map->valueType]);
			break;
		}
	}
}

#pragma endregion

void ClearBinField(BinField *binValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(cbz, "ClearBinField", 50, true);
#endif
	switch (binValue->type)
	{
		case BinType::Float32:
		case BinType::VEC2:
		case BinType::VEC3:
		case BinType::VEC4:
		case BinType::MTX44:
			delete binValue->data->floatv;
			break;
		case BinType::RGBA:
			delete binValue->data->rgba;
			break;
		case BinType::STRING:
			delete binValue->data->string;
			break;
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
				ClearBinField(cs->items[i].value);
			delete cs;
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
					ClearBinField(pe->items[i].value);
			}
			delete pe;
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				ClearBinField(map->items[i].key);
				ClearBinField(map->items[i].value);
			}
			delete map;
			break;
		}
	}
	delete binValue->data;
	delete binValue;
}

void ContructIdForBinField(BinField *binValue, int& tree)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(gsibz, "ContructIdForBinField", 50, true);
#endif
	switch (binValue->type)
	{
		case BinType::FLAG:
		case BinType::HASH:
		case BinType::LINK:
		case BinType::RGBA:
		case BinType::BOOLB:
		case BinType::SInt8:
		case BinType::UInt8:
		case BinType::SInt16:
		case BinType::UInt16:
		case BinType::SInt32:
		case BinType::UInt32:
		case BinType::SInt64:
		case BinType::UInt64:
		case BinType::STRING:
		case BinType::Float32:
		case BinType::WADENTRYLINK:
		{
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 1;
			}
			break;
		}
		case BinType::VEC2:
		{
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 2;
			}
			break;
		}
		case BinType::VEC3:
		{
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 3;
			}
			break;
		}
		case BinType::VEC4:
		{
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 4;
			}
			break;
		}
		case BinType::MTX44:
		{
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 16;
			}
			break;
		}
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			if (binValue->id == 0)
			{
				binValue->id = tree;
			    tree += 1;
			}
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				if (cs->items[i].id == 0)
				{
					cs->items[i].id = tree;
					tree += 2;
				}
				ContructIdForBinField(cs->items[i].value, tree);
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 5;
			}
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					if (pe->items[i].id == 0)
					{
						pe->items[i].id = tree;
						tree += 3;
					}
					ContructIdForBinField(pe->items[i].value, tree);
				}
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			if (binValue->id == 0)
			{
				binValue->id = tree;
				tree += 2;
			}
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				if (map->items[i].id == 0)
				{
					map->items[i].id = tree;
					tree += 2;
				}
				ContructIdForBinField(map->items[i].key, tree);
				ContructIdForBinField(map->items[i].value, tree);
			}
			break;
		}
	}
}

void BinTreeToList(BinField *binValue, mi_vector<BinField*>& list)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(bttlz, "BinTreeToList", 50, true);
#endif
	list.emplace_back(binValue);
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
				BinTreeToList(cs->items[i].value, list);
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
					BinTreeToList(pe->items[i].value, list);
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				BinTreeToList(map->items[i].key, list);
				BinTreeToList(map->items[i].value, list);
			}
			break;
		}
	}
}

void GetTotalBinFieldSize(BinField *binValue, uint32_t& size)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(gtsz, "GetTotalBinFieldSize", 50, true);
#endif
	size += 1;
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				GetTotalBinFieldSize(cs->items[i].value, size);
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					GetTotalBinFieldSize(pe->items[i].value, size);
				}
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				GetTotalBinFieldSize(map->items[i].key, size);
				GetTotalBinFieldSize(map->items[i].value, size);
			}
			break;
		}
	}
}

bool CanChangeBackcolor(BinField *binValue)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(ccbcz, "CanChangeBackcolor", 50, true);
#endif
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				if (IsComplexBinType(cs->valueType))
				{
					if (cs->items[i].isOver)
						return false;
				}
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					if (IsComplexBinType(pe->items[i].value->type))
					{
						if (pe->items[i].isOver)
							return false;
					}
				}
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				if (map->items[i].isOver)
					return false;
			}
			break;
		}
	}
	return true;
}

void SetTreeCloseState(BinField *binValue, ImGuiWindow& imguiWindow)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(stosz, "SetTreeOpenState", 50, true);
#endif
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				if (IsComplexBinType(cs->valueType))
				{
					cs->items[i].cursorMax.x = 0.f;
					imguiWindow.DC.StateStorage->SetInt(cs->items[i].idim, 0);
					SetTreeCloseState(cs->items[i].value, imguiWindow);
				}
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				imguiWindow.DC.StateStorage->SetInt(pe->idim, 0);
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					if (IsComplexBinType(pe->items[i].value->type))
					{
						pe->items[i].cursorMax.x = 0.f;
						imguiWindow.DC.StateStorage->SetInt(pe->items[i].idim, 0);
						SetTreeCloseState(pe->items[i].value, imguiWindow);
					}
				}
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				map->items[i].cursorMax.x = 0.f;
				imguiWindow.DC.StateStorage->SetInt(map->items[i].idim, 0);
				if (IsComplexBinType(map->keyType))
					SetTreeCloseState(map->items[i].key, imguiWindow);
				if (IsComplexBinType(map->valueType))
					SetTreeCloseState(map->items[i].value, imguiWindow);
			}
			break;
		}
	}
}

BinField *BinFieldNewClean(BinType current1, BinType current2, BinType current3)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(bfcz, "BinFieldNewClean", true);
#endif
	BinField *result = new BinField;
	result->type = current1;
	switch (current1)
	{
		case BinType::POINTER:
		case BinType::EMBEDDED:
			result->data->pe = new PointerOrEmbed;
			break;
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			result->data->cso = new ContainerOrStructOrOption;
			result->data->cso->valueType = current2;
			break;
		}
		case BinType::MAP:
		{
			result->data->map = new Map;
			result->data->map->keyType = current2;
			result->data->map->valueType = current3;
			break;
		}
	}
	return result;
}

bool IsItemVisible(ImGuiWindow& imguiWindow)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(iivz, "IsItemVisible", true);
#endif
	ImVec2 cursor = imguiWindow.DC.CursorPos;
	ImRect clip = imguiWindow.OuterRectClipped;
#ifdef NDEBUG
	clip.Min.y -= 50; clip.Max.y += 50;
#else
	clip.Min.y += 50; clip.Max.y -= 50;
#endif
	return cursor.y > clip.Min.y && cursor.y < clip.Max.y;
}

void MyNewLine(ImGuiWindow& imguiWindow)
{
	ImRect total_bb(imguiWindow.DC.CursorPos,
		ImVec2(imguiWindow.DC.CursorPos.x,
			imguiWindow.DC.CursorPos.y + GImGui->FontSize + GImGui->Style.FramePadding.y * 2.f
		)
	);
	ImGui::ItemSize(total_bb, GImGui->Style.FramePadding.y);
}

bool MyButtonEx(const int ide)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(bez, "ButtonExe", true);
#endif
	ImGuiWindow& imguiWindow = *ImGui::GetCurrentWindow();
	if (imguiWindow.SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = imguiWindow.GetID(ide);

	ImVec2 pos = ImVec2(imguiWindow.DC.CursorPos.x - g.FontSize / 2.f, imguiWindow.DC.CursorPos.y);
	ImVec2 size = ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.f;

	const ImRect bb(pos, pos + size);
	ImGui::ItemSize(size, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
		return false;

	ImGuiButtonFlags flags = 0;
	if (g.CurrentItemFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;
	bool hovered, held;
	bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

	ImVec2 center = bb.GetCenter();
	ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : 
		hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	imguiWindow.DrawList->AddCircleFilled(center, ImMax(2.f, g.FontSize * 0.5f + 1.f), col, 12);
	float cross_extent = g.FontSize * 0.5f * 0.7071f - 1.f;
	ImU32 cross_col = ImGui::GetColorU32(ImGuiCol_Text);
	center -= ImVec2(0.5f, 0.5f);
	imguiWindow.DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent),
		center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.f);
	imguiWindow.DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent),
		center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.f);

	return pressed;
}

bool MyTreeNodeEx(const int ide)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

	ImGuiID id = window->GetID(ide);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2),
		g.FontSize + style.FramePadding.y * 2);
    
	ImRect frame_bb;
    frame_bb.Min.x = window->DC.CursorPos.x;
    frame_bb.Min.y = window->DC.CursorPos.y;
    frame_bb.Max.x = window->WorkRect.Max.x;
    frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
	frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
	frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);

    const float text_offset_x = g.FontSize + style.FramePadding.x * 3;
    const float text_offset_y = ImMax(style.FramePadding.y, window->DC.CurrLineTextBaseOffset);
    ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
	ImGui::ItemSize(ImVec2(g.FontSize, frame_height), style.FramePadding.y);

	ImRect interact_bb = frame_bb;
    bool is_open = ImGui::TreeNodeBehaviorIsOpen(id, ImGuiTreeNodeFlags_None);

    bool item_add = ImGui::ItemAdd(interact_bb, id);
    g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    g.LastItemData.DisplayRect = frame_bb;

    if (!item_add)
    {
        if (is_open)
			ImGui::TreePushOverrideID(id);
        return is_open;
    }

	ImGuiButtonFlags button_flags = ImGuiButtonFlags_AllowItemOverlap | ImGuiButtonFlags_PressedOnDragDropHold;

    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + 
		(g.FontSize + style.FramePadding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
    if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_NoKeyModifiers;

    if (is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    bool toggled = false;
	if (pressed && g.DragDropHoldJustPressedId != id)
	{
		toggled = true;
	}
	else if (pressed && g.DragDropHoldJustPressedId == id)
	{
		IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
		if (!is_open)
			toggled = true;
	}

	if (g.NavId == id && g.NavMoveDir == ImGuiDir_Left && is_open)
	{
		toggled = true;
		ImGui::NavMoveRequestCancel();
	}
	if (g.NavId == id && g.NavMoveDir == ImGuiDir_Right && !is_open)
	{
		toggled = true;
		ImGui::NavMoveRequestCancel();
	}

	if (toggled)
	{
		is_open = !is_open;
		window->DC.StateStorage->SetInt(id, is_open);
		g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
	}

    ImGui::SetItemAllowOverlap();

    const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;

	const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ?
		ImGuiCol_HeaderHovered : ImGuiCol_Header);
	if (IsItemVisible(*window))
	{
		ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
		ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + style.FramePadding.x, text_pos.y),
			text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
	}

    if (is_open)
		ImGui::TreePushOverrideID(id);

    return is_open;
}

bool MyCombo(const char* label, uint8_t* current_item, const char** data, const int items_count)
{
	ImGuiContext& g = *GImGui;

	const char* preview_value = nullptr;
	if (*current_item >= 0 && *current_item < items_count)
		preview_value = data[*current_item];

	if (!(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0),
			ImVec2(FLT_MAX, (g.FontSize + g.Style.ItemSpacing.y) * 10 - g.Style.ItemSpacing.y + 
				(g.Style.WindowPadding.y * 2)));

	if (!ImGui::BeginCombo(label, preview_value, ImGuiComboFlags_None))
		return false;

	bool value_changed = false;
	for (uint8_t i = 0; i < items_count; i++)
	{
		ImGui::PushID(i);
		const bool item_selected = (i == *current_item);
		if (ImGui::Selectable(data[i], item_selected))
		{
			value_changed = true;
			*current_item = i;
		}
		if (item_selected)
			ImGui::SetItemDefaultFocus();
		ImGui::PopID();
	}

	ImGui::EndCombo();

	if (value_changed)
		ImGui::MarkItemEdited(g.LastItemData.ID);

	return value_changed;
}

bool BinFieldDelete(const int id)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(bfdz, "BinFieldDelete", true);
#endif
	bool ret = false;
	bool retb = MyButtonEx(id);
#ifdef _DEBUG
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Delete item? %d", id);
#else
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("Delete item?");
#endif

	ImGuiWindow& imguiWindow = *ImGui::GetCurrentWindow();

	ImGuiID ide = imguiWindow.GetID(id);
	if (retb)
		if (!ImGui::IsPopupOpen(ide, ImGuiPopupFlags_None))
			ImGui::OpenPopupEx(ide);
	if (ImGui::BeginPopupEx(ide, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar))
	{
		ImGui::Text("Are you sure?");
		if (ImGui::Button("Yes"))
		{
			ImGui::CloseCurrentPopup();
			ret = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("No"))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}
	return ret;
}

void BinFieldAdd(const int id, uint8_t& current1, uint8_t& current2, uint8_t& current3, bool showfirst = false)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedN(bfaz, "BinFieldAdd", true);
#endif
	if (showfirst)
	{
		ImGui::PushID(id);
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", nullptr, true).x * 1.5f);
		ImGui::SameLine(); MyCombo("##current1", &current1, Type_strings, IM_ARRAYSIZE(Type_strings));
		ImGui::PopID();
#ifdef _DEBUG
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%d", id);
#endif
	}
	if (IsComplexBinType((BinType)current1) && !IsPointerOrEmbedded((BinType)current1))
	{
		ImGui::PushID(id + (showfirst ? 1 : 0));
		if (current1 == (uint8_t)BinType::MAP) {
			ImGui::SameLine(); ImGui::Text("Key:");
		}
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", nullptr, true).x * 1.5f);
		ImGui::SameLine(); MyCombo("##current2", &current2, Type_strings, IM_ARRAYSIZE(Type_strings));
		ImGui::PopID();
#ifdef _DEBUG
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%d", id + (showfirst ? 1 : 0));
#endif
	}
	if (current1 == (uint8_t)BinType::MAP)
	{
		ImGui::PushID(id + (showfirst ? 2 : 1));
		ImGui::SameLine(); ImGui::Text("Value:");
		ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", nullptr, true).x * 1.5f);
		ImGui::SameLine(); MyCombo("##current3", &current3, Type_strings, IM_ARRAYSIZE(Type_strings));
		ImGui::PopID();
#ifdef _DEBUG
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%d", id + (showfirst ? 2 : 1));
#endif
	}
}

void DrawRectRainBow(BinField *binValue, ImGuiWindow& imguiWindow, float depth)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(drrbz, "DrawRectRainBow", 50, true);
#endif
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			float degresshsv = 0.f;
			ContainerOrStructOrOption *cs = binValue->data->cso;
			if (cs->items.size() > 0)
				degresshsv = fmodf(depth, 360.f) / 360.f;
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				if (IsComplexBinType(cs->valueType))
				{
					if (cs->items[i].expanded)
					{
						if (cs->items[i].cursorMax.x != 0.f)
						{
							if (cs->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
								cs->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
							{
								cs->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
								cs->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
								cs->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
								ImGui::ItemAdd(ImRect(cs->items[i].cursorMin, cs->items[i].cursorMax), 0);
								ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
								cs->items[i].isOver = ImGui::IsItemHovered();
								if (cs->items[i].isOver)
									if (CanChangeBackcolor(cs->items[i].value))
										col.SetHSV(degresshsv, 1.f, .4f);
								imguiWindow.DrawList->AddRectFilled(cs->items[i].cursorMin, cs->items[i].cursorMax, col);
								imguiWindow.DrawList->AddRect(cs->items[i].cursorMin, cs->items[i].cursorMax,
									ImColor().HSV(degresshsv, 1.f, 1.f));
								cs->items[i].cursorMax.x = 0.f;
							}
						}
						DrawRectRainBow(cs->items[i].value, imguiWindow, depth + 10.f);
					}
				}
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			float degresshsv = 0.f;
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->items.size() > 0)
				degresshsv = fmodf(depth, 360.f) / 360.f;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					if (IsComplexBinType(pe->items[i].value->type))
					{
						if (pe->items[i].expanded)
						{
							if (pe->items[i].cursorMax.x != 0.f)
							{
								if (pe->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
									pe->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
								{
									pe->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
									pe->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
									pe->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
									ImGui::ItemAdd(ImRect(pe->items[i].cursorMin, pe->items[i].cursorMax), 0);
									ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
									pe->items[i].isOver = ImGui::IsItemHovered();
									if (pe->items[i].isOver)
										if (CanChangeBackcolor(pe->items[i].value))
											col.SetHSV(degresshsv, 1.f, .4f);
									imguiWindow.DrawList->AddRectFilled(pe->items[i].cursorMin, pe->items[i].cursorMax, col);
									imguiWindow.DrawList->AddRect(pe->items[i].cursorMin, pe->items[i].cursorMax,
										ImColor().HSV(degresshsv, 1.f, 1.f));
									pe->items[i].cursorMax.x = 0.f;
								}
							}
							DrawRectRainBow(pe->items[i].value, imguiWindow, depth + 10.f);
						}
					}
				}
			}
			break;
		}
		case BinType::MAP:
		{
			float degresshsv = 0.f;
			Map *map = binValue->data->map;
			if (map->items.size() > 0)
				degresshsv = fmodf(depth, 360.f) / 360.f;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				if (map->items[i].key)
				{
					if (map->items[i].expanded)
					{
						if (map->items[i].cursorMax.x != 0.f)
						{
							if (map->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
								map->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
							{
								map->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
								map->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
								map->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
								ImGui::ItemAdd(ImRect(map->items[i].cursorMin, map->items[i].cursorMax), 0);
								ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
								map->items[i].isOver = ImGui::IsItemHovered();
								if (map->items[i].isOver)
									if (CanChangeBackcolor(map->items[i].value))
										col.SetHSV(degresshsv, 1.f, .4f);
								imguiWindow.DrawList->AddRectFilled(map->items[i].cursorMin, map->items[i].cursorMax, col);
								imguiWindow.DrawList->AddRect(map->items[i].cursorMin, map->items[i].cursorMax,
									ImColor().HSV(degresshsv, 1.f, 1.f));
								map->items[i].cursorMax.x = 0.f;
							}
						}
						DrawRectRainBow(map->items[i].key, imguiWindow, depth + 10.f);
						DrawRectRainBow(map->items[i].value, imguiWindow, depth + 10.f);
					}
				}
			}
			break;
		}
	}
}

void DrawRectNormal(BinField *binValue, ImGuiWindow& imguiWindow, float depth)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(drnz, "DrawRectNormal", 50, true);
#endif
	if (depth > 0.15f)
		depth = 0.09f;
	switch (binValue->type)
	{
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
			{
				if (IsComplexBinType(cs->valueType))
				{
					if (cs->items[i].expanded)
					{
						if (cs->items[i].cursorMax.x != 0.f)
						{
							if (cs->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
								cs->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
							{
								cs->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
								cs->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
								cs->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
								ImGui::ItemAdd(ImRect(cs->items[i].cursorMin, cs->items[i].cursorMax), 0);
								ImColor col(depth, depth, depth);
								cs->items[i].isOver = ImGui::IsItemHovered();
								if (cs->items[i].isOver)
									if (CanChangeBackcolor(cs->items[i].value))
										col.Value.x = col.Value.y = col.Value.z = depth * 1.3f;
								imguiWindow.DrawList->AddRectFilled(cs->items[i].cursorMin, cs->items[i].cursorMax, col);
								imguiWindow.DrawList->AddRect(cs->items[i].cursorMin, cs->items[i].cursorMax, IM_COL32(128, 128, 128, 255));
								cs->items[i].cursorMax.x = 0.f;
							}
						}
						DrawRectNormal(cs->items[i].value, imguiWindow, depth + 0.01f);
					}
				}
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
			if (pe->name != 0)
			{
				for (uint16_t i = 0; i < pe->items.size(); i++)
				{
					if (IsComplexBinType(pe->items[i].value->type))
					{
						if (pe->items[i].expanded)
						{
							if (pe->items[i].cursorMax.x != 0.f)
							{
								if (pe->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
									pe->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
								{
									pe->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
									pe->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
									pe->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
									ImGui::ItemAdd(ImRect(pe->items[i].cursorMin, pe->items[i].cursorMax), 0);
									ImColor col(depth, depth, depth);
									pe->items[i].isOver = ImGui::IsItemHovered();
									if (pe->items[i].isOver)
										if (CanChangeBackcolor(pe->items[i].value))
											col.Value.x = col.Value.y = col.Value.z = depth * 1.3f;
									imguiWindow.DrawList->AddRectFilled(pe->items[i].cursorMin, pe->items[i].cursorMax, col);
									imguiWindow.DrawList->AddRect(pe->items[i].cursorMin, pe->items[i].cursorMax, IM_COL32(128, 128, 128, 255));
									pe->items[i].cursorMax.x = 0.f;
								}
							}
							DrawRectNormal(pe->items[i].value, imguiWindow, depth + 0.01f);
						}
					}
				}
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				if (map->items[i].key)
				{
					if (map->items[i].expanded)
					{
						if (map->items[i].cursorMax.x != 0.f)
						{
							if (map->items[i].cursorMin.y < (imguiWindow.ClipRect.Max.y + 50.f) &&
								map->items[i].cursorMax.y > (imguiWindow.ClipRect.Min.y + 50.f))
							{
								map->items[i].cursorMin.x -= IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f - 1.0f);
								map->items[i].cursorMax.x += IM_FLOOR(imguiWindow.WindowPadding.x * 0.5f);
								map->items[i].cursorMax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
								ImGui::ItemAdd(ImRect(map->items[i].cursorMin, map->items[i].cursorMax), 0);
								ImColor col(depth, depth, depth);
								map->items[i].isOver = ImGui::IsItemHovered();
								if (map->items[i].isOver)
									if (CanChangeBackcolor(map->items[i].value))
										col.Value.x = col.Value.y = col.Value.z = depth * 1.3f;
								imguiWindow.DrawList->AddRectFilled(map->items[i].cursorMin, map->items[i].cursorMax, col);
								imguiWindow.DrawList->AddRect(map->items[i].cursorMin, map->items[i].cursorMax, IM_COL32(128, 128, 128, 255));
								map->items[i].cursorMax.x = 0.f;
							}
						}
						DrawRectNormal(map->items[i].key, imguiWindow, depth + 0.01f);
						DrawRectNormal(map->items[i].value, imguiWindow, depth + 0.01f);
					}
				}
			}
			break;
		}
	}
}

void DrawBinField(BinField *binValue, HashTable& hashT, TernaryTree& ternaryT,
	int& treeId, ImGuiWindow& imguiWindow, bool openTree, bool *previousNode = nullptr, ImVec2 *cursor = nullptr)
{
#ifdef TRACY_ENABLE_ZONES
	ZoneNamedNS(gvftz, "DrawBinField", 50, true);
	gvftz.Text(Type_strings[binValue->type], strlen(Type_strings[binValue->type]));
#endif

	switch (binValue->type)
	{
		case BinType::NONE:
			ImGui::Text("NULL");
			break;
		case BinType::SInt8:
		case BinType::UInt8:
		case BinType::SInt16:
		case BinType::UInt16:
		case BinType::SInt32:
		case BinType::UInt32:
		case BinType::SInt64:
		case BinType::UInt64:
		{
			char buf[64] = { '\0' };
			const char* fmt = Type_fmt[(uint8_t)binValue->type];
			myassert(sprintf_s(buf, 64, fmt, binValue->data->ui64) <= 0)
			char* string = InputText(buf, binValue->id);
			if (string)
			{
				myassert(sscanf_s(string, fmt, &binValue->data->ui64) <= 0)
				delete[] string;
			}
			break;
		}
		case BinType::BOOLB:
		case BinType::FLAG:
		{
			char* string = InputText(binValue->data->b ? truestr : falsestr, binValue->id);
			if (string)
			{
				for (int i = 0; string[i]; i++)
					string[i] = tolower(string[i]);
				if (strcmp(string, truestr) == 0)
					binValue->data->b = true;
				else
					binValue->data->b = false;
				delete[] string;
			}
			break;
		}
		case BinType::Float32:
		case BinType::VEC2:
		case BinType::VEC3:
		case BinType::VEC4:
		case BinType::MTX44:
			FloatArray(binValue->data->floatv, Type_size[(uint8_t)binValue->type] / 4, binValue->id);
			break;
		case BinType::RGBA:
		{
			float colors[4] = { 0 };
			uint8_t* arr = binValue->data->rgba;
			for (int i = 0; i < 4; i++)
				colors[i] = (float)(arr[i] / 255.f);
			ImGui::PushID(binValue->id);
			if (ImGui::ColorEdit4("", colors))
			{
				for (int i = 0; i < 4; i++)
					arr[i] = (uint8_t)(colors[i] * 255.f);
			}
			ImGui::PopID();
			break;
		}
		case BinType::HASH:
		case BinType::LINK:
		{
			InputTextHash(binValue->data->ui32, binValue->id, hashT, ternaryT);
			break;
		}
		case BinType::WADENTRYLINK:
		{
			InputTextHashxx(binValue->data->ui64, binValue->id, hashT, ternaryT);
			break;
		}
		case BinType::STRING:
		{
			char* string = InputTextWithSusjestion(binValue->id, binValue->data->string, ternaryT);
			if (string != nullptr)
				binValue->data->string = string;
			break;
		}
		case BinType::OPTION:
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = binValue->data->cso;
	#ifdef TRACY_ENABLE_ZONES
			ZoneNamedN(cspz, "ContainerOrStructOrOption", true);
			cspz.Text(Type_strings[cs->valueType], strlen(Type_strings[cs->valueType]));
	#endif
			if (IsComplexBinType(cs->valueType) && !IsPointerOrEmbedded(cs->valueType))
			{
				for (uint32_t i = 0; i < cs->items.size(); i++)
				{
					if (openTree)
						ImGui::SetNextItemOpen(true);
					cs->items[i].cursorMin = imguiWindow.DC.CursorPos;
					cs->items[i].expanded = MyTreeNodeEx(cs->items[i].id);
					cs->items[i].idim = imguiWindow.IDStack.back();

					if (IsItemVisible(imguiWindow))
					{
	#ifdef _DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%d", cs->items[i].id);
	#endif
						ImGui::SameLine(); ImGui::Text("%s", Type_strings[(uint8_t)cs->valueType]);
						ImGui::SameLine(0, 1); GetArrayTypeFromBinField(cs->items[i].value);
						ImGui::SameLine(); GetArrayCountFromBinField(cs->items[i].value);
						ImGui::SameLine();
						if (BinFieldDelete(cs->items[i].id + 1))
						{
							ClearBinField(cs->items[i].value);

							bool open = cs->items[i].expanded;
							cs->items.erase(cs->items.begin() + i);
							if (open)
								ImGui::TreePop();
							continue;
						}
						else if (cs->items[i].expanded)
						{
							ImGui::Indent();
							DrawBinField(cs->items[i].value, hashT, ternaryT,
								treeId, imguiWindow, openTree);
							ImGui::Unindent();
							ImGui::TreePop();
						}
					}
					else if (cs->items[i].expanded) 
					{
						ImGui::Indent();
						DrawBinField(cs->items[i].value, hashT, ternaryT,
							treeId, imguiWindow, openTree);
						ImGui::Unindent();
						ImGui::TreePop();
					}
					if (cs->items[i].expanded)
						cs->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x, imguiWindow.DC.CursorMaxPos.y);
					else
						cs->items[i].isOver = false;
				}
			}
			else if (IsPointerOrEmbedded(cs->valueType))
			{
				for (uint32_t i = 0; i < cs->items.size(); i++)
				{
					if (openTree)
						ImGui::SetNextItemOpen(true);
					cs->items[i].cursorMin = imguiWindow.DC.CursorPos;
					cs->items[i].expanded = MyTreeNodeEx(cs->items[i].id);
					cs->items[i].idim = imguiWindow.IDStack.back();

					if (IsItemVisible(imguiWindow))
					{
						ImVec2 cursor, cursorOld;
	#ifdef _DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%d", cs->items[i].id);
	#endif
						ImGui::SameLine();
						DrawBinField(cs->items[i].value, hashT, ternaryT, treeId, imguiWindow,
							openTree, &cs->items[i].expanded, &cursor);

						cursorOld = ImGui::GetCursorPos();
						ImGui::SetCursorPos(cursor);
						if (BinFieldDelete(cs->items[i].id + 1))
						{
							ClearBinField(cs->items[i].value);

							bool open = cs->items[i].expanded;
							cs->items.erase(cs->items.begin() + i);
							if (open)
								ImGui::TreePop();
							continue;
						}
						ImGui::SetCursorPos(cursorOld);
					}
					else if (cs->items[i].expanded)
					{
						ImGui::SameLine();
						DrawBinField(cs->items[i].value, hashT, ternaryT, treeId, imguiWindow,
							openTree, &cs->items[i].expanded);
					}

					if (cs->items[i].expanded)
					{
						ImGui::TreePop();
						if (cs->items[i].value->data->pe->name == 0)
						{
							cs->items[i].cursorMax.x = 0.f;
							continue;
						}
						cs->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x, imguiWindow.DC.CursorMaxPos.y);
					}
					else
						cs->items[i].isOver = false;
				}
			}
			else {
				for (uint32_t i = 0; i < cs->items.size(); i++)
				{
					if (IsItemVisible(imguiWindow))
					{
						DrawBinField(cs->items[i].value, hashT, ternaryT,
							treeId, imguiWindow, openTree);
						ImGui::SameLine();
						if (BinFieldDelete(cs->items[i].id))
						{
							ClearBinField(cs->items[i].value);
							cs->items.erase(cs->items.begin() + i);
						}
					}
					else if (cs->valueType == BinType::MTX44)
					{
						for (int o = 0; o < 4; o++)
							MyNewLine(imguiWindow);
					}
					else {
						MyNewLine(imguiWindow);
					}
				}
			}

			if (IsItemVisible(imguiWindow))
			{
				uint8_t typi = (uint8_t)cs->valueType;

				bool add = ImGui::Button("Add new item");
				BinFieldAdd(binValue->id, typi, cs->current2, cs->current3);
				if (add)
				{
					CSOField csItem;
					csItem.value = BinFieldNewClean(cs->valueType, (BinType)cs->current2, (BinType)cs->current3);
					csItem.value->parent = binValue;

					cs->items.emplace_back(csItem);
					ContructIdForBinField(binValue, treeId);
				}
			}
			else {
				MyNewLine(imguiWindow);
			}
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = binValue->data->pe;
	#ifdef TRACY_ENABLE_ZONES
			ZoneNamedN(pez, "PointerOrEmbed", true);
	#endif
			if (pe->name)
			{
				bool peexpanded = false;
				if (previousNode == nullptr)
				{
					if (openTree)
						ImGui::SetNextItemOpen(true);
					peexpanded = MyTreeNodeEx(binValue->id);
					pe->idim = imguiWindow.IDStack.back();

					if (IsItemVisible(imguiWindow))
					{
	#ifdef _DEBUG
						if (ImGui::IsItemHovered())
							ImGui::SetTooltip("%d", binValue->id);
	#endif
						ImGui::SameLine(); ImGui::Text("%s =", Type_strings[(uint8_t)binValue->type]);
						ImGui::SameLine(); InputTextHash(pe->name, binValue->id + 1, hashT, ternaryT);
						ImGui::SameLine(); GetArrayCountFromBinField(binValue);
					}
					else {
						MyNewLine(imguiWindow);
					}
				}
				else {
					peexpanded = *previousNode;

					if (IsItemVisible(imguiWindow))
					{
						ImGui::SameLine(); ImGui::Text("%s =", Type_strings[(uint8_t)binValue->type]);
						ImGui::SameLine(); InputTextHash(pe->name, binValue->id, hashT, ternaryT);
						ImGui::SameLine(); GetArrayCountFromBinField(binValue);
					}
					else {
						MyNewLine(imguiWindow);
					}
				}
				if (cursor)
				{
					ImVec2 old = ImGui::GetCursorPos();
					ImGui::SameLine(); *cursor = ImGui::GetCursorPos();
					ImGui::SetCursorPos(old);
				}
				if (peexpanded)
				{
					ImGui::Indent();

					for (uint16_t i = 0; i < pe->items.size(); i++)
					{
	#ifdef TRACY_ENABLE_ZONES
						pez.Text(Type_strings[typi], strlen(Type_strings[typi]));
	#endif
						if (IsComplexBinType(pe->items[i].value->type))
						{
							if (openTree)
								ImGui::SetNextItemOpen(true);
							pe->items[i].cursorMin = imguiWindow.DC.CursorPos;
							pe->items[i].expanded = MyTreeNodeEx(pe->items[i].id);
							pe->items[i].idim = imguiWindow.IDStack.back();

							if (IsItemVisible(imguiWindow))
							{
	#ifdef _DEBUG
								if (ImGui::IsItemHovered())
									ImGui::SetTooltip("%d", pe->items[i].id);
	#endif
								ImGui::SameLine(); InputTextHash(pe->items[i].key, pe->items[i].id + 1, hashT, ternaryT);
								if (!IsPointerOrEmbedded(pe->items[i].value->type))
								{
									ImGui::SameLine(); ImGui::Text(": %s", Type_strings[(uint8_t)pe->items[i].value->type]);
									ImGui::SameLine(0, 1); GetArrayTypeFromBinField(pe->items[i].value);
									ImGui::SameLine(); GetArrayCountFromBinField(pe->items[i].value);
									ImGui::SameLine();
									if (BinFieldDelete(pe->items[i].id + 2))
									{
										ClearBinField(pe->items[i].value);

										bool open = pe->items[i].expanded;
										pe->items.erase(pe->items.begin() + i);
										if (open)
											ImGui::TreePop();
										continue;
									}
									else if (pe->items[i].expanded)
									{
										ImGui::Indent();
										DrawBinField(pe->items[i].value, hashT, ternaryT,
											treeId, imguiWindow, openTree);
										ImGui::Unindent();
										ImGui::TreePop();
									}
								}
								else {
									ImVec2 cursor, cursorold;
									ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
									DrawBinField(pe->items[i].value, hashT, ternaryT, treeId, imguiWindow,
										openTree, &pe->items[i].expanded, &cursor);

									cursorold = ImGui::GetCursorPos();
									ImGui::SetCursorPos(cursor);
									if (BinFieldDelete(pe->items[i].id + 2))
									{
										ClearBinField(pe->items[i].value);

										bool open = pe->items[i].expanded;
										pe->items.erase(pe->items.begin() + i);
										if (open)
											ImGui::TreePop();
										continue;
									}
									ImGui::SetCursorPos(cursorold);
									if (pe->items[i].expanded)
										ImGui::TreePop();
								}
							}
							else if (!IsPointerOrEmbedded(pe->items[i].value->type))
							{
								if (pe->items[i].expanded)
								{
									ImGui::Indent();
									DrawBinField(pe->items[i].value, hashT, ternaryT,
										treeId, imguiWindow, openTree);
									ImGui::Unindent();
									ImGui::TreePop();
								}
							}
							else {
								ImGui::SameLine();
								DrawBinField(pe->items[i].value, hashT, ternaryT, treeId, imguiWindow,
									openTree, &pe->items[i].expanded);
								if (pe->items[i].expanded)
									ImGui::TreePop();
							}
							if (pe->items[i].expanded)
							{
								if (IsPointerOrEmbedded(pe->items[i].value->type))
								{
									if (pe->items[i].value->data->pe->name == 0)
									{
										pe->items[i].cursorMax.x = 0.f;
										continue;
									}
								}
								pe->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x, imguiWindow.DC.CursorMaxPos.y);
							}
							else
								pe->items[i].isOver = false;
						}
						else if (IsItemVisible(imguiWindow)) 
						{
							InputTextHash(pe->items[i].key, pe->items[i].id, hashT, ternaryT);
							ImGui::SameLine(); ImGui::Text(": %s =", Type_strings[(uint8_t)pe->items[i].value->type]); ImGui::SameLine();

							DrawBinField(pe->items[i].value, hashT, ternaryT,
								treeId, imguiWindow, openTree);
							ImGui::SameLine();
							if (BinFieldDelete(pe->items[i].id + 1))
							{
								ClearBinField(pe->items[i].value);
								pe->items.erase(pe->items.begin() + i);
							}
						}
						else if (pe->items[i].value->type == BinType::MTX44)
 {
							for (int o = 0; o < 4; o++)
								MyNewLine(imguiWindow);
						}
						else {
							MyNewLine(imguiWindow);
						}
					}

					if (IsItemVisible(imguiWindow))
					{
						bool add = ImGui::Button("Add new item");
						BinFieldAdd(binValue->id + 2, pe->current1, pe->current2, pe->current3, true);
						if (add)
						{
							EPField peItem;
							peItem.value = BinFieldNewClean((BinType)pe->current1, (BinType)pe->current2, (BinType)pe->current3);
							peItem.value->parent = binValue;

							pe->items.emplace_back(peItem);
							ContructIdForBinField(binValue, treeId);
						}
					}
					else {
						MyNewLine(imguiWindow);
					}

					ImGui::Unindent();

					if (previousNode == nullptr)
						ImGui::TreePop();
				}
			}
			else {
				if (IsItemVisible(imguiWindow))
				{
					ImGui::Text("%s =", Type_strings[(uint8_t)binValue->type]); ImGui::SameLine();
					InputTextHash(pe->name, binValue->id, hashT, ternaryT);
				}
				else
					MyNewLine(imguiWindow);
			}
			break;
		}
		case BinType::MAP:
		{
			Map *map = binValue->data->map;
	#ifdef TRACY_ENABLE_ZONES
			ZoneNamedN(mapz, "Map", true);
	#endif
			for (uint32_t i = 0; i < map->items.size(); i++)
			{
				if (openTree)
						ImGui::SetNextItemOpen(true);
				map->items[i].cursorMin = imguiWindow.DC.CursorPos;
				map->items[i].expanded = MyTreeNodeEx(map->items[i].id);
				map->items[i].idim = imguiWindow.IDStack.back();
				if (IsItemVisible(imguiWindow))
				{
#ifdef _DEBUG
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%d", map->items[i].id);
#endif
#ifdef TRACY_ENABLE_ZONES
					mapz.Text(Type_strings[map->keyType], strlen(Type_strings[map->keyType]));
					mapz.Text(Type_strings[map->valueType], strlen(Type_strings[map->valueType]));
#endif
					if (!IsPointerOrEmbedded(map->keyType)) {
						ImGui::SameLine(); ImGui::Text("%s =", Type_strings[(uint8_t)map->keyType]);
					}
					ImGui::SameLine();
					DrawBinField(map->items[i].key, hashT, ternaryT,
						treeId, imguiWindow, openTree);

					ImGui::SameLine();
					if (BinFieldDelete(map->items[i].id + 1))
					{
						ClearBinField(map->items[i].key);
						ClearBinField(map->items[i].value);

						bool open = map->items[i].expanded;
						map->items.erase(map->items.begin() + i);
						if (open)
							ImGui::TreePop();
						continue;
					}
				}
				else if (IsComplexBinType(map->keyType))
				{
					DrawBinField(map->items[i].key, hashT, ternaryT,
						treeId, imguiWindow, openTree);
				}
				else if (map->keyType == BinType::MTX44)
				{
					for (int o = 0; o < 3; o++)
						MyNewLine(imguiWindow);
				}
				if (map->items[i].expanded)
				{
					if (IsItemVisible(imguiWindow))
					{
						ImGui::Indent();
						if (!IsComplexBinType(map->valueType))
						{
							ImGui::Text("%s =", Type_strings[(uint8_t)map->valueType]);
							ImGui::SameLine();
						}
						DrawBinField(map->items[i].value, hashT, ternaryT,
							treeId, imguiWindow, openTree);
						ImGui::Unindent();
					}
					else if (IsComplexBinType(map->valueType))
					{
						ImGui::Indent();
						DrawBinField(map->items[i].value, hashT, ternaryT,
							treeId, imguiWindow, openTree);
						ImGui::Unindent();
					}
					else if (map->valueType == BinType::MTX44) 
					{
						for (int o = 0; o < 4; o++)
							MyNewLine(imguiWindow);
					}
					else {
						MyNewLine(imguiWindow);
					}
					ImGui::TreePop();
					if (IsPointerOrEmbedded(map->keyType))
					{
						if (map->items[i].value->data->pe->name == 0)
						{
							map->items[i].cursorMax.x = 0.f;
							continue;
						}
					}
					map->items[i].cursorMax = ImVec2(imguiWindow.WorkRect.Max.x, imguiWindow.DC.CursorMaxPos.y);
				}
				else
					map->items[i].isOver = false;
			}

			if (IsItemVisible(imguiWindow))
			{
				uint8_t typi = (uint8_t)map->keyType;
				uint8_t typf = (uint8_t)map->valueType;

				bool add = ImGui::Button("Add new item");
				BinFieldAdd(binValue->id, typi, map->current2, map->current3);			
				BinFieldAdd(binValue->id + 1, typf, map->current4, map->current5);
				if (add)
				{
					MapPair mapItem;
					mapItem.key = BinFieldNewClean(map->keyType, (BinType)map->current2, (BinType)map->current3);
					mapItem.value = BinFieldNewClean(map->valueType, (BinType)map->current4, (BinType)map->current5);

					mapItem.key->parent = binValue;
					mapItem.value->parent = binValue;

					map->items.emplace_back(mapItem);
					ContructIdForBinField(binValue, treeId);
				}
			}
			else {
				MyNewLine(imguiWindow);
			}
			break;
		}
	}
}