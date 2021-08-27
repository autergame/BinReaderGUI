//author https://github.com/autergame
#define IMGUI_DEFINE_MATH_OPERATORS
#define GLFW_EXPOSE_NATIVE_WIN32
#pragma comment(lib, "mimalloc-static")
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glfw3")
#ifdef TRACY_ENABLE
    #include <tracy/Tracy.hpp>
#endif
#include <mimalloc.h>
#include <Windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_internal.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>

//#ifdef _DEBUG
//    #define TRACY_ENABLE_ZONES
//#endif

void _assert(char const* msg, char const* file, unsigned line)
{
    fprintf(stderr, "ERROR: %s %s %d\n", msg, file, line);
    scanf_s("press enter to exit.");
    exit(1);
}
#define myassert(expression) if (expression) { _assert(#expression, __FILE__, __LINE__); }

#pragma region MemoryThings

void* callocb(size_t count, size_t size)
{
    void* p = mi_calloc(count, size);
    myassert(p == NULL);
#ifdef TRACY_ENABLE_ZONES
    TracyAlloc(p, count*size);
#endif
    return p;
}

void* reallocb(void* p, size_t size)
{
    void* pe = mi_realloc(p, size);
    myassert(pe == NULL);
    return pe;
}

void freeb(void* p)
{
#ifdef TRACY_ENABLE_ZONES
    TracyFree(p);
#endif
    mi_free(p);
}

static void* MallocbWrapper(size_t size, void* user_data) {
    IM_UNUSED(user_data); return callocb(1, size); 
}
static void  FreebWrapper(void* ptr, void* user_data) {
    IM_UNUSED(user_data); freeb(ptr); 
}

#pragma endregion

#pragma region FNV1Hash XXHash HashTable 

uint32_t FNV1Hash(char* str)
{
    uint32_t Hash = 0x811c9dc5;
    for (size_t i = 0; i < strlen(str); i++)
        Hash = (Hash ^ tolower(str[i])) * 0x01000193;
    return Hash;
}

uint64_t PRIME1 = 0x9E3779B185EBCA87ULL;
uint64_t PRIME2 = 0xC2B2AE3D27D4EB4FULL;
uint64_t PRIME3 = 0x165667B19E3779F9ULL;
uint64_t PRIME4 = 0x85EBCA77C2B2AE63ULL;
uint64_t PRIME5 = 0x27D4EB2F165667C5ULL;
uint64_t xxread8(const void* memPtr)
{
    uint8_t val;
    myassert(memcpy(&val, memPtr, 1) == NULL);
    return val;
}
uint64_t xxread32(const void* memPtr)
{
    uint32_t val;
    myassert(memcpy(&val, memPtr, 4) == NULL);
    return val;
}
uint64_t xxread64(const void* memPtr)
{
    uint64_t val;
    myassert(memcpy(&val, memPtr, 8) == NULL);
    return val;
}
uint64_t XXH_rotl64(uint64_t x, int r)
{
    return ((x << r) | (x >> (64 - r)));
}
uint64_t XXHash(const uint8_t* input, size_t len)
{
    uint64_t h64;
    const uint8_t* bEnd = input + len;

    if (len >= 32) {
        const uint8_t* const limit = bEnd - 32;
        uint64_t v1 = PRIME1 + PRIME2;
        uint64_t v2 = PRIME2;
        uint64_t v3 = 0;
        uint64_t v4 = 0 - PRIME1;

        do
        {
            v1 += xxread64(input) * PRIME2;
            v1 = XXH_rotl64(v1, 31);
            v1 *= PRIME1;
            input += 8;
            v2 += xxread64(input) * PRIME2;
            v2 = XXH_rotl64(v2, 31);
            v2 *= PRIME1;
            input += 8;
            v3 += xxread64(input) * PRIME2;
            v3 = XXH_rotl64(v3, 31);
            v3 *= PRIME1;
            input += 8;
            v4 += xxread64(input) * PRIME2;
            v4 = XXH_rotl64(v4, 31);
            v4 *= PRIME1;
            input += 8;
        } while (input <= limit);

        h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) + XXH_rotl64(v4, 18);

        v1 *= PRIME2;
        v1 = XXH_rotl64(v1, 31);
        v1 *= PRIME1;
        h64 ^= v1;
        h64 = h64 * PRIME1 + PRIME4;

        v2 *= PRIME2;
        v2 = XXH_rotl64(v2, 31);
        v2 *= PRIME1;
        h64 ^= v2;
        h64 = h64 * PRIME1 + PRIME4;

        v3 *= PRIME2;
        v3 = XXH_rotl64(v3, 31);
        v3 *= PRIME1;
        h64 ^= v3;
        h64 = h64 * PRIME1 + PRIME4;

        v4 *= PRIME2;
        v4 = XXH_rotl64(v4, 31);
        v4 *= PRIME1;
        h64 ^= v4;
        h64 = h64 * PRIME1 + PRIME4;
    }
    else {
        h64 = PRIME5;
    }

    h64 += (uint64_t)len;

    while (input + 8 <= bEnd)
    {
        uint64_t k1 = xxread64(input);
        k1 *= PRIME2;
        k1 = XXH_rotl64(k1, 31);
        k1 *= PRIME1;
        h64 ^= k1;
        h64 = XXH_rotl64(h64, 27) * PRIME1 + PRIME4;
        input += 8;
    }

    if (input + 4 <= bEnd)
    {
        h64 ^= (uint64_t)(xxread32(input)) * PRIME1;
        h64 = XXH_rotl64(h64, 23) * PRIME2 + PRIME3;
        input += 4;
    }

    while (input < bEnd)
    {
        h64 ^= xxread8(input) * PRIME5;
        h64 = XXH_rotl64(h64, 11) * PRIME1;
        input += 1;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME2;
    h64 ^= h64 >> 29;
    h64 *= PRIME3;
    h64 ^= h64 >> 32;
    return h64;
}

struct HashTableNode
{
    uint64_t key;
    char* value;
    HashTableNode* next;
};

typedef struct HashTable
{
    size_t size;
    HashTableNode** list;
} HashTable;

HashTable* CreateHashTable(size_t size)
{
    HashTable* hasht = (HashTable*)callocb(1, sizeof(HashTable));
    hasht->list = (HashTableNode**)callocb(size, sizeof(HashTableNode*));
    hasht->size = size;
    return hasht;
}

void InsertHashTable(HashTable* hasht, uint64_t key, char* val)
{
    uint64_t pos = key % hasht->size;
    HashTableNode* list = hasht->list[pos];
    HashTableNode* temp = list;
    while (temp) 
    {
        if (temp->key == key) 
        {
            temp->value = val;
            return;
        }
        temp = temp->next;
    }
    HashTableNode* newNode = (HashTableNode*)callocb(1, sizeof(HashTableNode));
    newNode->key = key;
    newNode->value = val;
    newNode->next = list;
    hasht->list[pos] = newNode;
}

char* LookupHashTable(HashTable* hasht, uint64_t key)
{
    HashTableNode* list = hasht->list[key % hasht->size];
    HashTableNode* temp = list;
    while (temp) 
    {
        if (temp->key == key) 
        {
            return temp->value;
        }
        temp = temp->next;
    }
    return NULL;
}

int AddToHashTable(HashTable* hasht, const char* filename, bool xxhash = false)
{
    FILE* file;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err)
    {
        char* errmsg = (char*)callocb(255, 1);
        strerror_s(errmsg, 255, err);
        printf("ERROR: cannot read file %s %s.\n", filename, errmsg);
        free(errmsg);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* fp = (char*)callocb(1, fsize + 1);
    myassert(fread(fp, fsize, 1, file) == NULL);
    fp[fsize] = 0;
    fclose(file);

    uint32_t lines = 0;
    char* next_token = NULL;
    char* hashend, *hashline = strtok_s(fp, "\n", &next_token);
    if (xxhash == false)
    {
        while (hashline != NULL)
        {
            lines += 1;
            uint64_t key = strtoul(hashline, &hashend, 16);
            if (LookupHashTable(hasht, key) == NULL)
                InsertHashTable(hasht, key, hashend + 1);
            hashline = strtok_s(next_token, "\n", &next_token);
        }
    }
    else
    {
        while (hashline != NULL)
        {
            lines += 1;
            uint64_t key = strtoull(hashline, &hashend, 16);
            if (LookupHashTable(hasht, key) == NULL)
                InsertHashTable(hasht, key, hashend + 1);
            hashline = strtok_s(next_token, "\n", &next_token);
        }
    }

    printf("file: %s loaded: %d hashes lines.\n", filename, lines);
    return 0;
}

#pragma endregion

#pragma region charv memfwrite memfread

typedef struct charv
{
    char* data;
    size_t lenght;
} charv;

void memfwrite(void* buf, size_t bytes, charv* membuf)
{
    char* oblock = (char*)reallocb(membuf->data, membuf->lenght + bytes);
    oblock += membuf->lenght;
    myassert(memcpy(oblock, buf, bytes) == NULL);
    membuf->data = oblock;
    membuf->data -= membuf->lenght;
    membuf->lenght += bytes;
}

void memfread(void* buf, size_t bytes, char** membuf)
{
    myassert(memcpy(buf, *membuf, bytes) == NULL);
    *membuf += bytes;
}

#pragma endregion

#pragma region BinStructures

