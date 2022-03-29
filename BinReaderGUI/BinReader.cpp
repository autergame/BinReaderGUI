#include "BinReader.h"

bool IsComplexBinType(BinType type)
{
	return (type >= BinType::CONTAINER && type != BinType::LINK && type != BinType::FLAG);
}

bool IsPointerOrEmbedded(BinType type)
{
	return (type == BinType::POINTER || type == BinType::EMBEDDED);
}

BinType Uint8ToType(uint8_t type)
{
	if (type & 0x80)
		type = (type - 0x80) + (uint8_t)BinType::CONTAINER;
	myassert(type > (uint8_t)BinType::FLAG)
	return (BinType)type;
}

uint8_t TypeToUint8(BinType type)
{
	uint8_t raw = (uint8_t)type;
	if (raw >= (uint8_t)BinType::CONTAINER)
		raw = (raw - (uint8_t)BinType::CONTAINER) + 0x80;
	return raw;
}

BinField *ReadValueByBinFieldType(const uint8_t type, HashTable& hashT, TernaryTree& ternaryT, BinField *parent, CharMemVector& input)
{
	BinField *binResult = new BinField;
	binResult->type = Uint8ToType(type);
	binResult->parent = parent;
	switch (binResult->type)
	{
		case BinType::SInt8:
		case BinType::UInt8:
		case BinType::SInt16:
		case BinType::UInt16:
		case BinType::SInt32:
		case BinType::UInt32:
		case BinType::SInt64:
		case BinType::UInt64:
		case BinType::HASH:
		case BinType::LINK:
		case BinType::WADENTRYLINK:
		{
			input.MemRead(&binResult->data->ui64, Type_size[(uint8_t)binResult->type]);
			break;
		}
		case BinType::BOOLB:
		case BinType::FLAG:
		{
			input.MemRead(&binResult->data->b, Type_size[(uint8_t)binResult->type]);
			break;
		}		
		case BinType::Float32:
		case BinType::VEC2:
		case BinType::VEC3:
		case BinType::VEC4:
		case BinType::MTX44:
		{
			binResult->data->floatv = (float*)input.MemRead(Type_size[(uint8_t)binResult->type]);
			break;
		}
		case BinType::RGBA:
		{
			binResult->data->rgba = (uint8_t*)input.MemRead(Type_size[(uint8_t)binResult->type]);
			break;
		}
		case BinType::STRING:
		{
			size_t stringLength = input.MemRead<uint16_t>();

			char* string = new char[stringLength + 1];
			input.MemRead(string, stringLength);
			string[stringLength] = '\0';

			hashT.Insert(FNV1Hash(string, stringLength), string);
			hashT.Insert(XXHash(string, stringLength), string);

			ternaryT.Insert(string);

			binResult->data->string = string;
			break;
		}
		case BinType::CONTAINER:
		case BinType::STRUCT:
		{
			uint8_t type = input.MemRead<uint8_t>();
			uint32_t size = input.MemRead<uint32_t>();
			uint32_t fieldCount = input.MemRead<uint32_t>();

			ContainerOrStructOrOption *cs = new ContainerOrStructOrOption;
			cs->valueType = Uint8ToType(type);
			cs->items.reserve(fieldCount);

			for (uint32_t i = 0; i < fieldCount; i++)
			{
				CSOField field;
				field.value = ReadValueByBinFieldType(type, hashT, ternaryT, binResult, input);
				cs->items.emplace_back(field);
			}
			binResult->data->cso = cs;
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = new PointerOrEmbed;
			input.MemRead(&pe->name, 4);
			if (pe->name == 0)
			{
				binResult->data->pe = pe;
				break;
			}

			uint32_t size = input.MemRead<uint32_t>();
			uint16_t fieldCount = input.MemRead<uint16_t>();

			pe->items.reserve(fieldCount);
			for (uint16_t i = 0; i < fieldCount; i++)
			{
				EPField field;

				input.MemRead(&field.key, 4);
				uint8_t type = input.MemRead<uint8_t>();

				field.value = ReadValueByBinFieldType(type, hashT, ternaryT, binResult, input);
				pe->items.emplace_back(field);
			}
			binResult->data->pe = pe;
			break;
		}
		case BinType::OPTION:
		{
			uint8_t type = input.MemRead<uint8_t>();
			uint8_t fieldCount = input.MemRead<uint8_t>();

			ContainerOrStructOrOption *option = new ContainerOrStructOrOption;
			option->valueType = Uint8ToType(type);
			option->items.reserve(fieldCount);

			for (uint32_t i = 0; i < fieldCount; i++)
			{
				CSOField field;
				field.value = ReadValueByBinFieldType(type, hashT, ternaryT, binResult, input);
				option->items.emplace_back(field);
			}
			binResult->data->cso = option;
			break;
		}
		case BinType::MAP:
		{
			uint8_t keyType = input.MemRead<uint8_t>();
			uint8_t valueType = input.MemRead<uint8_t>();
			uint32_t size = input.MemRead<uint32_t>();
			uint32_t fieldCount = input.MemRead<uint32_t>();

			Map *map = new Map;
			map->keyType = Uint8ToType(keyType);
			map->valueType = Uint8ToType(valueType);
			map->items.reserve(fieldCount);

			for (uint32_t i = 0; i < fieldCount; i++)
			{
				MapPair pair;
				pair.key = ReadValueByBinFieldType(keyType, hashT, ternaryT, binResult, input);
				pair.value = ReadValueByBinFieldType(valueType, hashT, ternaryT, binResult, input);
				map->items.emplace_back(pair);
			}
			binResult->data->map = map;
			break;
		}
	}
	return binResult;
}

