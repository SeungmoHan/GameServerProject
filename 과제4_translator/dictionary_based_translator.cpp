#include <Windows.h>
#include <stdio.h>

constexpr int ENG = 0;
constexpr int KOR = 1;
constexpr int LANGUAGE_SIZE = 2;
constexpr int DICTIONALRY_SIZE = 5;
constexpr int WORD_LENGTH = 30;
constexpr int INPUT_SENTENCE_SIZE = 128;
//[�ܾ� ��ġ][��� ����] = �ܾ�(���ܾ�)
char dictionary[DICTIONALRY_SIZE][LANGUAGE_SIZE][WORD_LENGTH]{ {"i","��"},{"student","�л�"},{"a","�ϳ���"},{"eat","�Դ�"},{"dinner","����"} };

char* Translate(char* enlishSentence);

int main()
{
	while (true)
	{
		printf("���� ���� �Է�(�ִ� 127��)\n");
		char buffer[INPUT_SENTENCE_SIZE];
		gets_s(buffer, INPUT_SENTENCE_SIZE - 1);
		if (strlen(buffer) == 0)
		{
			system("cls");
			continue;
		}
		char* afterTranslate = Translate(buffer);
		if (afterTranslate == nullptr)
		{
			system("cls");
			continue;
		}
		printf("%s\n\n", afterTranslate);
		free(afterTranslate);
		system("pause");
		system("cls");
	}
}
char* Translate(char* enlishSentence)
{
	constexpr int retSize = 128;
	char* ret = (char*)malloc(retSize);
	if (!ret) return ret;
	int currentCopiedIdx = 0;
	char* p1;
	char* p2;
	p1 = p2 = enlishSentence;
	while (true)
	{
		char word[30]{ 0 };
		while (*p2 != ' ' && *p2 != '\0') p2++;
		memcpy_s(word, 30, p1, p2 - p1);
		bool hasWord = false;
		int wordLength;
		for (size_t i = 0; i < DICTIONALRY_SIZE; i++)
		{
			if (strcmp(dictionary[i][ENG], word) == 0)
			{
				hasWord = true;
				wordLength = strlen(dictionary[i][KOR]);
				memcpy_s(ret + currentCopiedIdx, retSize - currentCopiedIdx, dictionary[i][KOR], wordLength);
				currentCopiedIdx += wordLength;
				ret[currentCopiedIdx++] = ' ';
				break;
			}
		}
		if (!hasWord)
		{
			wordLength = strlen(word);
			memcpy_s(ret + currentCopiedIdx, retSize - currentCopiedIdx, word, wordLength);
			currentCopiedIdx += wordLength;
			ret[currentCopiedIdx++] = ' ';
		}
		ret[currentCopiedIdx] = '\0';
		if (*p2 == '\0') break;
		p2++;
		p1 = p2;
	}
	return ret;
}