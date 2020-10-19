//author https://github.com/autergame
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

void _assert(char const* msg, char const* file, unsigned line)
{
    printf("ERROR: %s %s %d\n", msg, file, line);
    scanf("press enter to exit.");
    exit(1);
}
#define myassert(expression) if (expression) { _assert(#expression, __FILE__, __LINE__); }

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

struct node
{
    uint32_t key;
    char* value;
    node* next;
};

typedef struct HashTable
{
    uint32_t size;
    node** list;
} HashTable;

HashTable* createHashTable(uint32_t size)
{
    HashTable* t = (HashTable*)malloc(sizeof(HashTable));
    myassert(t == NULL);
    t->size = size;
    t->list = (node**)calloc(size, sizeof(node**));
    myassert(t->list == NULL);
    return t;
}

void insertHashTable(HashTable* t, uint32_t key, char* val)
{
    uint32_t pos = key % t->size;
    node* list = t->list[pos];
    node* newNode = (node*)malloc(sizeof(node));
    myassert(newNode == NULL);
    node* temp = list;
    while (temp) {
        if (temp->key == key) {
            temp->value = val;
            return;
        }
        temp = temp->next;
    }
    newNode->key = key;
    newNode->value = val;
    newNode->next = list;
    t->list[pos] = newNode;
}

char* lookupHashTable(HashTable* t, uint32_t key)
{
    node* list = t->list[key % t->size];
    node* temp = list;
    while (temp) {
        if (temp->key == key) {
            return temp->value;
        }
        temp = temp->next;
    }
    return NULL;
}

