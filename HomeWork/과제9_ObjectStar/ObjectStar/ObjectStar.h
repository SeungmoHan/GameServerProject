#pragma once
#ifndef __OBJECT_STAR_HEADER__
#define __OBJECT_STAR_HEADER__

class BaseObject
{
public:
	virtual void Action() = 0;
	virtual void Draw() = 0;
	bool RemoveFlag() const { return removeFlag; };

protected:
	int xPos = 0;
	bool removeFlag = false;
};

class OneStar : public BaseObject
{
public:
	virtual void Action() override;
	virtual void Draw() override;
};

class TwoStar : public BaseObject
{
public:
	virtual void Action() override;
	virtual void Draw() override;
};

class ThreeStar : public BaseObject
{
public:
	virtual void Action() override;
	virtual void Draw() override;
};

#endif // !__OBJECT_STAR_HEADER__
