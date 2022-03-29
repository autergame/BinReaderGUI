#ifndef _BINREADER_H_
#define _BINREADER_H_

#include <inttypes.h>

#include <algorithm>

#include "Myassert.h"
#include "MemoryWrapper.h"

#include "Hashtable.h"
#include "TernaryTree.h"

#include <mimalloc.h>

enum class BinType : uint8_t
{
	NONE,
	BOOLB,
	SInt8, UInt8, SInt16, UInt16, SInt32, UInt32, SInt64, UInt64,
	Float32, VEC2, VEC3, VEC4, MTX44,
	RGBA,
	STRING,
	HASH, WADENTRYLINK, 
	CONTAINER, STRUCT,
	POINTER, EMBEDDED,
	LINK,
	OPTION,
	MAP,
	FLAG
};

bool IsComplexBinType(BinType type);
bool IsPointerOrEmbedded(BinType type);

BinType Uint8ToType(uint8_t type);
uint8_t TypeToUint8(BinType type);

static const char* Type_strings[] = {
	"None",
	"Bool",
	"SInt8", "UInt8", "SInt16", "UInt16", "SInt32", "UInt32", "SInt64", "UInt64",
	"Float32", "Vector2", "Vector3", "Vector4", "Matrix4x4",
	"RGBA",
	"String", 
	"Hash", "WadEntryLink", 
	"Container", "Struct",
	"Pointer", "Embedded",
	"Link", 
	"Option",
	"Map",
	"Flag"
};

static const char* Type_fmt[] = {
	nullptr, nullptr, "%" PRIi8, "%" PRIu8, "%" PRIi16, "%" PRIu16, "%" PRIi32, "%" PRIu32, "%" PRIi64, "%" PRIu64
};

struct ContainerOrStructOrOption;
struct PointerOrEmbed;
struct Map;

struct BinData
{
	union
	{
		bool b;
		uint32_t ui32;
		uint64_t ui64 = 0;

		union {
			float* floatv;
			uint8_t* rgba;
			char* string;

			ContainerOrStructOrOption *cso;
			PointerOrEmbed *pe;
			Map *map;
		};
	};
};

struct BinField
{
	BinType type = BinType::NONE;
	BinData *data = new BinData;

	int id = 0;
	BinField *parent = nullptr;
};

struct CSOField
{
	BinField *value = nullptr;

	int id = 0;
	ImGuiID idim = 0;
	ImVec2 cursorMin = { 0,0 };
	ImVec2 cursorMax = { 0,0 };
	bool isOver = false;
	bool expanded = false;
};

struct EPField
{
	uint32_t key = 0;
	BinField *value = nullptr;

	int id = 0;
	ImGuiID idim = 0;
	ImVec2 cursorMin = { 0,0 };
	ImVec2 cursorMax = { 0,0 };
	bool isOver = false;
	bool expanded = false;
};

struct MapPair
{
	BinField *key = nullptr;
	BinField *value = nullptr;

	int id = 0;
	ImGuiID idim = 0;
	ImVec2 cursorMin = { 0,0 };
	ImVec2 cursorMax = { 0,0 };
	bool isOver = false;
	bool expanded = false;
};


struct ContainerOrStructOrOption
{
	BinType valueType = BinType::NONE;
	mi_vector<CSOField> items;

	uint8_t current2 = 0;
	uint8_t current3 = 0;
};

struct PointerOrEmbed
{
	uint32_t name = 0;
	mi_vector<EPField> items;

	uint8_t current1 = 0;
	uint8_t current2 = 0;
	uint8_t current3 = 0;
	ImGuiID idim = 0;
};

struct Map
{
	BinType keyType = BinType::NONE;
	BinType valueType = BinType::NONE;
	mi_vector<MapPair> items;

	uint8_t current2 = 0;
	uint8_t current3 = 0;
	uint8_t current4 = 0;
	uint8_t current5 = 0;
};

static const size_t Type_size[] = {
	0, //NONE
	1, //BOOLB
	1, 1, 2, 2, 4, 4, 8, 8, //SInt8, UInt8, SInt16, UInt16, SInt32, UInt32, SInt64, UInt64
	4, 8, 12, 16, 64, //Float32, VEC2, VEC3, VEC4, MTX44
	4, //RGBA
	0, //STRING
	4, 8, //HASH, WADENTRYLINK
	sizeof(ContainerOrStructOrOption), sizeof(ContainerOrStructOrOption),
	sizeof(PointerOrEmbed), sizeof(PointerOrEmbed),
	4, //LINK
	sizeof(ContainerOrStructOrOption),
	sizeof(Map),
	1 //FLAG
};

const uint32_t patchFNV = 0xf9100aa9; // FNV1Hash("patch")
const uint32_t pathFNV = 0x84874d36; // FNV1Hash("path")
const uint32_t valueFNV = 0x425ed3ca;  // FNV1Hash("value")

class PacketBin
{
public:
	bool m_isPatch = false;
	uint32_t m_Version = 0;
	uint64_t m_Unknown = 0;
	BinField *m_entriesBin = nullptr;
	BinField *m_patchesBin = nullptr;
	mi_vector<std::pair<int, mi_string>> m_linkedList;

	PacketBin() {}
	~PacketBin() {}

	int EncodeBin(char* filePath);
	int DecodeBin(char* filePath, HashTable& hasht, TernaryTree& ternaryt);
};

class CharMemVector
{
public:
	size_t m_Pointer = 0;
	mi_vector<uint8_t> m_Array;

	CharMemVector() {}
	~CharMemVector() {}

	template<typename T>
	void MemWrite(T value)
	{
		MemWrite((void*)&value, sizeof(T));
	}

	void MemWrite(void *value, size_t size)
	{
		m_Array.insert(m_Array.end(), (uint8_t*)value, (uint8_t*)value + size);
	}


	template<typename T>
	T MemRead()
	{
		return *(T*)MemRead(sizeof(T));
	}

	void *MemRead(size_t bytes)
	{
		uint8_t* buffer = new uint8_t[bytes];
		MemRead(buffer, bytes);
		return buffer;
	}

	void MemRead(void *buffer, size_t bytes)
	{
		auto arrayIt = m_Array.begin() + m_Pointer;
		std::_Adl_verify_range(arrayIt, arrayIt + bytes);

		myassert(memcpy(buffer, m_Array.data() + m_Pointer, bytes) != buffer)
		m_Pointer += bytes;
	}
};

#endif //_BINREADER_H_