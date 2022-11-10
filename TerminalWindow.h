#pragma once
#include "Window.h"
#include <vector>
#include <string>
#include <array>


class TerminalWindow : public Window
{
protected:

	struct Details
	{
		static const std::string WindowClassName;
		static const std::string FontName;
		static const int Margin = 4;
		static const int Columns = 80;
		static const int Rows = 30;
		static const int FontWidth = 8;
		static const int FontHeight = 16;
		static const int Width = FontWidth * Columns;
		static const int Height = FontHeight * Rows;
		static const int WindowStyle = WS_CAPTION | WS_SYSMENU;
		static const std::array<COLORREF, 8> Palette;
	};


public:

	enum class Color : unsigned char
	{
		Black,
		Red,
		Green,
		Yellow,
		Blue,
		Magenta,
		Cyan,
		White
	};


	static const UINT WM_USER_RESET = WM_USER;
	static const UINT WM_USER_WRITECHAR = WM_USER+1;

public:

	static bool RegisterWindowClasses();
	static void UnregisterWindowClasses();
	
public:

	bool Create();

	bool OnCreate(CREATESTRUCT& createStruct) override;
	bool OnClose() override;
	void OnDestroy() override;
	bool OnEraseBackground(HDC dc) override;
	bool OnPaint() override;
	void OnChar(UINT ch, UINT repeatCount, UINT flags) override;
	void OnKeyDown(UINT ch, UINT repeatCount, UINT flags) override;

	void QueueReset();
	void QueueCharacter(unsigned char ch);


protected:

	bool LoadFont(UINT resourceId);
	LRESULT ProcMessage(UINT message, WPARAM wParam, LPARAM lParam) override;



	struct TermChar
	{
		TermChar(unsigned char ch, Color bkgColor, Color textColor)
			:
			ch(ch),
			bkgColor(bkgColor),
			textColor(textColor)
		{}

		unsigned char	ch;
		Color			bkgColor;
		Color			textColor;
	};


	//	Terminal control
	virtual void ResetTerminal();

	//	Character writing and general control code support
	virtual void WriteCharacter(unsigned char ch);
	virtual void PlayBell();
	virtual void Backspace();
	virtual void CarriageReturn();

	//	Feature control
	virtual void EnableEndOfLineWrap();
	virtual void DisableEndOfLineWrap();

	//	Cursor management
	virtual void EnableCursor();
	virtual void DisableCursor();
	virtual void SaveCursorPosition();
	virtual void RestoreCursorPosition();
	virtual void HomeCursor();
	virtual void TabCursorRight();
	virtual void MoveCursorLeft();
	virtual void MoveCursorRight();
	virtual void MoveCursorUp();
	virtual void MoveCursorDown();
	virtual void OnCursorAdvancedHorizontally();

	//	Color and visual controls
	virtual void SetBackgroundColor(Color id);
	virtual void SetTextColor(Color id);

	//	Erase support
	virtual void ClearScreen();
	virtual void EraseCurrentLine();
	virtual void EraseToBeginningOfCurrentLine();
	virtual void EraseToEndOfCurrentLine();
	virtual void EraseOnCurrentLine(int xStart, int xEnd, bool moveCursor);

	//	Scrolling
	virtual void ScrollUp();

	//	Terminal util functions
	Point CharToScreenPosition(Point charPosition) const;
	virtual void SetCharacter(unsigned char ch, Point position);
	virtual void DrawCharacter(const TermChar& ch, Point position) const;
	virtual void EraseCursor();
	virtual void DrawCursor();
	virtual void UpdateCursor(bool forceUpdate = false);


protected:

	//	Render state
	HANDLE	fontResource_ = INVALID_HANDLE_VALUE;
	HFONT	font_ = nullptr;
	HDC		renderDC_ = nullptr;
	HBITMAP	renderBitmap_ = nullptr;
	Rect	renderRect_;

	//	Terminal config
	bool	enableCursor_ = true;
	bool	enableWrap_ = true;
	Color	backgroundColor_ = Color::Black;
	Color	textColor_ = Color::White;

	//	Terminal state
	Point	cursorPosition_;
	Point	lastCursorPosition_;
	Point	savedCursorPosition_;
	bool	processingEscapeSequence_ = false;
	std::vector<std::vector<TermChar>>	textBuffer_;
};