typedef enum Type
{
    NONE, BOOLB, SInt8,
    UInt8, SInt16, UInt16,
    SInt32, UInt32, SInt64,
    UInt64, Float32, VEC2,
    VEC3, VEC4, MTX44,
    RGBA, STRING, HASH,
    WADENTRYLINK, CONTAINER, STRUCT,
    POINTER, EMBEDDED, LINK,
    OPTION, MAP, FLAG
} Type;

typedef struct BinField
{
    Type type;
    void* data;
    uintptr_t id;
} BinField;

typedef struct Pair
{
    ImGuiID idim;
    uintptr_t id;
    BinField* key;
    BinField* value;
    ImVec2 cursormin;
    ImVec2 cursormax;
    bool isover; bool expanded;
} Pair;

typedef struct Field
{
    ImGuiID idim;
    uintptr_t id;
    uint32_t key;
    BinField* value;
    ImVec2 cursormin;
    ImVec2 cursormax;
    bool isover; bool expanded;
} Field;

typedef struct IdField
{
    ImGuiID idim;
    uintptr_t id;
    BinField* value;
    ImVec2 cursormin;
    ImVec2 cursormax;
    bool isover; bool expanded;
} IdField;

typedef struct ContainerOrStructOrOption
{
    int current2;
    int current3;
    Type valueType;
    IdField** items;
    uint32_t itemsize;
} ContainerOrStructOrOption;

typedef struct PointerOrEmbed
{
    int current1;
    int current2;
    int current3;
    ImGuiID idim;
    uint32_t name;
    Field** items;
    uint16_t itemsize;
} PointerOrEmbed;

typedef struct Map
{
    int current2;
    int current3;
    int current4;
    int current5;
    Type keyType;
    Type valueType;
    Pair** items;
    uint32_t itemsize;
} Map;

#pragma endregion

#pragma region BinTypes Uint8ToType TypeToUint8

static const char* Type_strings[] = {
    "None", "Bool", "SInt8", "UInt8", "SInt16", "UInt16", "SInt32", "UInt32", "SInt64",
    "UInt64", "Float32", "Vector2", "Vector3", "Vector4", "Matrix4x4", "RGBA", "String", "Hash",
    "WadEntryLink", "Container", "Struct", "Pointer", "Embedded", "Link", "Option", "Map", "Flag"
};

static const int Type_size[] = {
    0, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 12, 16, 64, 4, 0, 4, 8, 0, 0, 0, 0, 4, 0, 0, 1
};

static const char* Type_fmt[] = {
    NULL, NULL, "%" PRIi8, "%" PRIu8, "%" PRIi16, "%" PRIu16, "%" PRIi32, "%" PRIu32, "%" PRIi64, "%" PRIu64
};

static const int Type_sizeclean[] = {
    1, 1, 1, 1, 2, 2, 4, 4, 8, 8, 4, 8, 12, 16, 64, 4, 1, 4, 8,
    sizeof(ContainerOrStructOrOption), sizeof(ContainerOrStructOrOption),
    sizeof(PointerOrEmbed), sizeof(PointerOrEmbed), 4, sizeof(ContainerOrStructOrOption), sizeof(Map), 1
};

Type Uint8ToType(uint8_t type)
{
    if (type & 0x80)
        type = (type - 0x80) + CONTAINER;
    return (Type)type;
}

uint8_t TypeToUint8(Type type)
{
    uint8_t raw = type;
    if (raw >= CONTAINER)
        raw = (raw - CONTAINER) + 0x80;
    return raw;
}

#pragma endregion

#pragma region BinTextsHandlers

char* HashToString(HashTable* hasht, uint32_t value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(htsz, "HashToString", true);
    #endif
    char* strvalue = LookupHashTable(hasht, value);
    if (strvalue == NULL)
    {
        strvalue = (char*)callocb(16, 1);
        myassert(sprintf_s(strvalue, 16, "0x%08" PRIX32, value) <= 0);
    }
    return strvalue;
}

char* HashToStringxx(HashTable* hasht, uint64_t value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(htsxz, "HashToStringxx", true);
    #endif
    char* strvalue = LookupHashTable(hasht, value);
    if (strvalue == NULL)
    {
        strvalue = (char*)callocb(32, 1);
        myassert(sprintf_s(strvalue, 32, "0x%016" PRIX64, value) <= 0);
    }
    return strvalue;
}

static int MyResizeCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        char** string = (char**)data->UserData;
        char* newdata = (char*)callocb(1, data->BufSize);
        myassert(memcpy(newdata, *string, strlen(*string)) == NULL);
        freeb(*string); *string = newdata;
        data->Buf = *string;
    }
    return 0;
}

char* InputText(const char* inner, uintptr_t id, float sized = 0.f)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(itz, "InputText", true);
    #endif
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImGui::PushID((void*)id);
    size_t size = strlen(inner) + 1;
    char* string = (char*)callocb(size, 1);
    myassert(memcpy(string, inner, size) == NULL);
    char** userdata = (char**)callocb(1, sizeof(char*)); *userdata = string;
    if (sized == 0)
        sized = ImGui::CalcTextSize(inner).x;
    ImGui::PushItemWidth(ImMin(sized + GImGui->FontSize * 2.f,
        window->ContentRegionRect.Max.x - window->DC.CursorPos.x - GImGui->FontSize * 4.f));
    bool ret = ImGui::InputText("", string, size, ImGuiInputTextFlags_CallbackResize, MyResizeCallback, (void*)userdata);
    ImGui::PopID();
#ifdef _DEBUG
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("%d", id);
#endif
    if(ret)
        if (ImGui::IsKeyPressedMap(ImGuiKey_Enter) || GImGui->IO.MouseClicked[0])
            return *userdata;
    freeb(*userdata);
    freeb(userdata);
    return NULL;
}

void InputTextHash(HashTable* hasht, uint32_t* hash, uintptr_t id)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(itmz, "InputTextHash", true);
    #endif
    char* string = InputText(HashToString(hasht, *hash), id);
    if (string != NULL)
    {
        if (string[0] == '0' && string[1] == 'x')
            *hash = strtoul(string, NULL, 16);
        else
        {
            *hash = FNV1Hash(string);
            if (LookupHashTable(hasht, *hash) == NULL)
                InsertHashTable(hasht, *hash, string);
        }
    }
}

void InputTextHashxx(HashTable* hasht, uint64_t* hash, uintptr_t id)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(itmxz, "InputTextHashxx", true);
    #endif
    char* string = InputText(HashToStringxx(hasht, *hash), id);
    if (string != NULL)
    {
        if (string[0] == '0' && string[1] == 'x')
            *hash = strtoull(string, NULL, 16);
        else
        {
            *hash = XXHash((const uint8_t*)string, strlen(string));
            if (LookupHashTable(hasht, *hash) == NULL)
                InsertHashTable(hasht, *hash, string);
        }
    }
}

void FloatArray(void* data, int size, uintptr_t id)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(faz, "FloatArray", true);
    #endif
    int lengthm = 0;
    int arrindex = 0;
    float* arr = (float*)data;
    char** buf = (char**)callocb(size, sizeof(char*));
    for (int i = 0; i < size; i++)
    {
        buf[i] = (char*)callocb(64, 1);
        int length = sprintf_s(buf[i], 64, "%g", arr[i]);
        myassert(length <= 0);
        if (length > lengthm)
        {
            lengthm = length;
            arrindex = i;
        }
    }
    float indent = ImGui::GetCurrentWindow()->DC.CursorPos.x;
    float stringd = ImGui::CalcTextSize(buf[arrindex]).x;
    for (int i = 0; i < size; i++)
    {
        char* string = InputText(buf[i], id + i, stringd);
        if (string != NULL)
            myassert(sscanf_s(string, "%g", &arr[i]) <= 0);
        if (size == 16)
        {
            if ((i+1) % 4 == 0 && i != 15)
            {
                ImGui::ItemSize(ImVec2(0.f, GImGui->Style.FramePadding.y));
                ImGui::SameLine(indent);
            }
            else if (i < size - 1)
                ImGui::SameLine();
        }
        else if (i < size - 1)
            ImGui::SameLine();
        freeb(string);
    }
    for (int i  = 0; i < size; i++)
        freeb(buf[i]);
    freeb(buf);
}

void GetArrayCount(BinField* value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(gacz, "GetArrayCount", true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case CONTAINER:
        case STRUCT:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            if (cs->itemsize > 1)
                ImGui::Text("%d Items", cs->itemsize);
            else 
                ImGui::Text("%d Item", cs->itemsize);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->itemsize > 1)
                ImGui::Text("%d Items", pe->itemsize);
            else
                ImGui::Text("%d Item", pe->itemsize);
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            if (mp->itemsize > 1)
                ImGui::Text("%d Items", mp->itemsize);
            else
                ImGui::Text("%d Item", mp->itemsize);
            break;
        }
    }
}

void GetArrayType(BinField* value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(gatz, "GetArrayType", true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case CONTAINER:
        case STRUCT:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            ImGui::Text("[%s]", Type_strings[cs->valueType]);
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            ImGui::Text("[%s,%s]", Type_strings[mp->keyType], Type_strings[mp->valueType]);
            break;
        }
    }
}

#pragma endregion

void ClearBin(BinField* value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(cbz, "ClearBin", 50, true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
                if (cs->items[i]->value != NULL)
                    ClearBin(cs->items[i]->value);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                    if (pe->items[i]->value != NULL)
                        ClearBin(pe->items[i]->value);
            }
            break;
        }
        case MAP:
        {
            Map* map = (Map*)value->data;
            for (uint32_t i = 0; i < map->itemsize; i++)
            {
                if (map->items[i]->key != NULL)
                {
                    ClearBin(map->items[i]->key);
                    ClearBin(map->items[i]->value);
                }
            }
            break;
        }
    }
    freeb(value->data);
    freeb(value);
}

