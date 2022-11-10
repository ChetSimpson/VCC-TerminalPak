#pragma once
#include "Point.h"


struct Rect : RECT
{
	Rect()
	{
		left = 0;
		top = 0;
		right = 0;
		bottom = 0;
	}

	Rect(LONG left, LONG top, LONG right, LONG bottom)
	{
		this->left = left;
		this->top = top;
		this->right = right;
		this->bottom = bottom;
	}

	Rect(POINT position, LONG width, LONG height)
	{
		this->left = position.x;
		this->top = position.y;
		this->right = position.x + width;
		this->bottom = position.y + height;
	}

	Rect(POINT topLeft, POINT bottomRight)
	{
		this->left = topLeft.x;
		this->top = topLeft.y;
		this->right = bottomRight.x;
		this->bottom = bottomRight.y;
	}

	LONG Width() const
	{
		return right - left;
	}

	LONG Height() const
	{
		return bottom - top;
	}

	void Offset(LONG x, LONG y)
	{
		::OffsetRect(this, x, y);
	}

	void AdjustForWindow(DWORD style, bool includeMenuBar = false)
	{
		::AdjustWindowRect(this, style, includeMenuBar);
	}

	void Shrink(LONG leftAmount, LONG topAmount, LONG rightAmount, LONG bottomAmount)
	{
		left += leftAmount;
		top += topAmount;
		right -= rightAmount;
		bottom -= bottomAmount;
	}

	void Shrink(LONG amount)
	{
		left += amount;
		top += amount;
		right -= amount;
		bottom -= amount;
	}
};