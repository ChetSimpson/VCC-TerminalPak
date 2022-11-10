#pragma once
#include "Rect.h"
#include "Point.h"


class Window
{
public:

	void Attach(HWND hwnd);
	void Detach();

	void DestroyWindow();
	bool ShowWindow(int cmdShow);
	bool IsVisible() const;
	bool GetClientRect(RECT& rect) const;
	Rect GetClientRect() const;
	void InvalidateRect(const Rect& rect, bool erase) const;
	void InvalidateRect(const std::nullptr_t rect, bool erase) const;

	virtual bool OnCreate(CREATESTRUCT& createStruct);
	virtual bool OnClose();
	virtual void OnDestroy();
	virtual bool OnEraseBackground(HDC dc);
	virtual bool OnPaint();
	virtual void OnChar(UINT ch, UINT repeatCount, UINT flags);
	virtual void OnKeyDown(UINT ch, UINT repeatCount, UINT flags);
	virtual void OnKeyUp(UINT ch, UINT repeatCount, UINT flags);
	virtual void OnTimer(UINT id);


protected:

	LRESULT DefaultProc();
	virtual LRESULT ProcMessage(UINT message, WPARAM wParam, LPARAM lParam);

	static LRESULT CALLBACK StartWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK ThunkWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


protected:

	HWND	handle_ = nullptr;
	UINT	defaultProcMessage_ = 0;
	WPARAM	defaultProcWParam_ = 0;
	LPARAM	defaultProcLParam_ = 0;

};