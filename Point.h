#pragma once
#include <windef.h>


struct Point : POINT
{
	Point() = default;

	Point(LONG x, LONG y)
	{
		this->x = x;
		this->y = y;
	}


	bool operator==(const Point& other) const
	{
		return x == other.x && y == other.y;
	}

	bool operator!=(const Point& other) const
	{
		return x != other.x || y != other.y;
	}

};