void GetStructIdBin(BinField* value, uintptr_t* tree)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(gsibz, "GetStructIdBin", 50, true);
    #endif
    switch (value->type)
    {
        case FLAG:
        case HASH:
        case LINK:
        case BOOLB:
        case SInt8:
        case UInt8:
        case SInt16:
        case UInt16:
        case SInt32:
        case UInt32:
        case SInt64:
        case UInt64:
        case STRING:
        case Float32:
        case WADENTRYLINK:
        {
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 1;
            }
            break;
        }
        case VEC2:
        {
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 2;
            }
            break;
        }
        case VEC3:
        {
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 3;
            }
            break;
        }
        case VEC4:
        case RGBA:
        {
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 4;
            }
            break;
        }
        case MTX44:
        {
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 16;
            }
            break;
        }
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 1;
            }
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->id == 0)
                {
                    cs->items[i]->id = *tree;
                    *tree += 2;
                }
                if (cs->items[i]->value != NULL)
                    GetStructIdBin(cs->items[i]->value, tree);
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 5;
            }
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->id == 0)
                    {
                        pe->items[i]->id = *tree;
                        *tree += 3;
                    }
                    if (pe->items[i]->value != NULL)
                        GetStructIdBin(pe->items[i]->value, tree);
                }
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            if (value->id == 0)
            {
                value->id = *tree;
                *tree += 2;
            }
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->id == 0)
                {
                    mp->items[i]->id = *tree;
                    *tree += 2;
                }
                if (mp->items[i]->key != NULL)
                    GetStructIdBin(mp->items[i]->key, tree);
                if (mp->items[i]->value != NULL)
                    GetStructIdBin(mp->items[i]->value, tree);
            }
            break;
        }
    }
}

void GetTreeSize(BinField* value, uint32_t* size)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(gtsz, "GetTreeSize", 50, true);
    #endif
    *size += 1;
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->value != NULL)
                {
                    GetTreeSize(cs->items[i]->value, size);
                }
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        GetTreeSize(pe->items[i]->value, size);
                    }
                }
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {                      
                    GetTreeSize(mp->items[i]->key, size);
                    GetTreeSize(mp->items[i]->value, size);
                }
            }
            break;
        }
    }
}

bool CanChangeBackcolor(BinField* value)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(ccbcz, "CanChangeBackcolor", 50, true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->value != NULL)
                {
                    if (cs->valueType >= CONTAINER && cs->valueType != LINK && cs->valueType != FLAG)
                    {
                        if (cs->items[i]->isover)
                            return false;
                    }
                }
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        Type typi = pe->items[i]->value->type;
                        if (typi >= CONTAINER && typi != LINK && typi != FLAG)
                        {
                            if (pe->items[i]->isover)
                                return false;
                        }
                    }
                }
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {
                    if (mp->items[i]->isover)
                        return false;
                }
            }
            break;
        }
    }
    return true;
}

void SetTreeCloseState(BinField* value, ImGuiWindow* window)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(stosz, "SetTreeOpenState", 50, true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->value != NULL)
                {
                    if (cs->valueType >= CONTAINER && cs->valueType != LINK && cs->valueType != FLAG)
                    {
                        cs->items[i]->cursormax.x = 0.f;
                        window->DC.StateStorage->SetInt(cs->items[i]->idim, 0);
                        SetTreeCloseState(cs->items[i]->value, window);
                    }
                }
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                window->DC.StateStorage->SetInt(pe->idim, 0);
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        Type typi = pe->items[i]->value->type;
                        if (typi >= CONTAINER && typi != LINK && typi != FLAG)
                        {
                            pe->items[i]->cursormax.x = 0.f;
                            window->DC.StateStorage->SetInt(pe->items[i]->idim, 0);
                            SetTreeCloseState(pe->items[i]->value, window);
                        }
                    }
                }
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {
                    mp->items[i]->cursormax.x = 0.f;
                    window->DC.StateStorage->SetInt(mp->items[i]->idim, 0);
                    if (mp->keyType >= CONTAINER && mp->keyType != LINK && mp->keyType != FLAG)
                        SetTreeCloseState(mp->items[i]->key, window);
                    if (mp->valueType >= CONTAINER && mp->valueType != LINK && mp->valueType != FLAG)
                        SetTreeCloseState(mp->items[i]->value, window);
                }
            }
            break;
        }
    }
}

BinField* BinFieldClean(Type typi, Type typo, Type typu)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(bfcz, "BinFieldClean", true);
    #endif
    BinField* result = (BinField*)callocb(1, sizeof(BinField));
    result->type = typi;
    uint32_t size = Type_sizeclean[typi];
    myassert(size == NULL);
    result->data = callocb(1, size);
    switch (typi)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)result->data;
            cs->valueType = typo;
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)result->data;
            mp->keyType = typo;
            mp->valueType = typu;
            break;
        }
    }
    return result;
}

void BinFieldAdd(uintptr_t id, int* current1, int* current2, int* current3, bool showfirst = false)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(bfaz, "BinFieldAdd", true);
    #endif
    if (showfirst)
    {
        ImGui::PushID((void*)id);
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", NULL, true).x * 1.5f);
        ImGui::SameLine(); ImGui::Combo("", current1, Type_strings, 27);
        ImGui::PopID();
        #ifdef _DEBUG
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%d", id);
        #endif
    }
    if (*current1 == CONTAINER || *current1 == STRUCT || *current1 == OPTION || *current1 == MAP)
    {
        ImGui::PushID((void*)(id + (showfirst ? 1 : 0)));
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", NULL, true).x * 1.5f);
        ImGui::SameLine(); ImGui::Combo("", current2, Type_strings, 27);
        ImGui::PopID();
        #ifdef _DEBUG
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%d", id + (showfirst ? 1 : 0));
        #endif
    }
    if (*current1 == MAP)
    {
        ImGui::PushID((void*)(id + (showfirst ? 2 : 1)));
        ImGui::SetNextItemWidth(ImGui::CalcTextSize("WadEntryLink", NULL, true).x * 1.5f);
        ImGui::SameLine(); ImGui::Combo("", current3, Type_strings, 27);
        ImGui::PopID();
        #ifdef _DEBUG
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%d", id + (showfirst ? 2 : 1));
        #endif
    }
}

bool IsItemVisible(ImGuiWindow* window)
{
#ifdef TRACY_ENABLE_ZONES
    ZoneNamedN(iivz, "IsItemVisible", true);
#endif
    ImVec2 cursor = window->DC.CursorPos;
    ImVec2 clipmin = window->ClipRect.Min;
    ImVec2 clipmax = window->ClipRect.Max;
#ifdef NDEBUG
    clipmin.y -= 50; clipmax.y += 50;
#else
    clipmin.y += 50; clipmax.y -= 50;
#endif
    return cursor.y > clipmin.y && cursor.y < clipmax.y;
}

bool MyButtonEx(uintptr_t ide)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedN(bez, "ButtonExe", true);
    #endif
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID((void*)ide);

    ImVec2 pos = ImVec2(window->DC.CursorPos.x - g.FontSize / 2.f, window->DC.CursorPos.y);
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
    ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    window->DrawList->AddCircleFilled(center, ImMax(2.f, g.FontSize * 0.5f + 1.f), col, 12);
    float cross_extent = g.FontSize * 0.5f * 0.7071f - 1.f;
    ImU32 cross_col = ImGui::GetColorU32(ImGuiCol_Text);
    center -= ImVec2(0.5f, 0.5f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent), center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent), center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.f);

    return pressed;
}

bool MyTreeNodeEx(uintptr_t idp)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiID id = window->GetID((void*)idp);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth;

    ImRect frame_bb;
    const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), g.FontSize + style.FramePadding.y * 2);
    frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
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
    const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
    bool is_open = ImGui::TreeNodeBehaviorIsOpen(id, flags);
    if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

    bool item_add = ImGui::ItemAdd(interact_bb, id);
    window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    window->DC.LastItemDisplayRect = frame_bb;

    if (!item_add)
    {
        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
            ImGui::TreePushOverrideID(id);
        return is_open;
    }

    ImGuiButtonFlags button_flags = ImGuiTreeNodeFlags_None;
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        button_flags |= ImGuiButtonFlags_AllowItemOverlap;
    if (!is_leaf)
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

    const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
    const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + style.FramePadding.x * 2.0f) + style.TouchExtraPadding.x;
    const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
    if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_NoKeyModifiers;

    if (is_mouse_x_over_arrow)
        button_flags |= ImGuiButtonFlags_PressedOnClick;
    else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    else
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

    bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
    const bool was_selected = selected;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    bool toggled = false;
    if (!is_leaf)
    {
        if (pressed && g.DragDropHoldJustPressedId != id)
        {
            if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
                toggled = true;
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover;
            if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
                toggled = true;
        }
        else if (pressed && g.DragDropHoldJustPressedId == id)
        {
            IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
            if (!is_open) 
                toggled = true;
        }

        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
        {
            toggled = true;
            ImGui::NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open)
        {
            toggled = true;
            ImGui::NavMoveRequestCancel();
        }

        if (toggled)
        {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(id, is_open);
            window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
        }
    }
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        ImGui::SetItemAllowOverlap();

    if (selected != was_selected) 
        window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

    const ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;

    const ImU32 bg_col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
    if (IsItemVisible(window))
    {
        ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
        ImGui::RenderNavHighlight(frame_bb, id, nav_highlight_flags);
        ImGui::RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + style.FramePadding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
    }

    if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
        frame_bb.Max.x -= g.FontSize + style.FramePadding.x;

    if (g.LogEnabled)
        ImGui::LogSetNextTextDecoration("###", "###");

    if (is_open)
        ImGui::TreePushOverrideID(id);

    return is_open;
}

