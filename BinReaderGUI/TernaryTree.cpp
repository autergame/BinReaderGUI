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

int TernaryTree::Insert(mi_string word)
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

int TernaryTree::SujestionsSize(const char* query, const char** biggesttext)
{
	const TernaryNode* lastNode = TernaryTree::Search(query);
	if (lastNode)
	{
		if (lastNode->leftChild || lastNode->rightChild || lastNode->equalChild)
		{
			int index = 0;
			size_t textsizefinal = 0;

			mi_deque<const TernaryNode*> stack;
			stack.emplace_back(lastNode);

			while (stack.size() > 0)
			{
				const TernaryNode* nodeStack = stack.back(); stack.pop_back();

				size_t textsize = nodeStack->endText.size();
				if (textsize > 0)
				{
					index++;
					if (textsize > textsizefinal)
					{
						textsizefinal = textsize;
						*biggesttext = nodeStack->endText.c_str();
					}
				}

				if (nodeStack->rightChild)
					stack.emplace_back(nodeStack->rightChild);
				if (nodeStack->equalChild)
					stack.emplace_back(nodeStack->equalChild);
				if (nodeStack->leftChild)
					stack.emplace_back(nodeStack->leftChild);
			}

			return index - 1;
		}
	}
	return 0;
}

int TernaryTree::Sujestions(const char* query, const char** output,
	const int startat, const int maxsize)
{
	const TernaryNode *lastNode = TernaryTree::Search(query);
	if (lastNode)
	{
		if (lastNode->leftChild || lastNode->rightChild || lastNode->equalChild)
		{
			int index = 0;
			int minindex = 0;
			bool removequery = true;

			mi_deque<const TernaryNode*> stack;
			stack.emplace_back(lastNode);

			while (stack.size() > 0 && index < maxsize)
			{
				const TernaryNode *nodeStack = stack.back(); stack.pop_back();

				if (nodeStack->endText.size() > 0)
				{
					if (removequery)
					{
						if (query == nodeStack->endText)
						{
							removequery = false;
							goto jump;
						}
					}
					if (minindex++ >= startat)
						output[index++] = nodeStack->endText.c_str();
				}

				jump:

				if (nodeStack->rightChild)
					stack.emplace_back(nodeStack->rightChild);
				if (nodeStack->equalChild)
					stack.emplace_back(nodeStack->equalChild);
				if (nodeStack->leftChild)
					stack.emplace_back(nodeStack->leftChild);
			}

			return index;
		}
	}
	return 0;
}

void TernaryTree::LoadFromHashTable(const HashTable& hasht)
{
	size_t total = 0;
	for (auto it = hasht.table.begin(); it != hasht.table.end(); it++) {
		total += TernaryTree::Insert(it->second);
	}
	printf("Ternary loaded total of hashes: %zd\n", total);
}