#ifndef _TERNARYTREE_H_
#define _TERNARYTREE_H_

#include <ctype.h>

#include <random>

#include "MemoryWrapper.h"
#include "Hashtable.h"

struct TernaryNode
{
	char key = '\0';
	const char* endText = nullptr;

	TernaryNode *leftChild = nullptr;
	TernaryNode *equalChild = nullptr;
	TernaryNode *rightChild = nullptr;
};

void CleanTernaryNode(TernaryNode *node);

class TernaryTree
{
public:
	TernaryNode *m_nodeRoot = nullptr;

	TernaryTree() {}
	~TernaryTree() {
		CleanTernaryNode(m_nodeRoot);
	}

	int Insert(const char* word);
	const TernaryNode *Search(const char* query);
	void Sujestions(const char* query, const char** biggesttext, mi_vector<const char*>* list);
	void LoadFromHashTable(const HashTable& hasht);
};

#endif //_TERNARYTREE_H_