#include "console_game.h"
#include "Console.h"
#include <stdio.h>
#include <Windows.h>
#include <time.h>
#include <string.h>
#include "linkedlist.h"

const int FINAL_STAGE = 3;
const int enemySize = 30;

char screenBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];
Player player;
State currentState = START;
Enemy enemies[enemySize]{ 0 };

List bulletList;



const char* const stageInfoFileName = "StageInfo.txt";

int currentStage = 0;
char stageFileName[FINAL_STAGE][20];
int balance[30] = { 0 };
const int enemyFireRate = 1000;
time_t enemyFireTimer;

void MovePlayer();
void Draw();
void DrawCharactor(int x, int y, char c);
void DrawStartScene();
bool AllEnemyDead();
bool ReadCurrentStageInfo();
void MoveEnemy();
void MoveBullet();
void CheckHit();
void CheckEnemyHit();
void CheckPlayerHit();
void ResetGame();
void ShootBullet(int x, int y, bool myBullet);

bool GameInit()
{
	FILE* stageInfoFile;
	fopen_s(&stageInfoFile, stageInfoFileName, "r");
	if (stageInfoFile == nullptr) return false;
	cs_Initial();
	cs_ClearScreen();
	int idx = 0;
	fseek(stageInfoFile, 0, SEEK_END);
	int fileSize = ftell(stageInfoFile);
	rewind(stageInfoFile);
	char* buffer = (char*)malloc(fileSize);
	memset(buffer, 0, fileSize);
	if (buffer == nullptr) return false;
	fread_s(buffer, fileSize, fileSize, 1, stageInfoFile);
	size_t begin = 0, end = 0;
	buffer[fileSize] = '\0';
	while (buffer[begin] != '\0')
	{
		while (true)
		{
			if (buffer[end] == '\n' || buffer[end] == '\0') break;
			end++;
		}
		if (buffer[end] != '\0')
			buffer[end++] = '\0';
		else buffer[end] = '\0';
		printf("%s\n", &buffer[begin]);
		memcpy_s(stageFileName[idx], 20, &buffer[begin], end - begin);
		begin = end;
		idx++;
	}
	fclose(stageInfoFile);
	ListInit(&bulletList);
	player.x = 38;
	player.x = 18;
	srand(time(NULL));
	return true;
}

bool ReadCurrentStageInfo()
{
	FILE* stageInfo;
	fopen_s(&stageInfo, stageFileName[currentStage], "r");
	if (stageInfo == nullptr) return false;
	fseek(stageInfo, 0, SEEK_END);
	int fileSize = ftell(stageInfo);
	rewind(stageInfo);
	char* buffer = (char*)malloc(fileSize);
	if (buffer == nullptr)
	{
		fclose(stageInfo);
		return false;
	}
	fread_s(buffer, fileSize, fileSize, 1, stageInfo);
	size_t begin = 0, end = 0;
	char temp[20];
	int monsterIdx = 0;
	int cnt = 0;
	memset(enemies, 0, sizeof(Enemy) * enemySize);
	while (buffer[begin] != '\0')
	{
		while (true)
		{
			if (buffer[end] == '\n' || buffer[end] == '\0' || buffer[end] == ',') break;
			end++;
		}
		if (buffer[end] != '\0')
			buffer[end++] = '\0';
		memcpy_s(temp, 20, &buffer[begin], end - begin);
		begin = end;
		if (cnt == 0)
			enemies[monsterIdx].x = atoi(temp);
		else if (cnt == 1)
			enemies[monsterIdx].y = atoi(temp);
		else if (cnt == 2)
			enemies[monsterIdx].goingLeft = atoi(temp);
		enemies[monsterIdx].aliveFlag = true;
		cnt = (cnt + 1) % 3;

		if (cnt == 0) monsterIdx++;
	}
	player.x = 40;
	player.y = 18;

	fclose(stageInfo);
	return true;
}