uint32_t GetTotalBinFieldSize(BinField *value)
{
	uint32_t size = (uint32_t)Type_size[(uint8_t)value->type];
	switch (value->type)
	{
		case BinType::STRING:
			size = 2 + (uint32_t)strlen(value->data->string);
			break;
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			size = 1 + 4 + 4;
			ContainerOrStructOrOption *cs = value->data->cso;
			for (uint32_t i = 0; i < cs->items.size(); i++)
				size += GetTotalBinFieldSize(cs->items[i].value);
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			size = 4;
			PointerOrEmbed *pe = value->data->pe;
			if (pe->name != 0)
			{
				size += 4 + 2;
				for (uint16_t i = 0; i < pe->items.size(); i++)
					size += GetTotalBinFieldSize(pe->items[i].value) + 4 + 1;
			}
			break;
		}
		case BinType::OPTION:
		{
			size = 2;
			ContainerOrStructOrOption *option = value->data->cso;
			for (uint8_t i = 0; i < option->items.size(); i++)
				size += GetTotalBinFieldSize(option->items[i].value);
			break;
		}
		case BinType::MAP:
		{
			size = 1 + 1 + 4 + 4;
			Map *map = value->data->map;
			for (uint32_t i = 0; i < map->items.size(); i++)
				size += GetTotalBinFieldSize(map->items[i].key) +
						GetTotalBinFieldSize(map->items[i].value);
			break;
		}
	}
	return size;
}

