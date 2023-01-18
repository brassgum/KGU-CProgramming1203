#define _CRT_SECURE_NO_WARNINGS
// Ignore the return value of scanf
#pragma warning (disable: 6031)
#include <stdio.h>
#include <stdlib.h>

#include <assert.h>

#define NEGATIVE_WORDS_FILE "negative-words.txt"
#define POSITIVE_WORDS_FILE "positive-words.txt"
#define AVERAGE_WORD_SIZE 4.7F
#define ALPHABET_LETTERS 27
#define INPUT_BUFFER_SIZE 512

#define ROUND(f) ((int)((f) + 0.5F))
#define IS_A_VALID_CHARACTER(c)  (('a' <= (c) && (c) <= 'z') \
	|| ('A' <= (c) && (c) <= 'Z') || ('0' <= (c) && (c) <= '9') \
	|| (c) == '*' || (c) == '+' || (c) == '-' || (c) == '?')
#define DICTIONARY_1D_HASHING(value) (int)((((value) | 0x20) - 'a' + 1) \
	* ('a' <= ((value) | 0x20) && ((value) | 0x20) <= 'z'))

typedef unsigned short key_t;
typedef enum WordAttribute
{
	NEGATIVE, POSITIVE
} WordAttribute_t;

typedef enum Color
{
	RED, BLACK
} Color_t;

typedef enum ChildIndex
{
	LEFT, RIGHT
} ChildIndex_t;

typedef struct MemoryBlock_s
{
	struct MemoryBlock_s* Next;
} MemoryBlock_t, * pMemoryBlock_t;

typedef struct WordNode_s
{
	char* Word;
	struct WordNode_s* Next;
	unsigned int Length;
	WordAttribute_t bPositive;
} WordNode_t, * pWordNode_t;

typedef struct DictionaryNode_s
{
	struct DictionaryNode_s* Child[2];
	struct DictionaryNode_s* Parent;
	pWordNode_t WordList;
	Color_t Color;
	key_t KeyValue;
} DictionaryNode_t, * pDictionaryNode_t;

const size_t DICTIONARY_1D_SIZE = ALPHABET_LETTERS + 1;
const char* const RATE[2] = { "positive", "negative" };
pMemoryBlock_t MemBlockList = NULL;
pDictionaryNode_t* Dictionary = NULL;

long GetFileLength(FILE* stream);
int GetWordLength(const char* str);
int StrComp(const char* str0, size_t strLen0,
	const char* str1, size_t strLen1);
void* MemChunkAlloc(size_t size);
void ReleaseAllMemory(void);
pDictionaryNode_t _InsertLabelRecursion(
	pDictionaryNode_t trav, pDictionaryNode_t temp);
void RotateRight(pDictionaryNode_t* root, pDictionaryNode_t temp);
void RotateLeft(pDictionaryNode_t* root, pDictionaryNode_t temp);
void FixUp(pDictionaryNode_t* root, pDictionaryNode_t temp);
void InsertLabel(pDictionaryNode_t* root, pDictionaryNode_t newNode);
key_t CreateKey(const char* word);
void BuildDictionary(pWordNode_t const positiveList,
	pWordNode_t const negativeList, size_t wordsCount);
int Init(void);
pWordNode_t GetOnDictoinary(const char* word, int wordLen);
void ReadReviewFile(const char* fileName);

int main(int argc, char** argv)
{
	if (Init())
	{
		printf("File not exist %s/%s files", POSITIVE_WORDS_FILE, NEGATIVE_WORDS_FILE);
		exit(1);
	}
	for (size_t i = 1; i < argc; i++)
	{
		ReadReviewFile(argv[i]);
	}
	if (argc < 2)
	{
		
		char inputBuffer[INPUT_BUFFER_SIZE];
		do
		{
			printf("input review file(if you want to stop program, type ;): ");
			if (fgets(inputBuffer, INPUT_BUFFER_SIZE, stdin) == NULL) { break; }
			else if (inputBuffer[0] == ';') { break; }
			else
			{
				size_t i;
				for (i = 0; i < INPUT_BUFFER_SIZE; i++)
				{
					if (inputBuffer[i] == '\n')
					{
						
						break;
					}
				}
				inputBuffer[i] = 0;
			}
			ReadReviewFile(inputBuffer);
		} while (1);
	}
	ReleaseAllMemory();
	return 0;
}

