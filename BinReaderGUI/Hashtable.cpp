#include "Hashtable.h"

mi_string HashTable::Lookup(const uint64_t key)
{
	auto found = table.find(key);
	if (found != table.end())
		return found->second;
	return mi_string();
}

int HashTable::Insert(const uint64_t key, mi_string val)
{
	if (HashTable::Lookup(key).size() == 0)
	{
		table[key] = val;
		return 1;
	}
	return 0;
}

void HashTable::LoadFromFile(const char* filePath)
{
	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		char errMsg[255] = { "\0" };
		strerror_s(errMsg, 255, errno);
		printf("ERROR: Cannot read file %s %s\n", filePath, errMsg);
		return;
	}

	size_t lines = 0;
	mi_string line = "";

	while (std::getline(ifs, line))
	{
		const size_t hashEnd = line.find(" ");

		uint64_t key = 0;
		if (hashEnd == 8)
			key = strtoul(line.c_str(), nullptr, 16);
		else if (hashEnd == 16)
			key = strtoull(line.c_str(), nullptr, 16);
		else
			continue;

		mi_string value = line.data() + hashEnd + 1;
		lines += HashTable::Insert(key, value);
	}

	ifs.close();

	printf("File: %s loaded: %zd lines\n", filePath, lines);
}

uint32_t FNV1Hash(const char* string, size_t stringLen)
{
	uint32_t Hash = 0x811c9dc5;
	for (size_t i = 0; i < stringLen; i++)
		Hash = (Hash ^ tolower(string[i])) * 0x01000193;
	return Hash;
}

uint64_t xxread8(const void *memPtr)
{
	uint8_t val;
	myassert(memcpy(&val, memPtr, 1) != &val)
	return val;
}
uint64_t xxread32(const void *memPtr)
{
	uint32_t val;
	myassert(memcpy(&val, memPtr, 4) != &val)
	return val;
}
uint64_t xxread64(const void *memPtr)
{
	uint64_t val;
	myassert(memcpy(&val, memPtr, 8) != &val)
	return val;
}
uint64_t XXH_rotl64(uint64_t x, int r)
{
	return ((x << r) | (x >> (64 - r)));
}

uint64_t XXHash(const char* input, size_t len)
{
	uint64_t h64;
	const char* inputEnd = input + len;

	if (len >= 32) {
		const char* inputLimit = inputEnd - 32;
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
		} while (input <= inputLimit);

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

	while (input + 8 <= inputEnd)
	{
		uint64_t k1 = xxread64(input);
		k1 *= PRIME2;
		k1 = XXH_rotl64(k1, 31);
		k1 *= PRIME1;
		h64 ^= k1;
		h64 = XXH_rotl64(h64, 27) * PRIME1 + PRIME4;
		input += 8;
	}

	if (input + 4 <= inputEnd)
	{
		h64 ^= xxread32(input) * PRIME1;
		h64 = XXH_rotl64(h64, 23) * PRIME2 + PRIME3;
		input += 4;
	}

	while (input < inputEnd)
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