#include "TernaryTree.h"

void CleanTernaryNode(TernaryNode *node)
{
	if (node)
	{
		CleanTernaryNode(node->leftChild);
		CleanTernaryNode(node->equalChild);
		CleanTernaryNode(node->rightChild);
		delete node;
	}
}

int TernaryTree::Insert(const char* word)
{
	TernaryNode **nodeRootModifier = &m_nodeRoot;
	TernaryNode *nodeInterator;

	size_t i = 0;
	while ((nodeInterator = *nodeRootModifier))
	{
		const char diff = word[i] - nodeInterator->key;
		if (diff == 0)
		{
			if (word[i++] == '\0')
				return 0;
			nodeRootModifier = &(nodeInterator->equalChild);
		}
		else if (diff < 0)
			nodeRootModifier = &(nodeInterator->leftChild);
		else
			nodeRootModifier = &(nodeInterator->rightChild);
	}
	while (true)
	{
		nodeInterator = *nodeRootModifier = new TernaryNode;
		nodeInterator->key = word[i];
		if (word[i++] == '\0') {
			nodeInterator->endText = word;
			return 1;
		}
		nodeRootModifier = &(nodeInterator->equalChild);
	}
	return 0;
}

const TernaryNode *TernaryTree::Search(const char* query)
{
	const TernaryNode *nodeInterator = m_nodeRoot;

	size_t i = 0;
	while (nodeInterator)
	{
		const char diff = query[i] - nodeInterator->key;
		if (diff == 0)
		{
			nodeInterator = nodeInterator->equalChild;
			if (query[++i] == '\0')
				return nodeInterator;
		}
		else if (diff < 0)
			nodeInterator = nodeInterator->leftChild;
		else
			nodeInterator = nodeInterator->rightChild;
	}
	return nullptr;
}

void TernaryTree::Sujestions(const char* query, const char** biggesttext, mi_vector<const char*>* list)
{
	const TernaryNode *lastNode = TernaryTree::Search(query);
	if (lastNode)
	{
		if (lastNode->leftChild || lastNode->rightChild || lastNode->equalChild)
		{
			bool removequery = true;
			size_t textsizefinal = 0;

			mi_deque<const TernaryNode*> stack;
			stack.emplace_back(lastNode);

			while (stack.size() > 0)
			{
				const TernaryNode *nodeStack = stack.back(); stack.pop_back();

				if (nodeStack->endText)
				{
					if (strlen(nodeStack->endText) > 0)
					{
						if (removequery)
						{
							if (strcmp(query, nodeStack->endText) == 0)
							{
								removequery = false;
								goto jump;
							}
						}

						size_t textsize = strlen(nodeStack->endText);
						if (textsize > 0)
						{
							if (textsize > textsizefinal)
							{
								textsizefinal = textsize;
								*biggesttext = nodeStack->endText;
							}
						}

						list->push_back(nodeStack->endText);
					}
				}

			jump:
				if (nodeStack->rightChild)
					stack.emplace_back(nodeStack->rightChild);
				if (nodeStack->equalChild)
					stack.emplace_back(nodeStack->equalChild);
				if (nodeStack->leftChild)
					stack.emplace_back(nodeStack->leftChild);
			}
		}
	}
}

void TernaryTree::LoadFromHashTable(const HashTable& hasht)
{
	size_t total = 0;
	for (auto &it : hasht.table) {
		total += TernaryTree::Insert(it.second.c_str());
	}
	printf("Ternary loaded total of hashes: %zd\n", total);
}