void WriteValueByBinField(BinField *value, CharMemVector& output)
{
	switch (value->type)
	{
		case BinType::SInt8:
		case BinType::UInt8:
		case BinType::SInt16:
		case BinType::UInt16:
		case BinType::SInt32:
		case BinType::UInt32:
		case BinType::SInt64:
		case BinType::UInt64:
		case BinType::HASH:
		case BinType::LINK:
		case BinType::WADENTRYLINK:
		{
			size_t size = Type_size[(uint8_t)value->type];
			output.MemWrite(&value->data->ui64, size);
			break;
		}
		case BinType::BOOLB:
		case BinType::FLAG:
		{
			size_t size = Type_size[(uint8_t)value->type];
			output.MemWrite(&value->data->b, size);
			break;
		}
		case BinType::Float32:
		case BinType::VEC2:
		case BinType::VEC3:
		case BinType::VEC4:
		case BinType::MTX44: 
		{
			size_t size = Type_size[(uint8_t)value->type];
			output.MemWrite(value->data->floatv, size);
			break;
		}
		case BinType::RGBA:
		{
			size_t size = Type_size[(uint8_t)value->type];
			output.MemWrite(value->data->rgba, size);
			break;
		}
		case BinType::STRING:
		{
			char* string = value->data->string;
			uint16_t stringLen = (uint16_t)strlen(string);

			output.MemWrite(stringLen);
			output.MemWrite(string, stringLen);
			break;
		}
		case BinType::STRUCT:
		case BinType::CONTAINER:
		{
			ContainerOrStructOrOption *cs = value->data->cso;
			uint32_t size = 4, fieldCount = (uint32_t)cs->items.size();

			uint8_t type = TypeToUint8(cs->valueType);

			for (uint32_t i = 0; i < fieldCount; i++)
				size += GetTotalBinFieldSize(cs->items[i].value);

			output.MemWrite(type);
			output.MemWrite(size);
			output.MemWrite(fieldCount);

			for (uint32_t i = 0; i < fieldCount; i++)
				WriteValueByBinField(cs->items[i].value, output);
			break;
		}
		case BinType::POINTER:
		case BinType::EMBEDDED:
		{
			PointerOrEmbed *pe = value->data->pe;
			output.MemWrite(pe->name);
			if (pe->name == 0)
				break;

			uint32_t size = 2;
			uint16_t fieldCount = (uint16_t)pe->items.size();

			for (uint16_t i = 0; i < fieldCount; i++)
				size += GetTotalBinFieldSize(pe->items[i].value) + 4 + 1;

			output.MemWrite(size);
			output.MemWrite(fieldCount);

			for (uint16_t i = 0; i < fieldCount; i++)
			{
				uint8_t type = TypeToUint8(pe->items[i].value->type);
				output.MemWrite(pe->items[i].key);
				output.MemWrite(type);
				WriteValueByBinField(pe->items[i].value, output);
			}
			break;
		}
		case BinType::OPTION:
		{
			ContainerOrStructOrOption *op = value->data->cso;
			uint8_t fieldCount = (uint8_t)op->items.size();

			uint8_t type = TypeToUint8(op->valueType);
			output.MemWrite(type);
			output.MemWrite(fieldCount);

			for (uint8_t i = 0; i < fieldCount; i++)
				WriteValueByBinField(op->items[i].value, output);
			break;
		}
		case BinType::MAP:
		{
			Map *map = value->data->map;
			uint32_t size = 4, fieldCount = (uint32_t)map->items.size();

			for (uint32_t i = 0; i < fieldCount; i++)
				size += GetTotalBinFieldSize(map->items[i].key) + 
						GetTotalBinFieldSize(map->items[i].value);

			uint8_t typeKey = TypeToUint8(map->keyType);
			uint8_t typeValue = TypeToUint8(map->valueType);
			output.MemWrite(typeKey);
			output.MemWrite(typeValue);

			output.MemWrite(size);
			output.MemWrite(fieldCount);

			for (uint32_t i = 0; i < fieldCount; i++)
			{
				WriteValueByBinField(map->items[i].key, output);
				WriteValueByBinField(map->items[i].value, output);
			}
			break;
		}
	}
}

