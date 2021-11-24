#include <memory.h>
#include "EndingObject"

void EndingObject::Update(){}

void EndingObject::Render()
{
	ScreenBuffer::GetInstance()->DrawSprite(this->endingScene);
}

EndingObject::EndingObject(int x, int y, ObjectInterface::ObjectType t, ObjectManager* m,bool win) : ObjectInterface(x,y,t,m),win(win)
{
	if (win)
		memcpy_s(endingScene[20], bufferWidth, "                        you win                                                 ", bufferWidth);
	else
		memcpy_s(endingScene[20], bufferWidth, "                        you lose                                                ", bufferWidth);
}