bool BinFieldDelete(uintptr_t id)
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

    if (retb)
        ImGui::OpenPopupEx(GImGui->CurrentWindow->GetID((void*)id));
    if (GImGui->OpenPopupStack.Size <= GImGui->BeginPopupStack.Size)
    {
        GImGui->NextWindowData.ClearFlags();
        return false;
    }
    if (ImGui::BeginPopupEx(GImGui->CurrentWindow->GetID((void*)id),
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings))
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

void NewLine(ImGuiWindow* window)
{
    ImRect total_bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x, window->DC.CursorPos.y + GImGui->FontSize + GImGui->Style.FramePadding.y * 2.f));
    ImGui::ItemSize(total_bb, GImGui->Style.FramePadding.y);
}

void DrawRectRainBow(BinField* value, ImGuiWindow* window, float depth)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(drrbz, "DrawRectRainBow", 50, true);
    #endif
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            float degresshsv;
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            if (cs->itemsize > 0)
                degresshsv = fmodf(depth, 360.f) / 360.f;
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->value != NULL)
                {
                    if (cs->valueType >= CONTAINER && cs->valueType != LINK && cs->valueType != FLAG)
                    {
                        if (cs->items[i]->expanded)
                        {
                            if (cs->items[i]->cursormax.x != 0.f)
                            {
                                if (cs->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) && 
                                    cs->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                                {
                                    cs->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                    cs->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                    cs->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                    ImGui::ItemAdd(ImRect(cs->items[i]->cursormin, cs->items[i]->cursormax), 0);
                                    ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
                                    cs->items[i]->isover = ImGui::IsItemHovered();
                                    if (cs->items[i]->isover)
                                        if (CanChangeBackcolor(cs->items[i]->value))
                                            col.SetHSV(degresshsv, 1.f, .4f);
                                    window->DrawList->AddRectFilled(cs->items[i]->cursormin, cs->items[i]->cursormax, col);
                                    window->DrawList->AddRect(cs->items[i]->cursormin, cs->items[i]->cursormax,
                                        ImColor().HSV(degresshsv, 1.f, 1.f));
                                    cs->items[i]->cursormax.x = 0.f;
                                }
                            }
                            DrawRectRainBow(cs->items[i]->value, window, depth + 10.f);
                        }
                    }
                }
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            float degresshsv;
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->itemsize > 0)
                degresshsv = fmodf(depth, 360.f) / 360.f;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        Type typi = pe->items[i]->value->type;
                        if (typi >= CONTAINER && typi != LINK && typi != FLAG)
                        {
                            if (pe->items[i]->expanded)
                            {
                                if (pe->items[i]->cursormax.x != 0.f)
                                {
                                    if (pe->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) &&
                                        pe->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                                    {
                                        pe->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                        pe->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                        pe->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                        ImGui::ItemAdd(ImRect(pe->items[i]->cursormin, pe->items[i]->cursormax), 0);
                                        ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
                                        pe->items[i]->isover = ImGui::IsItemHovered();
                                        if (pe->items[i]->isover)
                                            if (CanChangeBackcolor(pe->items[i]->value))
                                                col.SetHSV(degresshsv, 1.f, .4f);
                                        window->DrawList->AddRectFilled(pe->items[i]->cursormin, pe->items[i]->cursormax, col);
                                        window->DrawList->AddRect(pe->items[i]->cursormin, pe->items[i]->cursormax,
                                            ImColor().HSV(degresshsv, 1.f, 1.f));
                                        pe->items[i]->cursormax.x = 0.f;
                                    }
                                }
                                DrawRectRainBow(pe->items[i]->value, window, depth + 10.f);
                            }
                        }
                    }
                }
            }
            break;
        }
        case MAP:
        {
            float degresshsv;
            Map* mp = (Map*)value->data;
            if (mp->itemsize > 0)
                degresshsv = fmodf(depth, 360.f) / 360.f;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {
                    if (mp->items[i]->expanded)
                    {
                        if (mp->items[i]->cursormax.x != 0.f)
                        {
                            if (mp->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) &&
                                mp->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                            {
                                mp->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                mp->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                mp->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                ImGui::ItemAdd(ImRect(mp->items[i]->cursormin, mp->items[i]->cursormax), 0);
                                ImColor col; col.SetHSV(degresshsv, 1.f, .35f);
                                mp->items[i]->isover = ImGui::IsItemHovered();
                                if (mp->items[i]->isover)
                                    if (CanChangeBackcolor(mp->items[i]->value))
                                        col.SetHSV(degresshsv, 1.f, .4f);
                                window->DrawList->AddRectFilled(mp->items[i]->cursormin, mp->items[i]->cursormax, col);
                                window->DrawList->AddRect(mp->items[i]->cursormin, mp->items[i]->cursormax,
                                    ImColor().HSV(degresshsv, 1.f, 1.f));
                                mp->items[i]->cursormax.x = 0.f;
                            }
                        }
                        DrawRectRainBow(mp->items[i]->key, window, depth + 10.f);
                        DrawRectRainBow(mp->items[i]->value, window, depth + 10.f);
                    }
                }
            }
            break;
        }
    }
}

void DrawRectNormal(BinField* value, ImGuiWindow* window, float depth)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(drnz, "DrawRectNormal", 50, true);
    #endif
    if (depth > 0.15f)
        depth = 0.09f;
    switch (value->type)
    {
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
            {
                if (cs->items[i]->value != NULL)
                {
                    if (cs->valueType >= CONTAINER && cs->valueType != LINK && cs->valueType != FLAG)
                    {
                        if (cs->items[i]->expanded)
                        {
                            if (cs->items[i]->cursormax.x != 0.f)
                            {
                                if (cs->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) &&
                                    cs->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                                {
                                    cs->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                    cs->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                    cs->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                    ImGui::ItemAdd(ImRect(cs->items[i]->cursormin, cs->items[i]->cursormax), 0);
                                    ImColor col; col.SetHSV(0.f, .0f, depth);
                                    cs->items[i]->isover = ImGui::IsItemHovered();
                                    if (cs->items[i]->isover)
                                        if (CanChangeBackcolor(cs->items[i]->value))
                                            col.SetHSV(0.f, .0f, depth * 1.3f);
                                    window->DrawList->AddRectFilled(cs->items[i]->cursormin, cs->items[i]->cursormax, col);
                                    window->DrawList->AddRect(cs->items[i]->cursormin, cs->items[i]->cursormax, IM_COL32(128, 128, 128, 255));
                                    cs->items[i]->cursormax.x = 0.f;
                                }
                            }
                            DrawRectNormal(cs->items[i]->value, window, depth + 0.01f);
                        }
                    }
                }
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        Type typi = pe->items[i]->value->type;
                        if (typi >= CONTAINER && typi != LINK && typi != FLAG)
                        {
                            if (pe->items[i]->expanded)
                            {
                                if (pe->items[i]->cursormax.x != 0.f)
                                {
                                    if (pe->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) &&
                                        pe->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                                    {
                                        pe->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                        pe->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                        pe->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                        ImGui::ItemAdd(ImRect(pe->items[i]->cursormin, pe->items[i]->cursormax), 0);
                                        ImColor col; col.SetHSV(0.f, .0f, depth);
                                        pe->items[i]->isover = ImGui::IsItemHovered();
                                        if (pe->items[i]->isover)
                                            if (CanChangeBackcolor(pe->items[i]->value))
                                                col.SetHSV(0.f, .0f, depth * 1.3f);
                                        window->DrawList->AddRectFilled(pe->items[i]->cursormin, pe->items[i]->cursormax, col);
                                        window->DrawList->AddRect(pe->items[i]->cursormin, pe->items[i]->cursormax, IM_COL32(128, 128, 128, 255));
                                        pe->items[i]->cursormax.x = 0.f;
                                    }
                                }
                                DrawRectNormal(pe->items[i]->value, window, depth + 0.01f);
                            }
                        }
                    }
                }
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {
                    if (mp->items[i]->expanded)
                    {
                        if (mp->items[i]->cursormax.x != 0.f)
                        {
                            if (mp->items[i]->cursormin.y < (window->ClipRect.Max.y + 50.f) &&
                                mp->items[i]->cursormax.y > (window->ClipRect.Min.y + 50.f))
                            {
                                mp->items[i]->cursormin.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
                                mp->items[i]->cursormax.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
                                mp->items[i]->cursormax.y += IM_FLOOR(GImGui->Style.FramePadding.y * 0.75f);
                                ImGui::ItemAdd(ImRect(mp->items[i]->cursormin, mp->items[i]->cursormax), 0);
                                ImColor col; col.SetHSV(0.f, .0f, depth);
                                mp->items[i]->isover = ImGui::IsItemHovered();
                                if (mp->items[i]->isover)
                                    if (CanChangeBackcolor(mp->items[i]->value))
                                        col.SetHSV(0.f, .0f, depth * 1.3f);
                                window->DrawList->AddRectFilled(mp->items[i]->cursormin, mp->items[i]->cursormax, col);
                                window->DrawList->AddRect(mp->items[i]->cursormin, mp->items[i]->cursormax, IM_COL32(128, 128, 128, 255));
                                mp->items[i]->cursormax.x = 0.f;
                            }
                        }
                        DrawRectNormal(mp->items[i]->key, window, depth + 0.01f);
                        DrawRectNormal(mp->items[i]->value, window, depth + 0.01f);
                    }
                }
            }
            break;
        }
    }
}