int PacketBin::DecodeBin(char* filePath, HashTable& hashT, TernaryTree& ternaryT)
{
	FILE *file;
	errno_t err = fopen_s(&file, filePath, "rb");
	if (err)
	{
		char errMsg[255] = { "\0" };
		strerror_s(errMsg, 255, errno);
		printf("ERROR: Cannot read file %s %s\n", filePath, errMsg);
		return 0;
	}

	printf("Reading file: %s\n", filePath);
	fseek(file, 0, SEEK_END);
	size_t fsize = (size_t)ftell(file);
	fseek(file, 0, SEEK_SET);
	mi_string fp(fsize + 1, '\0');
	myassert(fread(fp.data(), 1, fsize, file) != fsize)
	fclose(file);
	printf("Finised reading file\n");

	printf("Reading bin from file\n");

	CharMemVector input;
	input.MemWrite(fp.data(), fsize);

	uint32_t signature = input.MemRead<uint32_t>();
	if (memcmp(&signature, "PTCH", 4) == 0)
	{
		input.MemRead(&m_Unknown, 8);
		input.MemRead(&signature, 4);
		m_isPatch = true;
	}
	if (memcmp(&signature, "PROP", 4) != 0)
	{
		printf("Bin has no valid signature\n");
		return 0;
	}

	input.MemRead(&m_Version, 4);

	if (m_Version >= 2)
	{
		uint32_t linkedCount = input.MemRead<uint32_t>();
		if (linkedCount > 0)
		{
			m_linkedList.reserve(linkedCount);
			for (uint32_t i = 0; i < linkedCount; i++)
			{
				size_t stringLength = (size_t)input.MemRead<uint16_t>();

				mi_string linkedStr(stringLength + 1, '\0');
				input.MemRead(linkedStr.data(), stringLength);

				m_linkedList.emplace_back(0, linkedStr);
			}
		}
	}

	Map *entriesMap = new Map;
	entriesMap->keyType = BinType::HASH;
	entriesMap->valueType = BinType::EMBEDDED;

	m_entriesBin = new BinField;
	m_entriesBin->type = BinType::MAP;
	m_entriesBin->data->map = entriesMap;

	size_t entriesCount = (size_t)input.MemRead<uint32_t>();
	if (entriesCount > 0)
	{
		uint32_t* entryTypes = (uint32_t*)input.MemRead(entriesCount * 4);

		entriesMap->items.reserve(entriesCount);
		for (size_t i = 0; i < entriesCount; i++)
		{
			uint32_t entryLength = input.MemRead<uint32_t>();
			uint32_t entryKeyHash = input.MemRead<uint32_t>();
			uint16_t fieldCount = input.MemRead<uint16_t>();

			PointerOrEmbed *embed = new PointerOrEmbed;
			embed->name = entryTypes[i];
			embed->items.reserve(fieldCount);

			BinField *embedValue = new BinField;
			embedValue->parent = m_entriesBin;
			embedValue->type = BinType::EMBEDDED;
			embedValue->data->pe = embed;

			BinField *hashKey = new BinField;
			hashKey->parent = m_entriesBin;
			hashKey->type = BinType::HASH;
			hashKey->data->ui32 = entryKeyHash;

			MapPair pair;
			pair.key = hashKey;
			pair.value = embedValue;
			entriesMap->items.emplace_back(pair);

			for (uint16_t o = 0; o < fieldCount; o++)
			{
				uint32_t name = input.MemRead<uint32_t>();
				uint8_t type = input.MemRead<uint8_t>();

				EPField field;
				field.key = name;
				field.value = ReadValueByBinFieldType(type, hashT, ternaryT, embedValue, input);
				embed->items.emplace_back(field);
			}
		}
	}

	Map *patchMap = new Map;
	patchMap->keyType = BinType::HASH;
	patchMap->valueType = BinType::EMBEDDED;

	m_patchesBin = new BinField;
	m_patchesBin->type = BinType::MAP;
	m_patchesBin->data->map = patchMap;

	if (m_isPatch && m_Version >= 3) 
	{
		size_t patchCount = (size_t)input.MemRead<uint32_t>();
		if (patchCount > 0)
		{
			patchMap->items.reserve(patchCount);

			for (size_t i = 0; i < patchCount; i++)
			{
				uint32_t patchKeyHash = input.MemRead<uint32_t>();
				uint32_t patchLength = input.MemRead<uint32_t>();

				uint8_t type = input.MemRead<uint8_t>();
				size_t stringLength = input.MemRead<uint16_t>();
				char* string = new char[stringLength + 1];
				input.MemRead(string, stringLength);
				string[stringLength] = '\0';

				PointerOrEmbed *embed = new PointerOrEmbed;
				embed->name = patchFNV;

				BinField *embedValue = new BinField;
				embedValue->parent = m_patchesBin;
				embedValue->type = BinType::EMBEDDED;
				embedValue->data->pe = embed;

				BinField *stringBin = new BinField;
				stringBin->parent = embedValue;
				stringBin->type = BinType::STRING;
				stringBin->data->string = string;

				EPField firstField;
				firstField.key = pathFNV;
				firstField.value = stringBin;
				embed->items.emplace_back(firstField);

				EPField secondField;
				secondField.key = valueFNV;
				secondField.value = ReadValueByBinFieldType(type, hashT, ternaryT, embedValue, input);
				embed->items.emplace_back(secondField);

				BinField *hashKey = new BinField;
				hashKey->parent = m_patchesBin;
				hashKey->type = BinType::HASH;
				hashKey->data->ui32 = patchKeyHash;

				MapPair pair;
				pair.key = hashKey;
				pair.value = embedValue;
				patchMap->items.emplace_back(pair);
			}
		}
	}

	printf("Finished reading bin from file\n\n");
	return 1;
}

