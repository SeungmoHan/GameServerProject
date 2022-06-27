#include "TitleObject"
#include <memory.h>


void TitleObject::Update()
{

}


void TitleObject::Render()
{
	ScreenBuffer::GetInstance()->DrawSprite(titleScene);
}

TitleObject::TitleObject(int x, int y, ObjectInterface::ObjectType t, ObjectManager* m) : ObjectInterface(x, y, t, m)
{

}