void GetValueFromType(BinField* value, HashTable* hasht, uintptr_t* treeid,
    ImGuiWindow* window, bool opentree, bool* previousnode = NULL, ImVec2* cursor = NULL)
{
    #ifdef TRACY_ENABLE_ZONES
        ZoneNamedNS(gvftz, "GetValueFromType", 50, true);
        gvftz.Text(Type_strings[value->type], strlen(Type_strings[value->type]));
    #endif

    switch (value->type)
    {
        case NONE:
            ImGui::Text("NULL");
            break;
        case SInt8:
        case UInt8:
        case SInt16:
        case UInt16:
        case SInt32:
        case UInt32:
        case SInt64:
        case UInt64:
        {
            char* buf = (char*)callocb(64, 1);
            const char* fmt = Type_fmt[value->type];
            myassert(sprintf_s(buf, 64, fmt, *(uint64_t*)value->data) <= 0);
            char* string = InputText(buf, value->id);
            if (string != NULL)
            {
                myassert(sscanf_s(string, fmt, value->data) <= 0);
                freeb(string);
            }
            freeb(buf);
            break;
        }
        case FLAG:
        case BOOLB:
        {
            char* string = InputText(*(uint8_t*)value->data == 1 ? "true" : "false", value->id);
            if (string != NULL)
            {
                for (int i = 0; string[i]; i++)
                    string[i] = tolower(string[i]);
                if (strcmp(string, "true") == 0)
                    *(uint8_t*)value->data = 1;
                else
                    *(uint8_t*)value->data = 0;
            }
            freeb(string);
            break;
        }
        case Float32:
        case VEC2:
        case VEC3:
        case VEC4:
        case MTX44:
            FloatArray(value->data, Type_size[value->type] / 4, value->id);
            break;
        case RGBA:
        {
            uint8_t* arr = (uint8_t*)value->data;
            static float size = ImGui::CalcTextSize("255").x;
            for (int i = 0; i < 4; i++)
            {
                char* buf = (char*)callocb(64, 1);
                myassert(sprintf_s(buf, 64, "%" PRIu8, arr[i]) <= 0);
                char* string = InputText(buf, value->id + i, size);
                if (string != NULL)
                    myassert(sscanf_s(string, "%" PRIu8, &arr[i]) <= 0);
                freeb(string);
                freeb(buf);
                if (i < 3)
                    ImGui::SameLine();
            }
            break;
        }
        case STRING:
        {
            char* string = InputText((char*)value->data, value->id);
            if (string != NULL)
            {
                freeb(value->data);
                value->data = string;
            }
            break;
        }
        case HASH:
        case LINK:
            InputTextHash(hasht, (uint32_t*)value->data, value->id);
            break;
        case WADENTRYLINK:
            InputTextHashxx(hasht, (uint64_t*)value->data, value->id);
            break;
        case OPTION:
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
#ifdef TRACY_ENABLE_ZONES
            ZoneNamedN(cspz, "ContainerOrStructOrOption", true);
            cspz.Text(Type_strings[cs->valueType], strlen(Type_strings[cs->valueType]));
#endif
            if (cs->valueType == CONTAINER || cs->valueType == STRUCT || cs->valueType == OPTION || cs->valueType == MAP)
            {
                for (uint32_t i = 0; i < cs->itemsize; i++)
                {
                    if (cs->items[i]->value != NULL)
                    {
                        if (opentree)
                            ImGui::SetNextItemOpen(true);
                        cs->items[i]->cursormin = window->DC.CursorPos;
                        ImGui::AlignTextToFramePadding();
                        cs->items[i]->expanded = MyTreeNodeEx(cs->items[i]->id);
                        cs->items[i]->idim = window->IDStack.back();
                        ImGui::AlignTextToFramePadding();
                        if (IsItemVisible(window))
                        {
#ifdef _DEBUG
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("%d", cs->items[i]->id);
#endif
                            ImGui::SameLine(); ImGui::Text("%s", Type_strings[cs->valueType]);
                            ImGui::SameLine(0, 1); GetArrayType(cs->items[i]->value);
                            ImGui::SameLine(); GetArrayCount(cs->items[i]->value);
                            ImGui::SameLine();
                            if (BinFieldDelete(cs->items[i]->id+1))
                            {
                                ClearBin(cs->items[i]->value);
                                cs->items[i]->value = NULL;
                                if (cs->items[i]->expanded)
                                    ImGui::TreePop();
                            }
                            else if (cs->items[i]->expanded) {
                                ImGui::Indent();
                                GetValueFromType(cs->items[i]->value, hasht, treeid, window, opentree);
                                ImGui::Unindent();
                                ImGui::TreePop();
                            }
                        }
                        else if (cs->items[i]->expanded) {
                            ImGui::Indent();
                            GetValueFromType(cs->items[i]->value, hasht, treeid, window, opentree);
                            ImGui::Unindent();
                            ImGui::TreePop();
                        }
                        if (cs->items[i]->expanded)
                            cs->items[i]->cursormax = ImVec2(window->WorkRect.Max.x, window->DC.CursorMaxPos.y);                  
                        else
                            cs->items[i]->isover = false;
                    }
                }
            }
            else if (cs->valueType == POINTER || cs->valueType == EMBEDDED) {
                for (uint32_t i = 0; i < cs->itemsize; i++)
                {
                    if (cs->items[i]->value != NULL)
                    {
                        if (opentree)
                            ImGui::SetNextItemOpen(true);
                        cs->items[i]->cursormin = window->DC.CursorPos;
                        ImGui::AlignTextToFramePadding();
                        cs->items[i]->expanded = MyTreeNodeEx(cs->items[i]->id);
                        cs->items[i]->idim = window->IDStack.back();
                        if (IsItemVisible(window))
                        {
                            ImVec2 cursore, cursor;
#ifdef _DEBUG
                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip("%d", cs->items[i]->id);
#endif
                            ImGui::SameLine();
                            GetValueFromType(cs->items[i]->value, hasht, treeid, window,
                                opentree, &cs->items[i]->expanded, &cursor);
                            cursore = ImGui::GetCursorPos();
                            ImGui::SetCursorPos(cursor);
                            if (BinFieldDelete(cs->items[i]->id+1))
                            {
                                ClearBin(cs->items[i]->value);
                                cs->items[i]->value = NULL;
                            }
                            ImGui::SetCursorPos(cursore);
                        }
                        else if (cs->items[i]->expanded) {
                            ImGui::SameLine();
                            GetValueFromType(cs->items[i]->value, hasht, treeid, window,
                                opentree, &cs->items[i]->expanded);
                        }
                        if (cs->items[i]->expanded)
                        {
                            ImGui::TreePop();
                            if (((PointerOrEmbed*)cs->items[i]->value->data)->name == NULL)
                            {
                                cs->items[i]->cursormax.x = 0.f;
                                continue;
                            }
                            cs->items[i]->cursormax = ImVec2(window->WorkRect.Max.x, window->DC.CursorMaxPos.y);
                        } 
                        else
                            cs->items[i]->isover = false;
                    }
                }
            }
            else {
                for (uint32_t i = 0; i < cs->itemsize; i++)
                {
                    if (cs->items[i]->value != NULL)
                    {
                        if (IsItemVisible(window))
                        {
                            ImGui::AlignTextToFramePadding();
                            GetValueFromType(cs->items[i]->value, hasht, treeid, window, opentree);
                            ImGui::SameLine();
                            if (BinFieldDelete(cs->items[i]->id))
                            {
                                ClearBin(cs->items[i]->value);
                                cs->items[i]->value = NULL;
                            }
                        }
                        else if (cs->valueType == MTX44) {
                            for (int o = 0; o < 4; o++)
                                NewLine(window);
                        }
                        else {
                            NewLine(window);
                        }
                    }
                }
            }
            if (IsItemVisible(window))
            {
                int typi = cs->valueType;
                bool add = ImGui::Button("Add new item");
                BinFieldAdd(value->id, &typi, &cs->current2, &cs->current3);
                if (add)
                {
                    cs->itemsize += 1;
                    cs->items = (IdField**)reallocb(cs->items, cs->itemsize * sizeof(IdField*));
                    cs->items[cs->itemsize - 1] = (IdField*)callocb(1, sizeof(IdField));
                    cs->items[cs->itemsize - 1]->value = BinFieldClean(cs->valueType, (Type)cs->current2, (Type)cs->current3);
                    GetStructIdBin(value, treeid);
                }
            }
            else {
                NewLine(window);
            }
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
#ifdef TRACY_ENABLE_ZONES
            ZoneNamedN(pez, "PointerOrEmbed", true);
#endif
            if (pe->name != NULL)
            {
                bool peexpanded = false;
                ImGui::AlignTextToFramePadding();
                if (previousnode == NULL)
                {
                    if (opentree)
                        ImGui::SetNextItemOpen(true);
                    peexpanded = MyTreeNodeEx(value->id);
                    pe->idim = window->IDStack.back();
                    ImGui::AlignTextToFramePadding();
                    if (IsItemVisible(window))
                    {
#ifdef _DEBUG
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%d", value->id);
#endif
                        ImGui::SameLine();
                        InputTextHash(hasht, &pe->name, value->id + 1);
                        ImGui::SameLine(); ImGui::Text(": %s", Type_strings[value->type]);
                        ImGui::SameLine(); GetArrayCount(value);
                    }
                    else {
                        NewLine(window);
                    }
                }
                else {
                    peexpanded = *previousnode;
                    if (IsItemVisible(window))
                    {
                        ImGui::SameLine();
                        InputTextHash(hasht, &pe->name, value->id);
                        ImGui::SameLine(); GetArrayCount(value);
                    }
                    else {
                        NewLine(window);
                    }
                }
                if (cursor != NULL)
                {
                    ImVec2 old = ImGui::GetCursorPos();
                    ImGui::SameLine(); *cursor = ImGui::GetCursorPos();
                    ImGui::SetCursorPos(old);
                }
                if (peexpanded)
                {
                    ImGui::Indent();
                    for (uint16_t i = 0; i < pe->itemsize; i++)
                    {
                        if (pe->items[i]->value != NULL)
                        {
                            Type typi = pe->items[i]->value->type;
#ifdef TRACY_ENABLE_ZONES
                            pez.Text(Type_strings[typi], strlen(Type_strings[typi]));
#endif
                            if ((typi >= CONTAINER && typi <= EMBEDDED) || typi == OPTION || typi == MAP)
                            {
                                if (opentree)
                                    ImGui::SetNextItemOpen(true);
                                pe->items[i]->cursormin = window->DC.CursorPos;
                                ImGui::AlignTextToFramePadding();
                                pe->items[i]->expanded = MyTreeNodeEx(pe->items[i]->id);
                                pe->items[i]->idim = window->IDStack.back();
                                if (IsItemVisible(window))
                                {
#ifdef _DEBUG
                                    if (ImGui::IsItemHovered())
                                        ImGui::SetTooltip("%d", pe->items[i]->id);
#endif
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::SameLine(); InputTextHash(hasht, &pe->items[i]->key, pe->items[i]->id+1);
                                    if (typi != POINTER && typi != EMBEDDED)
                                    {
                                        ImGui::SameLine(); ImGui::Text(": %s", Type_strings[typi]);
                                        ImGui::SameLine(0, 1); GetArrayType(pe->items[i]->value);
                                        ImGui::SameLine(); GetArrayCount(pe->items[i]->value);
                                        ImGui::SameLine();
                                        if (BinFieldDelete(pe->items[i]->id+2))
                                        {
                                            ClearBin(pe->items[i]->value);
                                            pe->items[i]->value = NULL;
                                            if (pe->items[i]->expanded)
                                                ImGui::TreePop();
                                        }
                                        else if (pe->items[i]->expanded) {
                                            ImGui::Indent();
                                            GetValueFromType(pe->items[i]->value, hasht, treeid, window, opentree);
                                            ImGui::Unindent();
                                            ImGui::TreePop();
                                        }
                                    }
                                    else {
                                        ImVec2 cursore, cursor;
                                        ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
                                        GetValueFromType(pe->items[i]->value, hasht, treeid, window, 
                                            opentree, &pe->items[i]->expanded, &cursor);
                                        cursore = ImGui::GetCursorPos();
                                        ImGui::SetCursorPos(cursor);
                                        if (BinFieldDelete(pe->items[i]->id+2))
                                        {
                                            ClearBin(pe->items[i]->value);
                                            pe->items[i]->value = NULL;
                                        }
                                        ImGui::SetCursorPos(cursore);
                                        if (pe->items[i]->expanded)
                                            ImGui::TreePop();
                                    }
                                }
                                else if (typi != POINTER && typi != EMBEDDED) {
                                    if (pe->items[i]->expanded)
                                    {
                                        ImGui::Indent();
                                        GetValueFromType(pe->items[i]->value, hasht, treeid, window, opentree);
                                        ImGui::Unindent();
                                        ImGui::TreePop();
                                    }          
                                }
                                else {
                                    ImGui::SameLine();
                                    GetValueFromType(pe->items[i]->value, hasht, treeid, window,
                                            opentree, &pe->items[i]->expanded);
                                    if (pe->items[i]->expanded)
                                        ImGui::TreePop();
                                }
                                if (pe->items[i]->expanded)
                                {
                                    if (typi == POINTER || typi == EMBEDDED)
                                    {
                                        if (((PointerOrEmbed*)pe->items[i]->value->data)->name == NULL)
                                        {
                                            pe->items[i]->cursormax.x = 0.f;
                                            continue;
                                        }
                                    }
                                    pe->items[i]->cursormax = ImVec2(window->WorkRect.Max.x, window->DC.CursorMaxPos.y);
                                }
                                else
                                    pe->items[i]->isover = false;
                            }
                            else if (IsItemVisible(window)) {
                                ImGui::AlignTextToFramePadding();
                                InputTextHash(hasht, &pe->items[i]->key, pe->items[i]->id);
                                ImGui::SameLine(); ImGui::Text(": %s", Type_strings[typi]);
                                ImGui::SameLine(); ImGui::Text("="); ImGui::SameLine();
                                GetValueFromType(pe->items[i]->value, hasht, treeid, window, opentree);
                                ImGui::SameLine();
                                if (BinFieldDelete(pe->items[i]->id+1))
                                {
                                    ClearBin(pe->items[i]->value);
                                    pe->items[i]->value = NULL;
                                }
                            }
                            else if (typi == MTX44) {
                                for (int o = 0; o < 4; o++)
                                    NewLine(window);
                            }
                            else {
                                NewLine(window);
                            }
                        }
                    }
                    if (IsItemVisible(window))
                    {
                        bool add = ImGui::Button("Add new item");
                        BinFieldAdd(value->id+2, &pe->current1, &pe->current2, &pe->current3, true);
                        if (add)
                        {
                            pe->itemsize += 1;
                            pe->items = (Field**)reallocb(pe->items, pe->itemsize * sizeof(Field*));
                            pe->items[pe->itemsize - 1] = (Field*)callocb(1, sizeof(Field));
                            pe->items[pe->itemsize - 1]->value = BinFieldClean((Type)pe->current1, (Type)pe->current2, (Type)pe->current3);
                            GetStructIdBin(value, treeid);
                        }
                    }
                    else {
                        NewLine(window);
                    }
                    ImGui::Unindent();
                    if (previousnode == NULL)
                        ImGui::TreePop();
                }
            }
            else {
                if (IsItemVisible(window))
                    InputTextHash(hasht, &pe->name, value->id);
                else
                    NewLine(window);
            }
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
#ifdef TRACY_ENABLE_ZONES
            ZoneNamedN(mpz, "Map", true);
#endif
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                if (mp->items[i]->key != NULL)
                {
                    if (opentree)
                        ImGui::SetNextItemOpen(true);
                    mp->items[i]->cursormin = window->DC.CursorPos;
                    ImGui::AlignTextToFramePadding();
                    mp->items[i]->expanded = MyTreeNodeEx(mp->items[i]->id);
                    mp->items[i]->idim = window->IDStack.back();
                    ImGui::AlignTextToFramePadding();
                    if (IsItemVisible(window))
                    {
#ifdef _DEBUG
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("%d", mp->items[i]->id);
#endif
#ifdef TRACY_ENABLE_ZONES
                        mpz.Text(Type_strings[mp->keyType], strlen(Type_strings[mp->keyType]));
                        mpz.Text(Type_strings[mp->valueType], strlen(Type_strings[mp->valueType]));
#endif
                        ImGui::SameLine(); ImGui::Text("%s", Type_strings[mp->keyType]);
                        ImGui::SameLine(); ImGui::Text("="); ImGui::SameLine();
                        GetValueFromType(mp->items[i]->key, hasht, treeid, window, opentree);
                        ImGui::SameLine();
                        if (BinFieldDelete(mp->items[i]->id+1))
                        {
                            bool open = mp->items[i]->expanded;
                            ClearBin(mp->items[i]->key);
                            ClearBin(mp->items[i]->value);
                            mp->items[i]->key = NULL;
                            mp->items[i]->value = NULL;
                            if (open)
                            {
                                ImGui::TreePop();
                                continue;
                            }
                        }
                    }
                    else if (mp->keyType >= CONTAINER && mp->keyType != LINK && mp->keyType != FLAG) {
                        GetValueFromType(mp->items[i]->key, hasht, treeid, window, opentree);
                    }
                    else if (mp->keyType == MTX44) {
                        for (int o = 0; o < 3; o++)
                            NewLine(window);
                    }
                    if (mp->items[i]->expanded)
                    {
                        if (IsItemVisible(window))
                        {
                            ImGui::Indent();
                            ImGui::AlignTextToFramePadding();
                            if ((mp->valueType <= WADENTRYLINK) || mp->valueType == LINK || mp->valueType == FLAG)
                            {
                                ImGui::Text("%s", Type_strings[mp->valueType]);
                                ImGui::SameLine(); ImGui::Text("="); ImGui::SameLine();
                            }
                            GetValueFromType(mp->items[i]->value, hasht, treeid, window, opentree);
                            ImGui::Unindent();
                        }
                        else if (mp->valueType >= CONTAINER && mp->valueType != LINK && mp->valueType != FLAG) {
                            ImGui::Indent();
                            GetValueFromType(mp->items[i]->value, hasht, treeid, window, opentree);
                            ImGui::Unindent();
                        }
                        else if (mp->valueType == MTX44) {
                            for (int o = 0; o < 4; o++)
                                NewLine(window);
                        }
                        else {
                            NewLine(window);
                        }
                        ImGui::TreePop();
                        if (mp->keyType == POINTER || mp->keyType == EMBEDDED)
                        {
                            if (((PointerOrEmbed*)mp->items[i]->value->data)->name == NULL)
                            {
                                mp->items[i]->cursormax.x = 0.f;
                                continue;
                            }
                        }
                        mp->items[i]->cursormax = ImVec2(window->WorkRect.Max.x, window->DC.CursorMaxPos.y);
                    }
                    else
                        mp->items[i]->isover = false;
                }
            }
            if (IsItemVisible(window))
            {
                int typi = mp->keyType;
                int typf = mp->valueType;
                bool add = ImGui::Button("Add new item");
                BinFieldAdd(value->id, &typi, &mp->current2, &mp->current3);
                BinFieldAdd(value->id + 1, &typf, &mp->current4, &mp->current5);
                if (add)
                {
                    mp->itemsize += 1;
                    mp->items = (Pair**)reallocb(mp->items, mp->itemsize * sizeof(Pair*));
                    mp->items[mp->itemsize - 1] = (Pair*)callocb(1, sizeof(Pair));
                    mp->items[mp->itemsize - 1]->key = BinFieldClean(mp->keyType, (Type)mp->current2, (Type)mp->current3);
                    mp->items[mp->itemsize - 1]->value = BinFieldClean(mp->valueType, (Type)mp->current4, (Type)mp->current5);
                    GetStructIdBin(value, treeid);
                }
            }
            else {
                NewLine(window);
            }
            break;
        }
    }
}