int PacketBin::EncodeBin(char* filePath)
{
	printf("Creating bin file: %s\n", filePath);
	CharMemVector output;

	if (m_isPatch)
	{
		if (m_Unknown = 0)
			m_Unknown = 1;
		output.MemWrite((void*)"PTCH", 4);
		output.MemWrite(m_Unknown);
	}
	output.MemWrite((void*)"PROP", 4);
	output.MemWrite(m_Version);

	if (m_Version >= 2)
	{
		uint32_t linkedCount = (uint32_t)m_linkedList.size();
		output.MemWrite(linkedCount);

		for (uint32_t i = 0; i < linkedCount; i++)
		{
			uint16_t strlen = (uint16_t)m_linkedList[i].second.length();

			output.MemWrite(strlen);
			output.MemWrite(m_linkedList[i].second.data(), strlen);
		}
	}

	Map *entriesMap = m_entriesBin->data->map;
	uint32_t entriesCount = (uint32_t)entriesMap->items.size();
	output.MemWrite(entriesCount);

	for (uint32_t i = 0; i < entriesCount; i++)
		output.MemWrite(entriesMap->items[i].value->data->pe->name);

	for (uint32_t i = 0; i < entriesCount; i++)
	{
		uint32_t entryLength = 4 + 2;
		uint32_t entryKeyHash = entriesMap->items[i].key->data->ui32;

		PointerOrEmbed *pe = entriesMap->items[i].value->data->pe;
		uint16_t fieldCount = (uint16_t)pe->items.size();

		for (uint16_t k = 0; k < fieldCount; k++)
			entryLength += GetTotalBinFieldSize(pe->items[k].value) + 4 + 1;

		output.MemWrite(entryLength);
		output.MemWrite(entryKeyHash);
		output.MemWrite(fieldCount);

		for (uint16_t k = 0; k < fieldCount; k++)
		{
			uint8_t type = TypeToUint8(pe->items[k].value->type);

			output.MemWrite(pe->items[k].key);
			output.MemWrite(type);

			WriteValueByBinField(pe->items[k].value, output);
		}
	}

	if (m_isPatch && m_Version >= 3)
	{
		Map *patchesBin = m_patchesBin->data->map;
		uint32_t patchCount = (uint32_t)patchesBin->items.size();
		output.MemWrite(patchCount);

		for (uint32_t i = 0; i < patchCount; i++)
		{
			uint32_t patchKeyHash = patchesBin->items[i].key->data->ui32;
			uint32_t patchLength = 1;

			PointerOrEmbed *pe = patchesBin->items[i].value->data->pe;

			char* string = pe->items[0].value->data->string;
			uint16_t stringLen = (uint16_t)strlen(string);

			patchLength += 2 + stringLen;
			patchLength += GetTotalBinFieldSize(pe->items[1].value);

			output.MemWrite(patchKeyHash);
			output.MemWrite(patchLength);

			uint8_t type = TypeToUint8(pe->items[1].value->type);
			output.MemWrite(type);

			output.MemWrite(stringLen);
			output.MemWrite(string, stringLen);

			WriteValueByBinField(pe->items[1].value, output);
		}
	}

	printf("Finised creating bin file\n");

	printf("Writing bin to file\n");

	FILE *file;
	errno_t err = fopen_s(&file, filePath, "wb");
	if (err)
	{
		char errMsg[255] = { '\0' };
		strerror_s(errMsg, 255, err);
		printf("ERROR: Cannot write file %s %s\n", filePath, errMsg);
		return 0;
	}

	size_t arraySize = output.m_Array.size();
	myassert(fwrite(output.m_Array.data(), 1, arraySize, file) != arraySize)
	fclose(file);

	printf("Finised writing bin to file\n\n");
	return 1;
}