int addhash(HashTable* map, const char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (!file)
    {
        printf("ERROR: cannot read file \"%s\".\n", filename);
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* fp = (char*)malloc(fsize + 1);
    myassert(fp == NULL);
    myassert(fread(fp, fsize, 1, file) == NULL);
    fp[fsize] = '\0';
    fclose(file);
    char* hashend;
    char* hashline = strtok(fp, "\n");
    while (hashline != NULL) {
        uint32_t key = strtoul(hashline, &hashend, 16);
        insertHashTable(map, key, hashend + 1);
        hashline = strtok(NULL, "\n");
    }
    return 0;
}

typedef struct charv
{
    char* data;
    size_t lenght;
} charv;

void memfwrite(void* buf, size_t bytes, charv* membuf)
{
    char* oblock = (char*)realloc(membuf->data, membuf->lenght + bytes);
    myassert(oblock == NULL);
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

static const char* Type_strings[] = {
    "None", "Bool", "SInt8", "UInt8", "SInt16", "UInt16", "SInt32", "UInt32", "SInt64",
    "UInt64", "Float32", "Vector2", "Vector3", "Vector4", "Matrix4x4", "RGBA", "String",
    "Hash", "WadEntryLink", "Container", "Struct", "Pointer", "Embedded", "Link", "Option", "Map", "Flag"
};

static uint8_t flag = 128;
Type uinttotype(uint8_t type)
{
    if ((type & flag) == flag)
        type = (type - flag) + CONTAINER;
    return (Type)type;
}
uint8_t typetouint(Type type)
{
    uint8_t raw = type;
    if (raw >= CONTAINER)
        raw = (raw - CONTAINER) + flag;
    return raw;
}

typedef struct BinField
{
    uintptr_t id;
    Type typebin;
    void* data;
} BinField;

typedef struct Pair
{
    BinField* key;
    BinField* value;
} Pair;

typedef struct Field
{
    uint32_t key;
    BinField* value;
} Field;

typedef struct ContainerOrStruct
{
    Type valueType;
    BinField** items;
    uint32_t itemsize;
} ContainerOrStruct;

typedef struct PointerOrEmbed
{
    uint32_t name;
    Field** items;
    uint16_t itemsize;
} PointerOrEmbed;

typedef struct Option
{
    uint8_t count;
    Type valueType;
    BinField** items;
} Option;

typedef struct Map
{
    Type keyType;
    Type valueType;
    Pair** items;
    uint32_t itemsize;
} Map;

char* hashtostr(HashTable* hasht, uint32_t value)
{
    char* strvalue = lookupHashTable(hasht, value);
    if (strvalue == NULL)
    {
        strvalue = (char*)calloc(11, 1);
        myassert(strvalue == NULL);
        sprintf(strvalue, "0x%08" PRIX32, value);
    }
    return strvalue;
}

uint32_t hashfromstring(char* string)
{
    uint32_t hash = 0;
    if (string[0] == '0' && string[1] == 'x')
        hash = strtoul(string, NULL, 16);
    else
        hash = FNV1Hash(string);
    return hash;
}

uint64_t hashfromstringxx(char* string)
{
    uint64_t hash = 0;
    if (string[0] == '0' && string[1] == 'x')
        hash = strtoull(string, NULL, 16);
    else
        hash = XXHash((const uint8_t*)string, strlen(string));
    return hash;
}

static int MyResizeCallback(ImGuiInputTextCallbackData* data)
{
    if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
    {
        char** string = (char**)data->UserData;
        char* newdata = (char*)calloc(1, data->BufSize);
        myassert(newdata == NULL);
        myassert(memcpy(newdata, *string, strlen(*string)) == NULL);
        free(*string); *string = newdata;
        data->Buf = *string;
    }
    return 0;
}

char* inputtext(const char* inner, uintptr_t id)
{
    ImGui::PushID((void*)id);
    size_t size = strlen(inner) + 1;
    char* string = (char*)calloc(size, 1);
    myassert(string == NULL);
    myassert(memcpy(string, inner, size) == NULL);
    char** prot = (char**)calloc(1, sizeof(char*)); 
    myassert(prot == NULL); *prot = string;
    ImGui::SetNextItemWidth(ImGui::CalcTextSize(string, NULL, true).x + GImGui->Style.FramePadding.x * 8.f);
    bool ret = ImGui::InputText("", string, size, ImGuiInputTextFlags_CallbackResize, MyResizeCallback, (void*)prot);
    ImGui::PopID();
    if(ret)
        if (ImGui::IsKeyPressedMap(ImGuiKey_Enter) || GImGui->IO.MouseClicked[0])
            return *prot;
    free(*prot);
    free(prot);
    return NULL;
}

void inputtextmod(HashTable* hasht, uint32_t* hash, uintptr_t id)
{
    char* string = inputtext(hashtostr(hasht, *hash), id);
    if (string != NULL)
    {
        *hash = hashfromstring(string);
        free(string);
    }
}

void getarraycount(BinField* value)
{
    switch (value->typebin)
    {
        case CONTAINER:
        case STRUCT:
        {
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            ImGui::Text("Count %d", cs->itemsize);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            ImGui::Text("Count %d", pe->itemsize);
            break;
        }
        case OPTION:
        {
            Option* op = (Option*)value->data;
            ImGui::Text("Count %d", op->count);
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            ImGui::Text("Count %d", mp->itemsize);
            break;
        }
    }
}

void getarraytype(BinField* value)
{
    switch (value->typebin)
    {
        case CONTAINER:
        case STRUCT:
        {
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            ImGui::SameLine(0, 1); ImGui::Text("[%s]", Type_strings[cs->valueType]);
            break;
        }
        case OPTION:
        {
            Option* op = (Option*)value->data;
            ImGui::SameLine(0, 1); ImGui::Text("[%s]", Type_strings[op->valueType]);
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            ImGui::SameLine(0, 1); ImGui::Text("[%s,%s]",
                Type_strings[mp->keyType], Type_strings[mp->valueType]);
            break;
        }
    }
}

void cleanbin(BinField* value)
{
    switch (value->typebin)
    {
        case STRUCT:
        case CONTAINER:
        {
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
                cleanbin(cs->items[i]);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            if (pe->name != 0)
            {
                for (uint16_t i = 0; i < pe->itemsize; i++)
                    cleanbin(pe->items[i]->value);
            }
            break;
        }
        case OPTION:
        {
            Option* op = (Option*)value->data;
            for (uint8_t i = 0; i < op->count; i++)
                cleanbin(op->items[i]);
            break;
        }
        case MAP:
        {
            Map* map = (Map*)value->data;
            for (uint32_t i = 0; i < map->itemsize; i++)
            {
                cleanbin(map->items[i]->key);
                cleanbin(map->items[i]->value);
            }
            break;
        }
    }
    free(value);
}

void getstructidbin(BinField* value, uintptr_t* tree)
{
    *tree += 1;
    value->id = *tree;
    switch (value->typebin)
    {
        case VEC2:
            *tree += 1;
            break;
        case VEC3:
            *tree += 2;
            break;
        case VEC4:
        case RGBA:
            *tree += 3;
            break;
        case MTX44:
            *tree += 15;
            break;
        case STRUCT:
        case CONTAINER:
        {        
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
                getstructidbin(cs->items[i], tree);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            *tree += 1;
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            for (uint16_t i = 0; i < pe->itemsize; i++)
            {
                *tree += 1;
                Type typi = pe->items[i]->value->typebin;
                if ((typi >= CONTAINER && typi <= EMBEDDED) || typi == OPTION || typi == MAP)
                    *tree += 1;
                getstructidbin(pe->items[i]->value, tree);
            }           
            break;
        }
        case OPTION:
        {
            Option* op = (Option*)value->data;
            for (uint8_t i = 0; i < op->count; i++)
                getstructidbin(op->items[i], tree);
            break;
        }
        case MAP:
        {
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {
                getstructidbin(mp->items[i]->key, tree);
                getstructidbin(mp->items[i]->value, tree);
            }
            break;
        }
    }
}

void getvaluefromtype(BinField* value, HashTable* hasht)
{
    switch (value->typebin)
    {
        case NONE:
            ImGui::Text("NULL");
            break;
        case FLAG:
        case BOOLB:
        {
            char* string = inputtext(*(uint8_t*)value->data == 1 ? "true" : "false", value->id);
            if (string != NULL)
            {
                for (int i = 0; string[i]; i++)
                    string[i] = tolower(string[i]);
                if (strcmp(string, "true") == 0)
                    *(uint8_t*)value->data = 1;
                else
                    *(uint8_t*)value->data = 0;
            }
            free(string);
            break;
        }
        case SInt8:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIi8, *(int8_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIi8, (int8_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case UInt8:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIu8, *(uint8_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIu8, (uint8_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case SInt16:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIi16, *(int16_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIi16, (int16_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case UInt16:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIu16, *(uint16_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIu16, (uint16_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case SInt32:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIi32, *(int32_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIi32, (int32_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case UInt32:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIu32, *(uint32_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIu32, (uint32_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case SInt64:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIi64, *(int64_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIi64, (int64_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case UInt64:
        {
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            sprintf(buf, "%" PRIu64, *(uint64_t*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%" PRIu64, (uint64_t*)value->data);
            free(string);
            free(buf);
            break;
        }
        case Float32:
        {
            uint8_t havepoint = 0;
            char* buf = (char*)calloc(64, 1);
            myassert(buf == NULL);
            int length = sprintf(buf, "%.9g", *(float*)value->data);
            for (int i = 0; i < length; i++)
                if (buf[i] == '.')
                    havepoint = 1;
            if (havepoint == 0)
                length = sprintf(buf, "%.9g.0", *(float*)value->data);
            char* string = inputtext(buf, value->id);
            if (string != NULL)
                sscanf(string, "%g", (float*)value->data);
            free(string);
            free(buf);
            break;
        }
        case VEC2:
        {
            float* arr = (float*)value->data;
            for (int i = 0; i < 2; i++)
            {
                ImGui::SameLine();
                uint8_t havepoint = 0;
                char* buf = (char*)calloc(64, 1);
                myassert(buf == NULL);
                int length = sprintf(buf, "%.9g", arr[i]);
                for (int k = 0; k < length; k++)
                    if (buf[k] == '.')
                        havepoint = 1;
                if (havepoint == 0)
                    length = sprintf(buf, "%.9g.0", arr[i]);
                char* string = inputtext(buf, value->id+i);
                if (string != NULL)
                    sscanf(string, "%g", &arr[i]);
                free(string);
                free(buf);
            }
            break;
        }
        case VEC3:
        {
            float* arr = (float*)value->data;
            for (int i = 0; i < 3; i++)
            {
                ImGui::SameLine();
                uint8_t havepoint = 0;
                char* buf = (char*)calloc(64, 1);
                myassert(buf == NULL);
                int length = sprintf(buf, "%.9g", arr[i]);
                for (int k = 0; k < length; k++)
                    if (buf[k] == '.')
                        havepoint = 1;
                if (havepoint == 0)
                    length = sprintf(buf, "%.9g.0", arr[i]);
                char* string = inputtext(buf, value->id+i);
                if (string != NULL)
                    sscanf(string, "%g", &arr[i]);
                free(string);
                free(buf);
            }
            break;
        }
        case VEC4:
        {
            float* arr = (float*)value->data;
            for (int i = 0; i < 4; i++)
            {
                ImGui::SameLine();
                uint8_t havepoint = 0;
                char* buf = (char*)calloc(64, 1);
                myassert(buf == NULL);
                int length = sprintf(buf, "%.9g", arr[i]);
                for (int k = 0; k < length; k++)
                    if (buf[k] == '.')
                        havepoint = 1;
                if (havepoint == 0)
                    length = sprintf(buf, "%.9g.0", arr[i]);
                char* string = inputtext(buf, value->id+i);
                if (string != NULL)
                    sscanf(string, "%g", &arr[i]);
                free(string);
                free(buf);
            }
            break;
        }
        case MTX44:
        {
            float* arr = (float*)value->data;
            for (int i = 0; i < 16; i++)
            {
                ImGui::SameLine();
                uint8_t havepoint = 0;
                char* buf = (char*)calloc(64, 1);
                myassert(buf == NULL);
                int length = sprintf(buf, "%.9g", arr[i]);
                for (int k = 0; k < length; k++)
                    if (buf[k] == '.')
                        havepoint = 1;
                if (havepoint == 0)
                    length = sprintf(buf, "%.9g.0", arr[i]);
                char* string = inputtext(buf, value->id+i);
                if (string != NULL)
                    sscanf(string, "%g", &arr[i]);
                free(string);
                free(buf);
            }
            break;
        }
        case RGBA:
        {
            uint8_t* arr = (uint8_t*)value->data;
            for (int i = 0; i < 4; i++)
            {
                ImGui::SameLine();
                char* buf = (char*)calloc(64, 1);
                myassert(buf == NULL);
                sprintf(buf, "%" PRIu8, arr[i]);
                char* string = inputtext(buf, value->id+i);
                if (string != NULL)
                    sscanf(string, "%" PRIu8, &arr[i]);
                free(string);
                free(buf);
            }
            break;
        }
        case STRING:
        {
            char* string = inputtext((char*)value->data, value->id);
            if (string != NULL)
            {
                free(value->data);
                value->data = string;
            }
            break;
        }
        case HASH:
        case LINK:
        {
            char* string = inputtext(hashtostr(hasht, *(uint32_t*)value->data), value->id);
            if (string != NULL)
            {
                uint32_t data = hashfromstring(string);
                value->data = &data;
                free(string);
            }
            break;
        }
        case WADENTRYLINK:
        {
            char* strvalue = (char*)calloc(10, 1);
            myassert(strvalue == NULL);
            sprintf(strvalue, "0x%016" PRIX64, *(uint64_t*)value->data);
            char* string = inputtext(strvalue, value->id);
            if (string != NULL)
            {
                uint64_t data = hashfromstringxx(string);
                value->data = &data;
                free(string);
            }
            free(strvalue);
            break;
        }
        case CONTAINER:
        case STRUCT:
        {
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
                getvaluefromtype(cs->items[i], hasht);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            ImGui::AlignTextToFramePadding();
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            bool treeopen = ImGui::TreeNodeEx((void*)value->id,
                ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth, "");
            ImGui::SameLine(); inputtextmod(hasht, &pe->name, value->id+1);
            ImGui::SameLine(); getarraycount(value);
            if (treeopen)
            {            
                for (uint16_t i = 0; i < pe->itemsize; i++)
                {
                    ImGui::Indent();
                    Type typi = pe->items[i]->value->typebin;
                    if ((typi >= CONTAINER && typi <= EMBEDDED) || typi == OPTION || typi == MAP)
                    {        
                        ImGui::AlignTextToFramePadding();
                        bool treeopene = ImGui::TreeNodeEx((void*)(pe->items[i]->value->id-2),
                            ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_SpanAvailWidth, "");
                        ImGui::SameLine(); inputtextmod(hasht, &pe->items[i]->key, pe->items[i]->value->id-1);
                        ImGui::SameLine(); ImGui::Text(": %s", Type_strings[typi]); getarraytype(pe->items[i]->value);
                        ImGui::SameLine(); getarraycount(pe->items[i]->value);
                        if (treeopene)
                        {
                            ImGui::Indent();
                            getvaluefromtype(pe->items[i]->value, hasht);
                            ImGui::TreePop();
                            ImGui::Unindent();
                        }
                    }
                    else
                    {
                        inputtextmod(hasht, &pe->items[i]->key, pe->items[i]->value->id-1);
                        ImGui::SameLine(); ImGui::Text(": %s", Type_strings[typi]);
                        ImGui::SameLine(); ImGui::Text("="); ImGui::SameLine();
                        getvaluefromtype(pe->items[i]->value, hasht);
                    }
                    ImGui::Unindent();
                }
                ImGui::TreePop();
            }
            break;
        }
        case OPTION:
        {
            Option* op = (Option*)value->data;
            for (uint8_t i = 0; i < op->count; i++)
                getvaluefromtype(op->items[i], hasht);
            break;
        }
        case MAP:
        {   
            Map* mp = (Map*)value->data;
            for (uint32_t i = 0; i < mp->itemsize; i++)
            {         
                getvaluefromtype(mp->items[i]->key, hasht);
                ImGui::SameLine(); ImGui::Text(":"); ImGui::SameLine();
                getvaluefromtype(mp->items[i]->value, hasht);
            }           
            break;
        }
    }
}

BinField* readvaluebytype(uint8_t typeidbin, HashTable* hasht, char** fp)
{
    BinField* result = (BinField*)calloc(1, sizeof(BinField));
    myassert(result == NULL);
    result->typebin = uinttotype(typeidbin);
    switch (result->typebin)
    {
        case FLAG:
        case BOOLB:
        case SInt8:
        case UInt8:
        {
            uint8_t* data = (uint8_t*)calloc(1, 1);
            myassert(data == NULL);
            memfread(data, 1, fp);
            result->data = data;
            break;
        }
        case SInt16:
        case UInt16:
        {
            uint16_t* data = (uint16_t*)calloc(1, 2);
            myassert(data == NULL);
            memfread(data, 2, fp);
            result->data = data;
            break;
        }
        case LINK:
        case HASH:
        case RGBA:
        case SInt32:
        case UInt32:
        case Float32:
        {
            uint32_t* data = (uint32_t*)calloc(1, 4);
            myassert(data == NULL);
            memfread(data, 4, fp);
            result->data = data;
            break;
        }
        case VEC2:
        case SInt64:
        case UInt64:
        case WADENTRYLINK:
        {
            uint64_t* data = (uint64_t*)calloc(1, 8);
            myassert(data == NULL);
            memfread(data, 8, fp);
            result->data = data;
            break;
        }
        case VEC3:
        {
            float* data = (float*)calloc(3, 4);
            myassert(data == NULL);
            memfread(data, 4 * 3, fp);
            result->data = data;
            break;
        }
        case VEC4:
        {
            float* data = (float*)calloc(4, 4);
            myassert(data == NULL);
            memfread(data, 4 * 4, fp);
            result->data = data;
            break;
        }
        case MTX44:
        {
            float* data = (float*)calloc(16, 4);
            myassert(data == NULL);
            memfread(data, 4 * 16, fp);
            result->data = data;
            break;
        }
        case STRING:
        {
            uint16_t stringlength = 0;
            memfread(&stringlength, 2, fp);
            char* stringb = (char*)calloc(stringlength + 1, 1);
            myassert(stringb == NULL);
            memfread(stringb, (size_t)stringlength, fp);
            stringb[stringlength] = '\0';
            result->data = stringb;
            insertHashTable(hasht, FNV1Hash(stringb), stringb);
            break;
        }
        case STRUCT:
        case CONTAINER:
        {
            uint8_t type = 0;
            uint32_t size = 0;
            uint32_t count = 0;
            ContainerOrStruct* tmpcs = (ContainerOrStruct*)calloc(1, sizeof(ContainerOrStruct));
            myassert(tmpcs == NULL);
            memfread(&type, 1, fp);
            memfread(&size, 4, fp);
            memfread(&count, 4, fp);
            tmpcs->itemsize = count;
            tmpcs->valueType = uinttotype(type);
            tmpcs->items = (BinField**)calloc(count, sizeof(BinField**));
            myassert(tmpcs->items == NULL);
            for (uint32_t i = 0; i < count; i++)
                tmpcs->items[i] = readvaluebytype(tmpcs->valueType, hasht, fp);
            result->data = tmpcs;
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            uint32_t size = 0;
            uint16_t count = 0;
            PointerOrEmbed* tmppe = (PointerOrEmbed*)calloc(1, sizeof(PointerOrEmbed));
            myassert(tmppe == NULL);
            memfread(&tmppe->name, 4, fp);
            if (tmppe->name == 0)
            {
                result->data = tmppe;
                break;
            }
            memfread(&size, 4, fp);
            memfread(&count, 2, fp);
            tmppe->itemsize = count;
            tmppe->items = (Field**)calloc(count, sizeof(Field**));
            myassert(tmppe->items == NULL);
            for (uint16_t i = 0; i < count; i++)
            {
                uint8_t type = 0;
                Field* tmpfield = (Field*)calloc(1, sizeof(Field));
                myassert(tmpfield == NULL);
                memfread(&tmpfield->key, 4, fp);
                memfread(&type, 1, fp);
                tmpfield->value = readvaluebytype(type, hasht, fp);
                tmppe->items[i] = tmpfield;
            }
            result->data = tmppe;
            break;
        }
        case OPTION:
        {
            uint8_t type = 0;
            uint8_t count = 0;
            Option* tmpo = (Option*)calloc(1, sizeof(Option));
            myassert(tmpo == NULL);
            memfread(&type, 1, fp);
            memfread(&count, 1, fp);
            tmpo->count = count;
            tmpo->valueType = uinttotype(type);
            tmpo->items = (BinField**)calloc(count, sizeof(BinField**));
            myassert(tmpo->items == NULL);
            for (uint8_t i = 0; i < count; i++)
                tmpo->items[i] = readvaluebytype(tmpo->valueType, hasht, fp);
            result->data = tmpo;
            break;
        }
        case MAP:
        {
            uint32_t size = 0;
            uint8_t typek = 0;
            uint8_t typev = 0;
            uint32_t count = 0;
            Map* tmpmap = (Map*)calloc(1, sizeof(Map));
            myassert(tmpmap == NULL);
            memfread(&typek, 1, fp);
            memfread(&typev, 1, fp);
            memfread(&size, 4, fp);
            memfread(&count, 4, fp);
            tmpmap->itemsize = count;
            tmpmap->keyType = uinttotype(typek);
            tmpmap->valueType = uinttotype(typev);
            tmpmap->items = (Pair**)calloc(count, sizeof(Pair**));
            myassert(tmpmap->items == NULL);
            for (uint32_t i = 0; i < count; i++)
            {
                Pair* pairtmp = (Pair*)calloc(1, sizeof(Pair));
                myassert(pairtmp == NULL);
                pairtmp->key = readvaluebytype(tmpmap->keyType, hasht, fp);
                pairtmp->value = readvaluebytype(tmpmap->valueType, hasht, fp);
                tmpmap->items[i] = pairtmp;
            }
            result->data = tmpmap;
            break;
        }
    }
    return result;
}

uint32_t getsize(BinField* value)
{
    uint32_t size = 0;
    switch (value->typebin)
    {
        case FLAG:
        case BOOLB:
        case SInt8:
        case UInt8:
            size = 1;
            break;
        case SInt16:
        case UInt16:
            size = 2;
            break;
        case LINK:
        case HASH:
        case RGBA:
        case SInt32:
        case UInt32:
        case Float32:
            size = 4;
            break;
        case VEC2:
        case SInt64:
        case UInt64:
        case WADENTRYLINK:
            size = 8;
            break;
        case VEC3:
            size = 12;
            break;
        case VEC4:
            size = 16;
            break;
        case MTX44:
            size = 64;
            break;
        case STRING:
            size = 2 + strlen((char*)value->data);
            break;
        case STRUCT:
        case CONTAINER:
        {
            size = 1 + 4 + 4;
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            for (uint32_t i = 0; i < cs->itemsize; i++)
                size += getsize(cs->items[i]);
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
                    size += getsize(pe->items[i]->value) + 4 + 1;
            }
            break;
        }
        case OPTION:
        {
            size = 2;
            Option* op = (Option*)value->data;
            for (uint8_t i = 0; i < op->count; i++)
                size += getsize(op->items[i]);
            break;
        }
        case MAP:
        {
            size = 1 + 1 + 4 + 4;
            Map* map = (Map*)value->data;
            for (uint32_t i = 0; i < map->itemsize; i++)
                size += getsize(map->items[i]->key) + getsize(map->items[i]->value);
            break;
        }
    }
    return size;
}

void writevaluebybin(BinField* value, charv* str)
{
    switch (value->typebin)
    {
        case FLAG:
        case BOOLB:
        case SInt8:
        case UInt8:
            memfwrite(value->data, 1, str);
            break;
        case SInt16:
        case UInt16:
            memfwrite(value->data, 2, str);
            break;
        case LINK:
        case HASH:
        case RGBA:
        case SInt32:
        case UInt32:
        case Float32:
            memfwrite(value->data, 4, str);
            break;
        case VEC2:
        case SInt64:
        case UInt64:
        case WADENTRYLINK:
            memfwrite(value->data, 8, str);
            break;
        case VEC3:
            memfwrite(value->data, 12, str);
            break;
        case VEC4:
            memfwrite(value->data, 16, str);
            break;
        case MTX44:
            memfwrite(value->data, 64, str);
            break;
        case STRING:
        {
            char* string = (char*)value->data;
            uint16_t size = (uint16_t)strlen(string);
            memfwrite((char*)&size, 2, str);
            memfwrite(string, size, str);
            break;
        }
        case STRUCT:
        case CONTAINER:
        {
            uint32_t size = 4;
            ContainerOrStruct* cs = (ContainerOrStruct*)value->data;
            uint8_t type = typetouint(cs->valueType);
            memfwrite(&type, 1, str);
            for (uint16_t k = 0; k < cs->itemsize; k++)
                size += getsize(cs->items[k]);
            memfwrite((char*)&size, 4, str);
            memfwrite((char*)&cs->itemsize, 4, str);
            for (uint32_t i = 0; i < cs->itemsize; i++)
                writevaluebybin(cs->items[i], str);
            break;
        }
        case POINTER:
        case EMBEDDED:
        {
            uint32_t size = 2;
            PointerOrEmbed* pe = (PointerOrEmbed*)value->data;
            memfwrite((char*)&pe->name, 4, str);
            if (pe->name == 0)
                break;
            for (uint16_t k = 0; k < pe->itemsize; k++)
                size += getsize(pe->items[k]->value) + 4 + 1;
            memfwrite((char*)&size, 4, str);
            memfwrite((char*)&pe->itemsize, 2, str);
            for (uint16_t i = 0; i < pe->itemsize; i++)
            {
                uint8_t type = typetouint(pe->items[i]->value->typebin);
                memfwrite((char*)&pe->items[i]->key, 4, str);
                memfwrite((char*)&type, 1, str);
                writevaluebybin(pe->items[i]->value, str);
            }
            break;
        }
        case OPTION:
        {
            uint8_t count = 1;
            Option* op = (Option*)value->data;
            uint8_t type = typetouint(op->valueType);
            memfwrite(&type, 1, str);
            memfwrite(&op->count, 1, str);
            for (uint8_t i = 0; i < op->count; i++)
                writevaluebybin(op->items[i], str);
            break;
        }
        case MAP:
        {
            uint32_t size = 4;
            Map* map = (Map*)value->data;
            uint8_t typek = typetouint(map->keyType);
            uint8_t typev = typetouint(map->valueType);
            memfwrite((char*)&typek, 1, str);
            memfwrite((char*)&typev, 1, str);
            for (uint16_t k = 0; k < map->itemsize; k++)
                size += getsize(map->items[k]->key) + getsize(map->items[k]->value);
            memfwrite((char*)&size, 4, str);
            memfwrite((char*)&map->itemsize, 4, str);
            for (uint32_t i = 0; i < map->itemsize; i++)
            {
                writevaluebybin(map->items[i]->key, str);
                writevaluebybin(map->items[i]->value, str);
            }
            break;
        }
    }
}

typedef struct PacketBin
{
    bool isprop;
    uint32_t Version;
    uint32_t linkedsize; 
    char** LinkedList;
    BinField* entriesMap;
} PacketBin;

PacketBin* decode(char* filepath, HashTable* hasht)
{
    PacketBin* packet = (PacketBin*)calloc(1, sizeof(PacketBin*));
    myassert(packet == NULL);
    FILE* file = fopen(filepath, "rb");
    if (!file)
    {
        printf("ERROR: cannot read file \"%s\".\n", filepath);
        return NULL;
    }

    printf("reading file.\n");
    fseek(file, 0, SEEK_END);
    long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* fp = (char*)malloc(fsize + 1);
    myassert(fp == NULL);
    myassert(fread(fp, fsize, 1, file) == NULL);
    fp[fsize] = '\0';
    fclose(file);

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
    char** LinkedList = NULL;
    if (Version >= 2)
    {
        uint16_t stringlength = 0;
        memfread(&linkedFilesCount, 4, &fp);
        if (linkedFilesCount > 0)
        {
            char** LinkedListt = (char**)calloc(linkedFilesCount, sizeof(char**));
            myassert(LinkedListt == NULL);
            for (uint32_t i = 0; i < linkedFilesCount; i++) {
                memfread(&stringlength, 2, &fp);
                LinkedListt[i] = (char*)calloc(stringlength + 1, 1);
                myassert(LinkedListt[i] == NULL);
                memfread(LinkedListt[i], (size_t)stringlength, &fp);
                LinkedListt[i][stringlength] = '\0';
            }
            LinkedList = LinkedListt;
        }
    }
    packet->LinkedList = LinkedList;
    packet->linkedsize = linkedFilesCount;

    uint32_t entryCount = 0;
    memfread(&entryCount, 4, &fp);

    uint32_t* entryTypes = (uint32_t*)calloc(entryCount, 4);
    myassert(entryTypes == NULL);
    memfread(entryTypes, entryCount * 4, &fp);

    Map* entriesMap = (Map*)calloc(1, sizeof(Map));
    myassert(entriesMap == NULL);
    entriesMap->keyType = HASH;
    entriesMap->valueType = EMBEDDED;
    entriesMap->itemsize = entryCount;
    entriesMap->items = (Pair**)calloc(entryCount, sizeof(Pair**));
    myassert(entriesMap->items == NULL);
    for (size_t i = 0; i < entryCount; i++)
    {
        uint32_t entryLength = 0;
        memfread(&entryLength, 4, &fp);

        uint32_t entryKeyHash = 0;
        memfread(&entryKeyHash, 4, &fp);

        uint16_t fieldcount = 0;
        memfread(&fieldcount, 2, &fp);

        PointerOrEmbed* entry = (PointerOrEmbed*)calloc(1, sizeof(PointerOrEmbed));
        myassert(entry == NULL);
        entry->itemsize = fieldcount;
        entry->name = entryTypes[i];
        entry->items = (Field**)calloc(fieldcount, sizeof(Field**));
        myassert(entry->items == NULL);
        for (uint16_t o = 0; o < fieldcount; o++)
        {
            uint32_t name = 0;
            memfread(&name, 4, &fp);

            uint8_t type = 0;
            memfread(&type, 1, &fp);

            Field* fieldtmp = (Field*)calloc(1, sizeof(Field));
            myassert(fieldtmp == NULL);
            fieldtmp->key = name;
            fieldtmp->value = readvaluebytype(type, hasht, &fp);
            entry->items[o] = fieldtmp;
        }

        void* ptr = calloc(1, sizeof(uint32_t));
        myassert(ptr == NULL);
        *((uint32_t*)ptr) = entryKeyHash;
        BinField* hash = (BinField*)calloc(1, sizeof(BinField));
        myassert(hash == NULL);
        hash->typebin = HASH;
        hash->data = ptr;

        BinField* entrye = (BinField*)calloc(1, sizeof(BinField));
        myassert(entrye == NULL);
        entrye->typebin = EMBEDDED;
        entrye->data = entry;

        Pair* pairtmp = (Pair*)calloc(1, sizeof(Pair));
        myassert(pairtmp == NULL);
        pairtmp->key = hash;
        pairtmp->value = entrye;
        entriesMap->items[i] = pairtmp;
    }

    BinField* entriesMapbin = (BinField*)calloc(1, sizeof(BinField));
    myassert(entriesMapbin == NULL);
    entriesMapbin->typebin = MAP;
    entriesMapbin->data = entriesMap;
    packet->entriesMap = entriesMapbin;
    free(fp-fsize);

    printf("finised reading file.\n");
    return packet;
}

int encode(char* filepath, PacketBin* packet)
{
    printf("creating bin file.\n");
    charv* str = (charv*)calloc(1, sizeof(charv));
    myassert(str == NULL);
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
            uint16_t len = (uint16_t)strlen(packet->LinkedList[i]);
            memfwrite(&len, 2, str);
            memfwrite(packet->LinkedList[i], len, str);
        }
    }

    Map* entriesMap = (Map*)packet->entriesMap->data;
    memfwrite(&entriesMap->itemsize, 4, str);
    for (uint32_t i = 0; i < entriesMap->itemsize; i++)
        memfwrite(&((PointerOrEmbed*)entriesMap->items[i]->value->data)->name, 4, str);

    for (uint32_t i = 0; i < entriesMap->itemsize; i++)
    {
        uint32_t entryLength = 4 + 2;
        PointerOrEmbed* pe = (PointerOrEmbed*)entriesMap->items[i]->value->data;
        uint32_t entryKeyHash = *(uint32_t*)entriesMap->items[i]->key->data;
        for (uint16_t k = 0; k < pe->itemsize; k++)
            entryLength += getsize(pe->items[k]->value) + 4 + 1;

        memfwrite(&entryLength, 4, str);
        memfwrite(&entryKeyHash, 4, str);
        memfwrite(&pe->itemsize, 2, str);
        for (uint16_t k = 0; k < pe->itemsize; k++)
        {
            uint32_t name = pe->items[k]->key;
            uint8_t type = typetouint(pe->items[k]->value->typebin);
            memfwrite(&name, 4, str);
            memfwrite(&type, 1, str);
            writevaluebybin(pe->items[k]->value, str);
        }
    }

    printf("finised creating bin file.\n");
    printf("writing to file.\n");
    FILE* file = fopen(filepath, "wb");
    if (!file)
    {
        printf("ERROR: cannot write file \"%s\".", filepath);
        return 1;
    }
    fwrite(str->data, str->lenght, 1, file);
    printf("finised writing to file.\n");
    fclose(file);
    free(str->data);
    return 0;
}