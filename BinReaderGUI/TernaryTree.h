#ifndef _TERNARYTREE_H_
#define _TERNARYTREE_H_

#include <ctype.h>

#include <random>

#include "MemoryWrapper.h"
#include "Hashtable.h"

struct TernaryNode
{
	char key = '\0';
	mi_string endText = "";
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

	int Insert(mi_string word);
	const TernaryNode *Search(const char* query);
	int SujestionsSize(const char* query, const char** biggesttext);
	int Sujestions(const char* query, const char** output,
		const int startat, const int maxsize);
	void LoadFromHashTable(const HashTable& hasht);
};

#endif //_TERNARYTREE_H_