long GetFileLength(FILE* stream)
{
	fpos_t posBackup;
	fgetpos(stream, &posBackup);
	fseek(stream, 0L, SEEK_END);
	long res = ftell(stream);
	fsetpos(stream, &posBackup);
	return res;
}

int GetWordLength(const char* str)
{
	int result = 0;

	if (IS_A_VALID_CHARACTER(*str))
	{
		while (IS_A_VALID_CHARACTER(*str))
		{
			++result;
			++str;
		}
	}
	else
	{
		while (!IS_A_VALID_CHARACTER(*str))
		{
			--result;
			++str;
		}
	}
	return result;
}

// 0: str0 == str1, 1: str0 < str1, -1: str0 > st1
int StrComp(const char* str0, size_t strLen0,
	const char* str1, size_t strLen1)
{
	unsigned char temp0;
	unsigned char temp1;
	for (size_t i = 0;
		i < strLen0 && i < strLen1;
		++i, ++str0, ++str1)
	{
		temp0 = *str0;
		temp1 = *str1;
		if (temp0 == '?') { temp0 = 0xff; }
		if (temp1 == '?') { temp1 = 0xff; }
		if (temp0 == temp1) { continue; }
		if (temp0 < temp1) { return 1; }
		else { return -1; }
	}
	if (strLen0 == strLen1) { return 0; }
	return -2 * (strLen0 > strLen1) + 1;
}

void* MemChunkAlloc(size_t size)
{
	if (size == 0) { return NULL; }
	pMemoryBlock_t newBlock = malloc(size + sizeof(MemoryBlock_t));
	if (newBlock == NULL) { return NULL; }
	newBlock->Next = MemBlockList;
	MemBlockList = newBlock;
	return ((char*)newBlock) + sizeof(MemoryBlock_t);
}

void ReleaseAllMemory(void)
{
	pMemoryBlock_t temp = MemBlockList;
	MemBlockList = NULL;
	while (temp != NULL)
	{
		void* freeBlock = temp;
		temp = temp->Next;
		free(freeBlock);
	}
}

int Init(void)
{
	int ret = 0;
	size_t i = 0;
	FILE* positiveFile = fopen(POSITIVE_WORDS_FILE, "r");
	if (positiveFile == NULL) { return -1; }
	FILE* negativeFile = fopen(NEGATIVE_WORDS_FILE, "r");
	if (negativeFile == NULL)
	{
		ret = -1;
		goto LB_CLOSE_POSITIVE_WORDS_FILE;
	}
	long positiveFileLen = GetFileLength(positiveFile) + 1;
	long negativeFileLen = GetFileLength(negativeFile) + 1;
	size_t estimatedWordCount = ((size_t)positiveFileLen + negativeFileLen) / 5/*ROUND(AVERAGE_WORD_SIZE)*/;
	char* bytePtr = MemChunkAlloc(
		(sizeof(pDictionaryNode_t) + sizeof(DictionaryNode_t)) * DICTIONARY_1D_SIZE
		+ sizeof(WordNode_t) * estimatedWordCount
		+ positiveFileLen + negativeFileLen);
	assert(bytePtr != NULL);
	Dictionary = (pDictionaryNode_t*)bytePtr;
	for (i = 0; i < DICTIONARY_1D_SIZE; ++i)
	{
		Dictionary[i] = ((pDictionaryNode_t)(Dictionary + DICTIONARY_1D_SIZE)) + i;
	}
	pWordNode_t wordList = (pWordNode_t)(Dictionary[DICTIONARY_1D_SIZE - 1] + 1);
	char* positiveWordsSource = (char*)(wordList + estimatedWordCount);
	char* negativeWordsSource = positiveWordsSource + positiveFileLen;
	void* sourceEnd = negativeWordsSource + negativeFileLen;
	fread(positiveWordsSource, 1, positiveFileLen, positiveFile);
	fread(negativeWordsSource, 1, negativeFileLen, negativeFile);
	positiveWordsSource[positiveFileLen - 1] = negativeWordsSource[negativeFileLen - 1] = 0;

	char* wordInitial = positiveWordsSource;
	size_t sumWordLen = 0;
	size_t wordsCount = 0;
	pWordNode_t list = wordList;
	pWordNode_t negativeWordList = NULL;
FILL_WORDS_LIST:
	for (i = 0; i < estimatedWordCount && (size_t)wordInitial < (size_t)sourceEnd;)
	{
		int wordLen = GetWordLength(wordInitial);
		if (wordLen < 0)
		{
			wordInitial = wordInitial - wordLen;
		}
		else if (wordLen > 0)
		{
			++i;
			list->Word = wordInitial;
			list->Length = wordLen;
			list->Next = list + 1;
			sumWordLen += wordLen;
			list->bPositive = (size_t)wordInitial < (size_t)negativeWordsSource;
			if (negativeWordList == NULL && !list->bPositive)
			{
				negativeWordList = list;
			}
			wordInitial = wordInitial + wordLen;
			++list;
			++wordsCount;
		}
		else
		{
			++wordInitial;
		}
	}
	if ((size_t)wordInitial < (size_t)sourceEnd)
	{
		const float averageWordLength = (float)sumWordLen / (float)estimatedWordCount;
		estimatedWordCount =
			((size_t)sourceEnd - (size_t)wordInitial) / ROUND(averageWordLength);
		list->Next = MemChunkAlloc(sizeof(WordNode_t) * estimatedWordCount);
		assert(list->Next != NULL);
		list = list->Next;
		goto FILL_WORDS_LIST;
	}
	list = list - 1;
	list->Next = NULL;
	BuildDictionary(wordList, negativeWordList, wordsCount);
	fclose(negativeFile);
LB_CLOSE_POSITIVE_WORDS_FILE:
	fclose(positiveFile);
	return ret;
}