#pragma region FromBinReader

BinField* ReadValueByType(uint8_t typeidbin, HashTable* hasht, char** fp)
{
    BinField* result = (BinField*)callocb(1, sizeof(BinField));
    result->type = Uint8ToType(typeidbin);
    myassert(result->type > FLAG);
    int size = Type_size[result->type];
    if (size != 0)
    {
        void* data = (void*)callocb(1, size);
        memfread(data, size, fp);
        result->data = data;
    }
    else
    {
        switch (result->type)
        {
            case STRING:
            {
                uint16_t stringlength = 0;
                memfread(&stringlength, 2, fp);
                char* stringb = (char*)callocb(stringlength + 1, 1);
                memfread(stringb, (size_t)stringlength, fp);
                stringb[stringlength] = '\0';
                result->data = stringb;
                InsertHashTable(hasht, FNV1Hash(stringb), stringb);
                break;
            }
            case STRUCT:
            case CONTAINER:
            {
                uint8_t type = 0;
                uint32_t size = 0;
                uint32_t count = 0;
                ContainerOrStructOrOption* tmpcs = (ContainerOrStructOrOption*)callocb(1, sizeof(ContainerOrStructOrOption));
                memfread(&type, 1, fp);
                memfread(&size, 4, fp);
                memfread(&count, 4, fp);
                tmpcs->itemsize = count;
                tmpcs->valueType = Uint8ToType(type);
                tmpcs->items = (IdField**)callocb(count, sizeof(IdField*));
                for (uint32_t i = 0; i < count; i++)
                {
                    tmpcs->items[i] = (IdField*)callocb(1, sizeof(IdField));
                    tmpcs->items[i]->value = ReadValueByType(tmpcs->valueType, hasht, fp);
                }
                result->data = tmpcs;
                break;
            }
            case POINTER:
            case EMBEDDED:
            {
                uint32_t size = 0;
                uint16_t count = 0;
                PointerOrEmbed* tmppe = (PointerOrEmbed*)callocb(1, sizeof(PointerOrEmbed));
                memfread(&tmppe->name, 4, fp);
                if (tmppe->name == 0)
                {
                    result->data = tmppe;
                    break;
                }
                memfread(&size, 4, fp);
                memfread(&count, 2, fp);
                tmppe->itemsize = count;
                tmppe->items = (Field**)callocb(count, sizeof(Field*));
                for (uint16_t i = 0; i < count; i++)
                {
                    uint8_t type = 0;
                    Field* tmpfield = (Field*)callocb(1, sizeof(Field));
                    memfread(&tmpfield->key, 4, fp);
                    memfread(&type, 1, fp);
                    tmpfield->value = ReadValueByType(type, hasht, fp);
                    tmppe->items[i] = tmpfield;
                }
                result->data = tmppe;
                break;
            }
            case OPTION:
            {
                uint8_t type = 0;
                uint8_t count = 0;
                ContainerOrStructOrOption* tmpo = (ContainerOrStructOrOption*)callocb(1, sizeof(ContainerOrStructOrOption));
                memfread(&type, 1, fp);
                memfread(&count, 1, fp);
                tmpo->itemsize = count;
                tmpo->valueType = Uint8ToType(type);
                tmpo->items = (IdField**)callocb(count, sizeof(IdField*));
                for (uint32_t i = 0; i < count; i++)
                {
                    tmpo->items[i] = (IdField*)callocb(1, sizeof(IdField));
                    tmpo->items[i]->value = ReadValueByType(tmpo->valueType, hasht, fp);
                }
                result->data = tmpo;
                break;
            }
            case MAP:
            {
                uint32_t size = 0;
                uint8_t typek = 0;
                uint8_t typev = 0;
                uint32_t count = 0;
                Map* tmpmap = (Map*)callocb(1, sizeof(Map));
                memfread(&typek, 1, fp);
                memfread(&typev, 1, fp);
                memfread(&size, 4, fp);
                memfread(&count, 4, fp);
                tmpmap->itemsize = count;
                tmpmap->keyType = Uint8ToType(typek);
                tmpmap->valueType = Uint8ToType(typev);
                tmpmap->items = (Pair**)callocb(count, sizeof(Pair*));
                for (uint32_t i = 0; i < count; i++)
                {
                    Pair* pairtmp = (Pair*)callocb(1, sizeof(Pair));
                    pairtmp->key = ReadValueByType(tmpmap->keyType, hasht, fp);
                    pairtmp->value = ReadValueByType(tmpmap->valueType, hasht, fp);
                    tmpmap->items[i] = pairtmp;
                }
                result->data = tmpmap;
                break;
            }
        }
    }
    return result;
}