void Load()
{
	ResetGame();
	if (ReadCurrentStageInfo())
	{
		currentState = PLAYING;
	}
}

void Play()
{
	MovePlayer();
	MoveEnemy();
	MoveBullet();
	CheckHit();
	Draw();
	if (AllEnemyDead())
	{
		
		currentState = State::LOAD;
		currentStage++;
		ResetGame();
	}
}

void ResetGame()
{
	cs_Initial();
	cs_ClearScreen();
	player.x = 38;
	player.y = 18;
	if (currentStage >= FINAL_STAGE)
	{
		currentState = END;
		currentStage = 0;
	}
	for (int i = 0; i < enemySize; i++)
		balance[i] = 0;
	Bullet* currentBullet;
	while (!ListIsEmpty(&bulletList))
	{
		currentBullet = (Bullet*)(bulletList.head->data);
		ListRemove(&bulletList, 0);
		free(currentBullet);
	}
}





void End()
{
	ResetGame();

	char endingScene[SCREEN_HEIGHT][SCREEN_WIDTH]{
		"********************************************************************************",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*                       ######                                                 *",
		"*                       #    #          #  #  #  ###                           *",
		"*                       ###### ######   ## # ## #   #                          *",
		"*                            # #    ##  # # # # ####                           *",
		"*                       ###### ###### # #  #  #  ####                          *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*                      #######                                                 *",
		"*                      #     # #   #  ###  # ###                               *",
		"*                      #     # #   # #   # ##                                  *",
		"*                      #     #  # #  ####  ##                                  *",
		"*                      #######   #    #### #                                   *",
		"*                                                                              *",
		"*                                                                              *",
		"*                         press enter to start scene                           *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"********************************************************************************",
	};
	memcpy_s(screenBuffer, SCREEN_HEIGHT * SCREEN_WIDTH, endingScene, SCREEN_HEIGHT * SCREEN_WIDTH);

	if (GetAsyncKeyState(VK_RETURN))
		currentState = State::START;
}

bool AllEnemyDead()
{
	for (size_t i = 0; i < enemySize; i++)
	{
		if (enemies[i].aliveFlag)
			return false;
	}
	return true;
}

void MoveEnemy()
{
	time_t currentTime = clock();
	for (size_t i = 0; i < enemySize; i++)
	{
		if (!enemies[i].aliveFlag) continue;
		if (enemies[i].goingLeft)
		{
			enemies[i].x++;
			balance[i]--;
			if (balance[i] <= -5)
				enemies[i].goingLeft = false;
		}
		else
		{
			enemies[i].x--;
			balance[i]++;
			if (balance[i] >= 5)
				enemies[i].goingLeft = true;
		}
		//각 적 기체당 1/100 확률 * 대략 초당 11~12프레임정도 초당 약 10퍼센트 확률로 발사
		if (rand() % 100 == 0 )
		{
			ShootBullet(enemies[i].x, enemies[i].y + 1, false);
		}
	}
}

void ShootBullet(int x, int y, bool myBullet)
{
	Bullet* newBullet = (Bullet*)malloc(sizeof(Bullet));
	if (newBullet == nullptr) return;
	newBullet->isMyBullet = myBullet;
	newBullet->x = x;
	newBullet->y = y;
	ListInsert(&bulletList, newBullet, bulletList.size);
}

void MoveBullet()
{
	Node* temp = bulletList.head;
	int i = 0;
	int removeIdx = 0;
	while (temp != nullptr)
	{
		i++;
		Bullet* pCurrentBullet = (Bullet*)(temp->data);
		if (pCurrentBullet == nullptr) return;
		if (pCurrentBullet->isMyBullet)
			pCurrentBullet->y--;
		else pCurrentBullet->y++;
		if (pCurrentBullet->y <= 0 || pCurrentBullet->y >= SCREEN_HEIGHT)
		{
			removeIdx = FindPos(&bulletList, pCurrentBullet, sizeof(Bullet));
			if (removeIdx != -1)
			{
				ListRemove(&bulletList, removeIdx);
				temp = bulletList.head;
				free(pCurrentBullet);
			}
		}
		if (temp != nullptr)
			temp = temp->next;
	}
}
void CheckHit()
{
	CheckEnemyHit();
	CheckPlayerHit();
}