key_t CreateKey(const char* word)
{
	key_t keyH = word[1] << 8;
	key_t keyL = word[2];
	// Exceptional characters: '?'
	if (keyH == ('?' << 8)) { keyH = 0xff00; }
	if (keyL == '?') { keyL = 0xff; }
	return keyH | keyL;
}

void BuildDictionary(pWordNode_t const positiveList, pWordNode_t const negativeList, size_t wordsCount)
{
	pWordNode_t positiveBak = positiveList;
	pWordNode_t negativeBak = negativeList;
	pWordNode_t attributeContext = NULL;
	size_t memIndex = 0;
	size_t maxIndex = ((wordsCount + 7) & ~7) >> 3;
	pDictionaryNode_t currentMemBlock = MemChunkAlloc(sizeof(DictionaryNode_t) * maxIndex);
	pDictionaryNode_t* setter = Dictionary;
	for (size_t i = 0; i < DICTIONARY_1D_SIZE; ++i, ++setter)
	{
		(*setter)->KeyValue = -1;
		(*setter)->Color = BLACK;
		(*setter)->Parent = (*setter)->Child[LEFT] = (*setter)->Child[RIGHT] = NULL;
	}
	if (StrComp(
		positiveBak->Word, positiveBak->Length,
		negativeBak->Word, negativeBak->Length) != -1)
	{
		attributeContext = positiveBak;
		positiveBak = positiveBak->Next;
	}
	else
	{
		attributeContext = negativeBak;
		negativeBak = negativeBak->Next;
	}

	unsigned int dictionaryIndex = DICTIONARY_1D_HASHING(*attributeContext->Word);
	Dictionary[dictionaryIndex]->KeyValue = CreateKey(attributeContext->Word);
	unsigned int indexBak = dictionaryIndex;

	int hitCount = 1;
	Dictionary[dictionaryIndex]->WordList = attributeContext;
	while (1)
	{
		if (positiveBak == negativeList)
		{
			attributeContext = attributeContext->Next = negativeBak;
			break;
		}
		if (negativeBak == NULL)
		{
			attributeContext = attributeContext->Next = positiveBak;
			break;
		}
		if (StrComp(
			positiveBak->Word, positiveBak->Length,
			negativeBak->Word, negativeBak->Length) != -1)
		{
			attributeContext = attributeContext->Next = positiveBak;
			positiveBak = positiveBak->Next;
		}
		else
		{
			attributeContext = attributeContext->Next = negativeBak;
			negativeBak = negativeBak->Next;
		}
		dictionaryIndex = DICTIONARY_1D_HASHING(*attributeContext->Word);
		if (*attributeContext->Word == 'f')
		{
			hitCount = hitCount;
		}
		if (indexBak != dictionaryIndex)
		{
			hitCount = 0;
			pDictionaryNode_t temp = Dictionary[dictionaryIndex];
			temp->KeyValue = CreateKey(attributeContext->Word);
			temp->WordList = attributeContext;
			indexBak = dictionaryIndex;
		}
		else if ((hitCount & 7) == 0)
		{
			pDictionaryNode_t newNode = currentMemBlock + memIndex;
			++memIndex;
			newNode->KeyValue = CreateKey(attributeContext->Word);
			newNode->WordList = attributeContext;
			newNode->Parent = newNode->Child[RIGHT] = newNode->Child[LEFT] = NULL;
			newNode->Color = RED;
			InsertLabel(Dictionary + dictionaryIndex, newNode);
		}
		++hitCount;
	}
	return;
}