uint32_t GetSize(BinField* value)
{
    uint32_t size = Type_size[value->type];
    if (size == 0)
    {
        switch (value->type)
        {
            case STRING:
                size = 2 + (uint32_t)strlen((char*)value->data);
                break;
            case STRUCT:
            case CONTAINER:
            {
                size = 1 + 4 + 4;
                ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
                for (uint32_t i = 0; i < cs->itemsize; i++)
                    size += GetSize(cs->items[i]->value);
                break;
            }
            case POINTER:
            case EMBEDDED:
            {
                size = 4;
                PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
                if (pe->name != 0)
                {
                    size += 4 + 2;
                    for (uint16_t i = 0; i < pe->itemsize; i++)
                        size += GetSize(pe->items[i]->value) + 4 + 1;
                }
                break;
            }
            case OPTION:
            {
                size = 2;
                ContainerOrStructOrOption* op = (ContainerOrStructOrOption*)value->data;
                for (uint8_t i = 0; i < op->itemsize; i++)
                    size += GetSize(op->items[i]->value);
                break;
            }
            case MAP:
            {
                size = 1 + 1 + 4 + 4;
                Map* map = (Map*)value->data;
                for (uint32_t i = 0; i < map->itemsize; i++)
                    size += GetSize(map->items[i]->key) + GetSize(map->items[i]->value);
                break;
            }
        }
    }
    return size;
}

void WriteValueByBin(BinField* value, charv* str)
{
    int size = Type_size[value->type];
    if (size != 0)
    {
        memfwrite(value->data, size, str);
    }
    else
    {
        switch (value->type)
        {
            case STRING:
            {
                char* string = (char*)value->data;
                uint16_t size = (uint16_t)strlen(string);
                memfwrite(&size, 2, str);
                memfwrite(string, size, str);
                break;
            }
            case STRUCT:
            case CONTAINER:
            {
                uint32_t size = 4, realsize = 0;
                ContainerOrStructOrOption* cs = (ContainerOrStructOrOption*)value->data;
                for (uint32_t i = 0; i < cs->itemsize; i++)
                {
                    if (cs->items[i]->value != NULL)
                    {
                        realsize++;
                        size += GetSize(cs->items[i]->value);
                    }
                }
                uint8_t type = TypeToUint8(cs->valueType);
                memfwrite(&type, 1, str);
                memfwrite(&size, 4, str);
                memfwrite(&realsize, 4, str);
                for (uint32_t i = 0; i < cs->itemsize; i++)
                    if (cs->items[i]->value != NULL)
                        WriteValueByBin(cs->items[i]->value, str);
                break;
            }
            case POINTER:
            case EMBEDDED:
            {
                uint32_t size = 2;
                uint16_t realsize = 0;
                PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
                memfwrite(&pe->name, 4, str);
                if (pe->name == 0)
                    break;
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        realsize++;
                        size += GetSize(pe->items[i]->value) + 4 + 1;
                    }
                }
                memfwrite(&size, 4, str);
                memfwrite(&realsize, 2, str);
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    if (pe->items[i]->value != NULL)
                    {
                        uint8_t type = TypeToUint8(pe->items[i]->value->type);
                        memfwrite(&pe->items[i]->key, 4, str);
                        memfwrite(&type, 1, str);
                        WriteValueByBin(pe->items[i]->value, str);
                    }
                }
                break;
            }
            case OPTION:
            {
                uint8_t realsize = 0;
                ContainerOrStructOrOption* op = (ContainerOrStructOrOption*)value->data;
                for (uint8_t i = 0; i < op->itemsize; i++)
                    if (op->items[i]->value != NULL)
                        realsize++;
                uint8_t type = TypeToUint8(op->valueType);
                memfwrite(&type, 1, str);
                memfwrite(&realsize, 1, str);
                for (uint8_t i = 0; i < op->itemsize; i++)
                    if (op->items[i]->value != NULL)
                        WriteValueByBin(op->items[i]->value, str);
                break;
            }
            case MAP:
            {
                Map* map = (Map*)value->data;
                uint32_t size = 4, realsize = 0;
                for (uint32_t i = 0; i < map->itemsize; i++)
                {
                    if (map->items[i]->key != NULL)
                    {
                        realsize++;
                        size += GetSize(map->items[i]->key) + GetSize(map->items[i]->value);
                    }
                }
                uint8_t typek = TypeToUint8(map->keyType);
                uint8_t typev = TypeToUint8(map->valueType);
                memfwrite(&typek, 1, str);
                memfwrite(&typev, 1, str);
                memfwrite(&size, 4, str);
                memfwrite(&realsize, 4, str);
                for (uint32_t i = 0; i < map->itemsize; i++)
                {
                    if (map->items[i]->key != NULL)
                    {
                        WriteValueByBin(map->items[i]->key, str);
                        WriteValueByBin(map->items[i]->value, str);
                    }
                }
                break;
            }
        }
    }
}

