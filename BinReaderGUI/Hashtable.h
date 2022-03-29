#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "Myassert.h"
#include "MemoryWrapper.h"


struct custom_hash {
	uint64_t operator()(uint64_t x) const {
		return x;
	}
};

class HashTable
{
public:
	mi_unordered_map<uint64_t, mi_string, custom_hash> table;

	HashTable() {}
	~HashTable() {}
	mi_string Lookup(const uint64_t key);
	int Insert(const uint64_t key, mi_string val);
	void LoadFromFile(const char* filePath);
};

uint32_t FNV1Hash(const char* string, size_t stringLen);

static const uint64_t PRIME1 = 0x9E3779B185EBCA87ULL;
static const uint64_t PRIME2 = 0xC2B2AE3D27D4EB4FULL;
static const uint64_t PRIME3 = 0x165667B19E3779F9ULL;
static const uint64_t PRIME4 = 0x85EBCA77C2B2AE63ULL;
static const uint64_t PRIME5 = 0x27D4EB2F165667C5ULL;
uint64_t xxread8(const void *memPtr);
uint64_t xxread32(const void *memPtr);
uint64_t xxread64(const void *memPtr);
uint64_t XXH_rotl64(uint64_t x, int r);
uint64_t XXHash(const char* input, size_t len);

#endif //_HASHTABLE_H_