void InsertLabel(pDictionaryNode_t* root, pDictionaryNode_t newNode)
{
	*root = _InsertLabelRecursion(*root, newNode);
	FixUp(root, newNode);
}

pDictionaryNode_t _InsertLabelRecursion(pDictionaryNode_t trav, pDictionaryNode_t temp)
{
	if (trav == NULL) { return temp; }
	unsigned char condition = trav->KeyValue < temp->KeyValue;
	if (temp->KeyValue == trav->KeyValue)
	{
		condition = 1 == StrComp(
			trav->WordList->Word, trav->WordList->Length,
			temp->WordList->Word, temp->WordList->Length);
	}
	trav->Child[condition] = _InsertLabelRecursion(trav->Child[condition], temp);
	trav->Child[condition]->Parent = trav;
	return trav;
}

void RotateRight(pDictionaryNode_t* root, pDictionaryNode_t temp)
{
	pDictionaryNode_t left = temp->Child[LEFT];
	temp->Child[LEFT] = left->Child[RIGHT];
	if (temp->Child[LEFT] != NULL) { temp->Child[LEFT]->Parent = temp; }
	left->Parent = temp->Parent;
	if (temp->Parent == NULL) { *root = left; }
	else if (temp == temp->Parent->Child[LEFT])
	{
		temp->Parent->Child[LEFT] = left;
	}
	else { temp->Parent->Child[RIGHT] = left; }
	left->Child[RIGHT] = temp;
	temp->Parent = left;
}
void RotateLeft(pDictionaryNode_t* root, pDictionaryNode_t temp)
{
	pDictionaryNode_t right = temp->Child[RIGHT];
	temp->Child[RIGHT] = right->Child[LEFT];
	if (temp->Child[RIGHT] != NULL) { temp->Child[RIGHT]->Parent = temp; }
	right->Parent = temp->Parent;
	if (temp->Parent == NULL) { *root = right; }
	else if (temp == temp->Parent->Child[LEFT])
	{
		temp->Parent->Child[LEFT] = right;
	}
	else { temp->Parent->Child[RIGHT] = right; }
	right->Child[LEFT] = temp;
	temp->Parent = right;
}
void FixUp(pDictionaryNode_t* root, pDictionaryNode_t temp)
{
	pDictionaryNode_t parent;
	pDictionaryNode_t grandParent;
	pDictionaryNode_t uncle;
	Color_t colorBak;
	while ((temp != (*root)) && (temp->Color != BLACK)
		&& (temp->Parent->Color == RED))
	{
		parent = temp->Parent;
		grandParent = temp->Parent->Parent;

		if (parent == grandParent->Child[LEFT])
		{
			uncle = grandParent->Child[RIGHT];

			if (uncle != NULL && uncle->Color == RED)
			{
				grandParent->Color = RED;
				parent->Color = uncle->Color = BLACK;
				temp = grandParent;
			}
			else
			{
				if (temp == parent->Child[RIGHT])
				{
					RotateLeft(root, parent);
					temp = parent;
					parent = temp->Parent;
				}
				RotateRight(root, grandParent);
				colorBak = parent->Color;
				parent->Color = grandParent->Color;
				grandParent->Color = colorBak;
				temp = parent;
			}
		}
		else
		{
			uncle = grandParent->Child[LEFT];
			if ((uncle != NULL) && (uncle->Color == RED))
			{
				grandParent->Color = RED;
				parent->Color = uncle->Color = BLACK;
				temp = grandParent;
			}
			else
			{
				if (temp == parent->Child[LEFT])
				{
					RotateRight(root, parent);
					temp = parent;
					parent = temp->Parent;
				}
				RotateLeft(root, grandParent);
				colorBak = parent->Color;
				parent->Color = grandParent->Color;
				grandParent->Color = colorBak;
				temp = parent;
			}
		}
	}

	(*root)->Color = BLACK;
}