void CheckEnemyHit()
{
	Node* temp= bulletList.head;
	while (temp != nullptr)
	{

		Bullet* pCurrentBullet = (Bullet*)(temp->data);

		if (pCurrentBullet == nullptr) return;

		if (!pCurrentBullet->isMyBullet)
		{
			temp = temp->next;
			continue;
		}

		for (int i = 0; i < enemySize; i++)
		{
			if (enemies[i].aliveFlag && enemies[i].x == pCurrentBullet->x && enemies[i].y == pCurrentBullet->y)
			{
				enemies[i].aliveFlag = false;
				int removeIdx = FindPos(&bulletList, pCurrentBullet, sizeof(Bullet));
				if (removeIdx != -1)
				{
					ListRemove(&bulletList, removeIdx);
					temp = bulletList.head;
					free(pCurrentBullet);
				}
				break;
			}
		}
		if (temp != nullptr)
			temp = temp->next;
	}
}

void CheckPlayerHit()
{
	Node* temp = bulletList.head;
	while (temp != nullptr)
	{
		Bullet* pCurrentBullet = (Bullet*)(temp->data);

		if (pCurrentBullet == nullptr) return;

		if (pCurrentBullet->isMyBullet)
		{
			temp = temp->next;
			continue;
		}

		if (player.x == pCurrentBullet->x && player.y == pCurrentBullet->y)
		{
			int removeIdx = FindPos(&bulletList, pCurrentBullet, sizeof(Bullet));
			if (removeIdx != -1)
			{
				if (temp != bulletList.head)
					temp = temp->prev;
				ListRemove(&bulletList, removeIdx);
				free(pCurrentBullet);
			}
			currentState = State::END;
		}
		temp = temp->next;
	}
}
State GetCurrentState()
{
	return currentState;
}
void Start()
{
	DrawStartScene();

	if (GetAsyncKeyState(VK_RETURN))
	{
		currentState = State::LOAD;
	}
}
//텍스트가 한글자씩 나오게 할라고...
char strs[20][81] = {
	"*                           p                                                  *", 
	"*                           pr                                                 *", 
	"*                           pre                                                *", 
	"*                           pres                                               *", 
	"*                           press                                              *", 
	"*                           press e                                            *", 
	"*                           press en                                           *", 
	"*                           press ent                                          *", 
	"*                           press ente                                         *", 
	"*                           press enter                                        *", 
	"*                           press enter t                                      *", 
	"*                           press enter to                                     *", 
	"*                           press enter to s                                   *", 
	"*                           press enter to st                                  *", 
	"*                           press enter to sta                                 *", 
	"*                           press enter to star                                *", 
	"*                           press enter to start                               *", 
	"*                           press enter to start.                              *", 
	"*                           press enter to start..                             *", 
	"*                           press enter to start...                            *", 
};
int textCounter = 0;
clock_t timer = clock();
int cycle;
void DrawStartScene()
{
	time_t currentTime = clock();
	if (currentTime - timer >= 1000)
	{
		if (textCounter >= 19)
		{
			textCounter = 19;
			if (cycle == 2)
			{
				textCounter = 0;
				cycle = 0;
			}
			cycle++;
		}
		timer = clock();
	}
	if (textCounter <= 18)
		textCounter++;
	char startScene[SCREEN_HEIGHT][SCREEN_WIDTH]{
		"********************************************************************************",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*             ###### #                    #    #                               *",
		"*             #      #                  #####                                  *",
		"*             ###### ###### ##### #####   #    #  ##### #####                  *",
		"*                  # #    # #   # #   #   #    #  #   # #   #                  *",
		"*             ###### #    # ##### #####   #    #  #   # #####                  *",
		"*                                                           #                  *",
		"*                                                       #####                  *",
		"*                                                                              *",
		"*                 #######                                                      *",
		"*                 #     # #####   #  #  #  ###                                 *",
		"*                 ####### #   #   ## # ## #   #                                *",
		"*                       # #   ##  # # # # ####                                 *",
		"*                 ####### ##### # #  #  #  ####                                *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"*                                                                              *",
		"********************************************************************************",
	};

	memcpy_s(screenBuffer, SCREEN_HEIGHT * SCREEN_WIDTH, startScene, sizeof(startScene));
	memcpy_s(screenBuffer[18], SCREEN_WIDTH, strs[textCounter], SCREEN_WIDTH);
}