typedef struct PacketId
{
    char* str;
    uintptr_t id;
} PacketId;

typedef struct PacketBin
{
    bool isprop;
    uint32_t Version;
    uint32_t linkedsize; 
    PacketId** LinkedList;
    BinField* entriesMap;
} PacketBin;

PacketBin* DecodeBin(char* filepath, HashTable* hasht)
{
    FILE* file;
    errno_t err = fopen_s(&file, filepath, "rb");
    if (err)
    {
        char* errmsg = (char*)callocb(255, 1);
        strerror_s(errmsg, 255, err);
        printf("ERROR: cannot read file %s %s.\n", filepath, errmsg);
        free(errmsg);
        return NULL;
    }

    printf("reading file: %s.\n", filepath);
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* fp = (char*)callocb(1, fsize + 1);
    myassert(fread(fp, fsize, 1, file) == NULL);
    fp[fsize] = '\0';
    fclose(file);

    PacketBin* packet = (PacketBin*)callocb(1, sizeof(PacketBin));

    uint32_t Signature = 0;
    memfread(&Signature, 4, &fp);
    if (memcmp(&Signature, "PTCH", 4) == 0)
    {
        fp += 8;
        memfread(&Signature, 4, &fp);
        if (memcmp(&Signature, "PROP", 4) != 0)
        {
            printf("bin has no valid signature\n");
            return NULL;
        }
        packet->isprop = false;
    }
    if (memcmp(&Signature, "PROP", 4) != 0)
    {
        printf("bin has no valid signature\n");
        return NULL;
    }
    else
        packet->isprop = true;

    uint32_t Version = 0;
    memfread(&Version, 4, &fp);
    packet->Version = Version;

    uint32_t linkedFilesCount = 0;
    PacketId** LinkedList = NULL;
    if (Version >= 2)
    {
        uint16_t stringlength = 0;
        memfread(&linkedFilesCount, 4, &fp);
        if (linkedFilesCount > 0)
        {
            PacketId** LinkedListt = (PacketId**)callocb(linkedFilesCount, sizeof(PacketId*));
            for (uint32_t i = 0; i < linkedFilesCount; i++) {
                memfread(&stringlength, 2, &fp);
                LinkedListt[i] = (PacketId*)callocb(1, sizeof(PacketId));
                LinkedListt[i]->str = (char*)callocb(stringlength + 1, 1);
                memfread(LinkedListt[i]->str, (size_t)stringlength, &fp);
                LinkedListt[i]->str[stringlength] = '\0';
            }
            LinkedList = LinkedListt;
        }
    }
    packet->LinkedList = LinkedList;
    packet->linkedsize = linkedFilesCount;

    uint32_t entryCount = 0;
    memfread(&entryCount, 4, &fp);

    uint32_t* entryTypes = (uint32_t*)callocb(entryCount, 4);
    memfread(entryTypes, entryCount * 4, &fp);

    Map* entriesMap = (Map*)callocb(1, sizeof(Map));
    entriesMap->keyType = HASH;
    entriesMap->valueType = EMBEDDED;
    entriesMap->itemsize = entryCount;
    entriesMap->items = (Pair**)callocb(entryCount, sizeof(Pair*));
    for (size_t i = 0; i < entryCount; i++)
    {
        uint32_t entryLength = 0;
        memfread(&entryLength, 4, &fp);

        uint32_t entryKeyHash = 0;
        memfread(&entryKeyHash, 4, &fp);

        uint16_t fieldcount = 0;
        memfread(&fieldcount, 2, &fp);

        PointerOrEmbed* entry = (PointerOrEmbed*)callocb(1, sizeof(PointerOrEmbed));
        entry->itemsize = fieldcount;
        entry->name = entryTypes[i];
        entry->items = (Field**)callocb(fieldcount, sizeof(Field*));
        for (uint16_t o = 0; o < fieldcount; o++)
        {
            uint32_t name = 0;
            memfread(&name, 4, &fp);

            uint8_t type = 0;
            memfread(&type, 1, &fp);

            Field* fieldtmp = (Field*)callocb(1, sizeof(Field));
            fieldtmp->key = name;
            fieldtmp->value = ReadValueByType(type, hasht, &fp);
            entry->items[o] = fieldtmp;
        }

        void* ptr = callocb(1, sizeof(uint32_t));
        *((uint32_t*)ptr) = entryKeyHash;
        BinField* hash = (BinField*)callocb(1, sizeof(BinField));
        hash->type = HASH;
        hash->data = ptr;

        BinField* entrye = (BinField*)callocb(1, sizeof(BinField));
        entrye->type = EMBEDDED;
        entrye->data = entry;

        Pair* pairtmp = (Pair*)callocb(1, sizeof(Pair));
        pairtmp->key = hash;
        pairtmp->value = entrye;
        entriesMap->items[i] = pairtmp;
    }

    BinField* entriesMapbin = (BinField*)callocb(1, sizeof(BinField));
    entriesMapbin->type = MAP;
    entriesMapbin->data = entriesMap;
    packet->entriesMap = entriesMapbin;
    freeb(fp-fsize);

    printf("finised reading file.\n");
    return packet;
}

int EncodeBin(char* filepath, PacketBin* packet)
{
    printf("creating bin file: %s.\n", filepath);
    charv* str = (charv*)callocb(1, sizeof(charv));
    if (packet->isprop == false)
    {
        uint32_t unk1 = 1, unk2 = 0;
        memfwrite((void*)"PTCH", 4, str);
        memfwrite(&unk1, 4, str);
        memfwrite(&unk2, 4, str);
    }
    memfwrite((void*)"PROP", 4, str);
    memfwrite(&packet->Version, 4, str);

    if (packet->Version >= 2)
    {
        memfwrite(&packet->linkedsize, 4, str);
        for (uint32_t i = 0; i < packet->linkedsize; i++)
        {
            uint16_t len = (uint16_t)strlen(packet->LinkedList[i]->str);
            memfwrite(&len, 2, str);
            memfwrite(packet->LinkedList[i]->str, len, str);
        }
    }

    uint32_t realsize = 0;
    Map* entriesMap = (Map*)packet->entriesMap->data;

    for (uint32_t i = 0; i < entriesMap->itemsize; i++)
        if (entriesMap->items[i]->key != NULL)
            realsize++;
    memfwrite(&realsize, 4, str);

    for (uint32_t i = 0; i < entriesMap->itemsize; i++)
        if (entriesMap->items[i]->key != NULL)
            memfwrite(&((PointerOrEmbed*)entriesMap->items[i]->value->data)->name, 4, str);

    for (uint32_t i = 0; i < entriesMap->itemsize; i++)
    {
        realsize = 0;
        uint32_t entryLength = 4 + 2;
        uint32_t entryKeyHash = *(uint32_t*)entriesMap->items[i]->key->data;
        PointerOrEmbed* pe = (PointerOrEmbed*)entriesMap->items[i]->value->data;

        for (uint16_t k = 0; k < pe->itemsize; k++)
        {
            if (pe->items[k]->value != NULL)
            {
                realsize++;
                entryLength += GetSize(pe->items[k]->value) + 4 + 1;
            }
        }

        memfwrite(&entryLength, 4, str);
        memfwrite(&entryKeyHash, 4, str);
        memfwrite(&realsize, 2, str);
        for (uint16_t k = 0; k < pe->itemsize; k++)
        {
            if (pe->items[k]->value != NULL)
            {
                uint32_t name = pe->items[k]->key;
                uint8_t type = TypeToUint8(pe->items[k]->value->type);
                memfwrite(&name, 4, str);
                memfwrite(&type, 1, str);
                WriteValueByBin(pe->items[k]->value, str);
            }
        }
    }

    printf("finised creating bin file.\n");
    printf("writing to file.\n");
    FILE* file;
    errno_t err = fopen_s(&file, filepath, "wb");
    if (err)
    {
        char* errmsg = (char*)callocb(255, 1);
        strerror_s(errmsg, 255, err);
        printf("ERROR: cannot write file %s %s.\n", filepath, errmsg);
        free(errmsg);
        return 1;
    }

    fwrite(str->data, str->lenght, 1, file);
    printf("finised writing to file.\n");
    fclose(file);
    freeb(str->data);

    return 0;
}

#pragma endregion