pWordNode_t GetOnDictoinary(const char* word, int wordLen)
{
	key_t key = CreateKey(word);
	pDictionaryNode_t temp = Dictionary[DICTIONARY_1D_HASHING(*word)];
	int condition = 0;
	pWordNode_t ret = NULL;
	pWordNode_t nodeBak = NULL;
	if (temp->KeyValue == -1)
	{
		return NULL;
	}
	while (temp != NULL)
	{
		condition = 0;
		if (temp->KeyValue == key)
		{
			ret = temp->WordList;
			condition = StrComp(ret->Word, ret->Length, word, wordLen);
			if (condition == 0) { break; }
			condition = condition == 1;
		}
		condition = condition || temp->KeyValue < key;

		if (temp->Child[condition] == NULL)
		{
			if (condition)
			{
				ret = temp->WordList;
			}
			else
			{
				ret = nodeBak;
				nodeBak = NULL;
			}

			break;
		}
		if (condition) { nodeBak = temp->WordList; }
		temp = temp->Child[condition];
	}
	if (ret != NULL)
	{
		if (nodeBak != NULL)
		{
			for (size_t i = 0; i < 8 && nodeBak != NULL; i++)
			{
				int comp = StrComp(nodeBak->Word, nodeBak->Length, word, wordLen);
				if (comp == 0) { return nodeBak; }
				// if (comp == -1) { return NULL; }
				nodeBak = nodeBak->Next;
			}
		}
		for (size_t i = 0; i < 8 && ret != NULL; i++)
		{
			int comp = StrComp(ret->Word, ret->Length, word, wordLen);
			if (comp == 0) { return ret; }
			// if (comp == -1) { return NULL; }
			ret = ret->Next;
		}
		return NULL;
	}
	return ret;
}

void ReadReviewFile(const char* fileName)
{

	int count[2] = { 0, };
	FILE* reviewFile = fopen(fileName, "r");
	if (reviewFile == NULL)
	{
		printf("%s file was not found\n", fileName);
		return;
	}

	long fileLen = GetFileLength(reviewFile) + 1;
	char* reviewSource = malloc(fileLen);
	assert(reviewSource != NULL);
	fread(reviewSource, 1, fileLen, reviewFile);
	reviewSource[fileLen - 1] = 0;
	const char* word = reviewSource;
	void* sourceEnd = reviewSource + fileLen - 1;
	if (word == NULL) { goto LB_RET; }
	while ((size_t)word < (size_t)sourceEnd && *word)
	{
		int length = GetWordLength(word);
		if (0 < length)
		{
			pWordNode_t temp = GetOnDictoinary(word, length);
			if (temp == NULL)
			{
				temp = temp;
			}
			if (temp != NULL)
			{
				++count[temp->bPositive];
			RETEST_NEXT:
				if (temp->Next != NULL && StrComp(
					temp->Word, temp->Length,
					temp->Next->Word, temp->Next->Length) == 0
					&& temp->bPositive != temp->Next->bPositive)
				{
					++count[temp->Next->bPositive];
					temp = temp->Next;
					goto RETEST_NEXT;
				}
			}
			word = word + length;
		}
		else
		{
			word = word - length;
		}
	}
	printf("(%d, %d)%s is a %s assessment\n", count[POSITIVE], count[NEGATIVE], fileName, RATE[count[POSITIVE] < count[NEGATIVE]]);
LB_RET:
	free(reviewSource);
	fclose(reviewFile);
}