void MovePlayer()
{
	if (GetAsyncKeyState(VK_LEFT) && player.x - 1 > 0)
	{
		player.x--;
	}
	if (GetAsyncKeyState(VK_RIGHT) && player.x + 2 < SCREEN_WIDTH - 1)
	{
		player.x++;
	}
	if (GetAsyncKeyState(VK_UP) && player.y - 1 > 0)
	{
		player.y--;
	}
	if (GetAsyncKeyState(VK_DOWN) && player.y + 1 < SCREEN_HEIGHT - 1)
	{
		player.y++;
	}
	if (GetAsyncKeyState(VK_CONTROL))
	{
		ShootBullet(player.x, player.y, true);
	}
}

void Draw()
{
	for (size_t i = 0; i < enemySize; i++)
	{
		if (enemies[i].aliveFlag)
		{
			DrawCharactor(enemies[i].x, enemies[i].y, 'E');
		}
	}
	Node* tempBulletNode = bulletList.head;
	while (tempBulletNode != nullptr)
	{
		//printf("bullet x : %d y : %d", ((Bullet*)(bulletList.head->data))->x, ((Bullet*)(bulletList.head->data))->y);
		Bullet* tempBulletPtr = (Bullet*)tempBulletNode->data;
		DrawCharactor(tempBulletPtr->x, tempBulletPtr->y, 'B');
		tempBulletNode = tempBulletNode->next;
	}
	DrawCharactor(player.x, player.y, 'O');
	DrawCharactor(69, 19, 's');
	DrawCharactor(70, 19, 't');
	DrawCharactor(71, 19, 'a');
	DrawCharactor(72, 19, 'g');
	DrawCharactor(73, 19, 'e');
	DrawCharactor(75, 19, currentStage + '0' + 1);
}

void DrawCharactor(int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= SCREEN_WIDTH - 1 || y >= SCREEN_HEIGHT)
		return;
	screenBuffer[y][x] = c;
}

void ClearBuffer()
{
	memset(screenBuffer, ' ', SCREEN_WIDTH * SCREEN_HEIGHT);

	for (int i = 0; i < SCREEN_HEIGHT; i++)
	{
		screenBuffer[i][SCREEN_WIDTH - 1] = '\0';
	}
	for (size_t i = 0; i < SCREEN_HEIGHT; i++)
	{
		screenBuffer[i][0] = '*';
		screenBuffer[i][SCREEN_WIDTH - 2] = '*';
	}
	for (size_t i = 0; i < SCREEN_WIDTH-1; i++)
	{
		screenBuffer[0][i] = '*';
		screenBuffer[SCREEN_HEIGHT - 1][i] = '*';
	}
}
void Render()
{
	cs_MoveCursor(0, 0);
	for (size_t i = 0; i < SCREEN_HEIGHT; i++)
	{
		cs_MoveCursor(0, i);
		printf("%s", screenBuffer[i]);
	}
}

void AdjustFrame()
{
	Sleep(80);
}
