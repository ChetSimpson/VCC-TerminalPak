#include <Windows.h>
#include <Windowsx.h>
#include "TerminalWindow.h"
#include "TerminalPak.h"
#include "Resources/resource.h"
#include <stdexcept>

#pragma comment(lib, "Winmm.lib")


const std::string TerminalWindow::Details::WindowClassName = "TerminalPakWindow";
const std::string TerminalWindow::Details::FontName = "Modern DOS 8x16";
const std::array<COLORREF, 8> TerminalWindow::Details::Palette =
{
	RGB(0, 0, 0),		//	Black
	RGB(170, 0, 0),		//	Red
	RGB(0, 170, 0),		//	Green,
	RGB(170, 85, 0),	//	Yellow,
	RGB(0, 0, 170),		//	Blue,
	RGB(170, 0, 170),	//	Magenta,
	RGB(0, 170, 170),	//	Cyan,
	RGB(170, 170, 170),	//	White
};



#pragma region Windows_Support_And_Message_Handlers
bool TerminalWindow::RegisterWindowClasses()
{
	WNDCLASSEX windowClass;

	//	See if the class is already registered
	if (::GetClassInfoEx(HtxCurrentModuleInstance, Details::WindowClassName.c_str(), &windowClass))
	{
		return true;
	}

	windowClass.cbSize			= sizeof(WNDCLASSEX);
	windowClass.style			= CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc		= StartWndProc;
	windowClass.cbClsExtra		= 0;
	windowClass.cbWndExtra		= sizeof(TerminalWindow*);	//	FIXME: Is this needed?
	windowClass.hInstance		= HtxCurrentModuleInstance;
	windowClass.hIcon			= LoadIcon(HtxCurrentModuleInstance, MAKEINTRESOURCE(IDI_TERMINALWINDOW));
	windowClass.hCursor			= LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground	= static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
	windowClass.lpszMenuName	= nullptr;
	windowClass.lpszClassName	= Details::WindowClassName.c_str();
	windowClass.hIconSm			= LoadIcon(HtxCurrentModuleInstance, MAKEINTRESOURCE(IDI_TERMINALWINDOW));


	if (!::RegisterClassEx(&windowClass))
	{
		MessageBox(nullptr, "Unable to register window class for terminal window", "Startup Error", MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}


bool TerminalWindow::Create()
{
	RegisterWindowClasses();

	Rect windowRect(
		0,
		0,
		Details::Width + (Details::Margin * 2),
		Details::Height + (Details::Margin * 2));
	windowRect.AdjustForWindow(Details::WindowStyle);

	auto windowHandle = ::CreateWindow(
		Details::WindowClassName.c_str(),
		HtxLoadString(IDS_TERMINALWINDOWNAME).c_str(),
		Details::WindowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT, 
		windowRect.Width(),
		windowRect.Height(),
		nullptr,
		nullptr,
		HtxCurrentModuleInstance,
		this);

	return windowHandle != nullptr;
}


void TerminalWindow::UnregisterWindowClasses()
{
	::UnregisterClass(Details::WindowClassName.c_str(), HtxCurrentModuleInstance);
}


bool TerminalWindow::LoadFont(UINT resourceId)
{
	auto moduleInstance(reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(handle_, GWL_HINSTANCE)));

	if (fontResource_ != INVALID_HANDLE_VALUE)
	{
		::RemoveFontMemResourceEx(fontResource_);
		fontResource_ = INVALID_HANDLE_VALUE;
	}

	auto fontResource = ::FindResource(moduleInstance, MAKEINTRESOURCE(resourceId), "BINARY");
	if (fontResource)
	{
		auto fontMemoryHandle = ::LoadResource(moduleInstance, fontResource);
		if (fontMemoryHandle != nullptr)
		{
			void* FntData = ::LockResource(fontMemoryHandle);
			DWORD nFonts = 0;
			DWORD len = SizeofResource(moduleInstance, fontResource);
			fontResource_ = ::AddFontMemResourceEx(FntData, len, nullptr, &nFonts); // Install font!
		}
	}

	return true;
}


LRESULT TerminalWindow::ProcMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_USER_RESET:
		ResetTerminal();
		return 0;

	case WM_USER_WRITECHAR:
		WriteCharacter(static_cast<unsigned char>(wParam));
		return 0;
	}

	return Window::ProcMessage(message, wParam, lParam);
}


void TerminalWindow::QueueReset()
{
	::PostMessage(handle_, WM_USER_RESET, 0, 0);
}


void TerminalWindow::QueueCharacter(unsigned char ch)
{
	::PostMessage(handle_, WM_USER_WRITECHAR, ch, 0);
}


bool TerminalWindow::OnCreate(CREATESTRUCT& /*createStruct*/)
{
	if (!LoadFont(IDF_TERMINALFONT))
	{
		return false;
	}

	font_ = ::CreateFont(
		-Details::FontHeight,
		-Details::FontWidth,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		ANSI_CHARSET,
		OUT_TT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		FIXED_PITCH | FF_DONTCARE,
		Details::FontName.c_str());


	renderRect_ = GetClientRect();
	auto hdc(GetDC(handle_));

	renderDC_ = CreateCompatibleDC(hdc);
	if (renderDC_ == nullptr)
	{
		OutputDebugString("failed to create the backDC");
		return false;
	}

	renderBitmap_ = CreateCompatibleBitmap(hdc, renderRect_.Width(), renderRect_.Height());
	if (renderBitmap_ == nullptr)
	{
		OutputDebugString("failed to create window bitmap");
		return false;
	}

	DeleteObject(SelectObject(renderDC_, font_));
	DeleteObject(SelectObject(renderDC_, renderBitmap_));

	ReleaseDC(handle_, hdc);

	ResetTerminal();

	return true;
}


bool TerminalWindow::OnClose()
{
	ShowWindow(SW_HIDE);

	return false;
}


void TerminalWindow::OnDestroy()
{
	Window::OnDestroy();

	if (fontResource_ != INVALID_HANDLE_VALUE)
	{
		::RemoveFontMemResourceEx(fontResource_);
		fontResource_ = INVALID_HANDLE_VALUE;
	}

	if (font_ != nullptr)
	{
		::DeleteFont(font_);
		font_ = nullptr;
	}

	if (renderDC_ != nullptr)
	{
		::DeleteDC(renderDC_);
		renderDC_ = nullptr;
	}
}


bool TerminalWindow::OnEraseBackground(HDC /*dc*/)
{
	//	Return false to prevent call of default window proc (skipping erasing)
	return false;
}


bool TerminalWindow::OnPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(handle_, &ps);

	BitBlt(
		hdc,
		renderRect_.left,
		renderRect_.top,
		renderRect_.Width(),
		renderRect_.Height(),
		renderDC_,
		0,
		0,
		SRCCOPY);

	EndPaint(handle_, &ps);

	return true;
}


void TerminalWindow::OnChar(UINT ch, UINT repeatCount, UINT /*flags*/)
{
	//switch (ch)
	//{
	//case 'L'-'A'+1:
	//	EraseCurrentLine();
	//	return;

	//case 'O'-'A'+1:
	//	EraseToBeginningOfCurrentLine();
	//	return;

	//case 'H'-'A'+1:
	//	HomeCursor();
	//	return;

	//case 'X'-'A'+1:
	//	SaveCursorPosition();
	//	return;

	//case 'Y'-'A'+1:
	//	RestoreCursorPosition();
	//	return;
	//}


	while (repeatCount--)
	{
		WriteCharacter(static_cast<unsigned char>(ch));
	}
}


void TerminalWindow::OnKeyDown(UINT ch, UINT /*repeatCount*/, UINT /*flags*/)
{
	switch (ch)
	{
	case VK_LEFT:
		MoveCursorLeft();
		break;

	case VK_RIGHT:
		MoveCursorRight();
		break;

	case VK_UP:
		MoveCursorUp();
		break;

	case VK_DOWN:
		MoveCursorDown();
		break;
	}
}
#pragma endregion








#pragma region Terminal_Initialization_And_Reset
void TerminalWindow::ResetTerminal()
{
	enableCursor_ = true;
	enableWrap_ = true;
	savedCursorPosition_ = { 0, 0 };

	processingEscapeSequence_ = false;
	SetBackgroundColor(Color::Black);
	SetTextColor(Color::White);
	ClearScreen();
}
#pragma endregion




#pragma region Character_Writing_And_Support

//	VT52 escape codes
// ------------------------------------
// 
//	+	[ESC] A		Cursor up				Move cursor one line upwards.
//											Does not cause scrolling when it reaches the top.
//	+	[ESC] B		Cursor down				Move cursor one line downwards.
//	+	[ESC] C		Cursor right			Move cursor one column to the right.
//	+	[ESC] D		Cursor left				Move cursor one column to the left.
//	-	[ESC] F		Enter graphics mode		Use special graphics character set, VT52 and later.
//	-	[ESC] G		Exit graphics mode		Use normal US/UK character set
//	+	[ESC] H		Cursor home				Move cursor to the upper left corner.
//		[ESC] I		Reverse line feed		Insert a line above the cursor, then move the cursor into it.
//											May cause a reverse scroll if the cursor was on the first line.
//		[ESC] J		Clear to end of screen	Clear screen from cursor onwards.
//	+	[ESC] K		Clear to end of line	Clear line from cursor onwards.
//		[ESC] L		Insert line				Insert a line.
//		[ESC] M		Delete line				Remove line.
//		[ESC] Yrc	Set cursor position		Move cursor to position c,r, encoded as single characters.
//											The VT50H also added the "SO" command that worked identically,
//											providing backward compatibility with the VT05.
//		[ESC] Z		ident					Identify what the terminal is, see notes below.
//		[ESC] =		Alternate keypad		Changes the character codes returned by the keypad.
//		[ESC] >		Exit alternate keypad	Changes the character codes returned by the keypad.
//
//	GEMTOS/TOS extensions
// ------------------------------------
// 
//	+	[ESC] E		Clear screen				Clear screen and place cursor at top left corner. Essentially the same as [ESC] H [ESC] J
//	*	[ESC] b#	Foreground color			Set text colour to the selected value
//	*	[ESC] c#	Background color			Set background colour
//		[ESC] d		Clear to start of screen	Clear screen from the cursor up to the home position.
//	+	[ESC] e		Enable cursor				Makes the cursor visible on the screen.
//	+	[ESC] f		Disable cursor				Makes the cursor invisible.
//	+	[ESC] j		Save cursor					Saves the current position of the cursor in memory, TOS 1.02 and later.
//	+	[ESC] k		Restore cursor				Return the cursor to the settings previously saved with j.
//	+	[ESC] l		Clear line					Erase the entire line and positions the cursor on the left.
//	+	[ESC] o		Clear to start of line		Clear current line from the start to the left side to the cursor.
//		[ESC] p		Reverse video				Switch on inverse video text.
//		[ESC] q		Normal video				Switch off inverse video text.
//	+	[ESC] v		Wrap on						Enable line wrap, removing the need for CR/LF at line endings.
//	+	[ESC] w		Wrap off					Disable line wrap.
void TerminalWindow::WriteCharacter(unsigned char ch)
{
	if (processingEscapeSequence_)
	{
		processingEscapeSequence_ = false;
		switch (ch)
		{
		case 'A':
			MoveCursorUp();
			return;

		case 'B':
			MoveCursorDown();
			return;

		case 'C':
			MoveCursorRight();
			return;

		case 'D':
			MoveCursorLeft();
			return;

		case 'H':
			HomeCursor();
			return;

		case 'K':
			EraseToEndOfCurrentLine();
			return;

		//	GEMTOS/TOS extension
		case 'E':
			ClearScreen();
			return;

		case 'e':
			EnableCursor();
			return;

		case 'f':
			DisableCursor();
			return;

		case 'j':
			SaveCursorPosition();
			return;

		case 'k':
			RestoreCursorPosition();
			return;

		case 'l':
			EraseCurrentLine();
			return;

		case 'o':
			EraseToBeginningOfCurrentLine();
			return;

		case 'v':
			EnableEndOfLineWrap();
			return;

		case 'w':
			DisableEndOfLineWrap();
			return;
		}
	}

	switch (ch)
	{
	case 0x07:
		PlayBell();
		return;

	case 0x08:
		Backspace();
		return;

	case 0x09:
		TabCursorRight();
		return;

	case 0x0a:
		MoveCursorDown();
		return;

	case 0x0c:
		ClearScreen();
		return;

	case 0x0d:
		CarriageReturn();
		return;

	case 0x27:
		processingEscapeSequence_ = true;
		return;
	}

	SetCharacter(ch, cursorPosition_);

	++cursorPosition_.x;
	OnCursorAdvancedHorizontally();
}


void TerminalWindow::PlayBell()
{
	// Find the WAVE resource. 
	HRSRC hResInfo = ::FindResource(HtxCurrentModuleInstance, MAKEINTRESOURCE(IDW_TERMINALBELL), "WAVE"); 
	if (hResInfo == nullptr)
	{
		return;
	}

	// Load the WAVE resource. 
	auto hRes = LoadResource(HtxCurrentModuleInstance, hResInfo); 
	if (hRes == nullptr)
	{
		return;
	}

	//	Lock and play it
	auto lpRes = LockResource(hRes); 
	if (lpRes != nullptr)
	{ 
		::sndPlaySound(static_cast<LPCSTR>(lpRes), SND_MEMORY | SND_ASYNC | SND_NODEFAULT); 
		UnlockResource(hRes); 
	} 

	FreeResource(hRes); 
}


void TerminalWindow::Backspace()
{
	const auto originalPosition(cursorPosition_);
	//	If we're not at the beginning of the line we just move the cursor
	if (cursorPosition_.x > 0)
	{
		--cursorPosition_.x;
	}

	//	The cursor is at the beginning of the line. If we're not at the top of
	//	the screen move to the end of the line above
	else if (cursorPosition_.y > 0)
	{
		cursorPosition_.x = Details::Columns - 1;
		--cursorPosition_.y;
	}

	//	We're at the beginning of the line and at the top of the screen
	//  so we just alert the user and do nothing
	else
	{
		PlayBell();
	}

	//	If teh cursor moves erase the character we are now at and update
	//	the cursor
	if (originalPosition != cursorPosition_)
	{
		SetCharacter(' ', cursorPosition_);
		UpdateCursor();
	}
}


void TerminalWindow::CarriageReturn()
{
	cursorPosition_.x = 0;
	if (cursorPosition_.y == Details::Rows - 1)
	{
		ScrollUp();
	}
	else
	{
		++cursorPosition_.y;
	}

	UpdateCursor();	//	FIXME: ScrollUp should already update the cursor
}
#pragma endregion




#pragma region Feature_Control
void TerminalWindow::EnableEndOfLineWrap()
{
	enableWrap_ = true;
}


void TerminalWindow::DisableEndOfLineWrap()
{
	enableWrap_ = false;
}
#pragma endregion




#pragma region Cursor_Management
void TerminalWindow::EnableCursor()
{
	EraseCursor();
	enableCursor_ = false;
}


void TerminalWindow::DisableCursor()
{
	enableCursor_ = true;
	DrawCursor();
}


void TerminalWindow::SaveCursorPosition()
{
	savedCursorPosition_ = cursorPosition_;
}


void TerminalWindow::RestoreCursorPosition()
{
	cursorPosition_ = savedCursorPosition_;
	UpdateCursor();
}


void TerminalWindow::HomeCursor()
{
	cursorPosition_ = { 0,0 };
	UpdateCursor();
}


void TerminalWindow::TabCursorRight()
{
	cursorPosition_.x = (cursorPosition_.x + 8) & ~7;
	OnCursorAdvancedHorizontally();
}


void TerminalWindow::MoveCursorLeft()
{
	if (cursorPosition_.x != 0)
	{
		--cursorPosition_.x;
		UpdateCursor();
	}
}


void TerminalWindow::MoveCursorRight()
{
	if (cursorPosition_.x != Details::Columns - 1)
	{
		++cursorPosition_.x;
		UpdateCursor();
	}
}


void TerminalWindow::MoveCursorUp()
{
	if (cursorPosition_.y != 0)
	{
		--cursorPosition_.y;
		UpdateCursor();
	}
}


void TerminalWindow::MoveCursorDown()
{
	if (cursorPosition_.y != Details::Rows - 1)
	{
		++cursorPosition_.y;
		UpdateCursor();
	}
}


void TerminalWindow::OnCursorAdvancedHorizontally()
{
	if (cursorPosition_.x > Details::Columns)
	{
		throw std::runtime_error("Cursor advanced too far");
	}


	if (enableWrap_ && cursorPosition_.x == Details::Columns)
	{
		CarriageReturn();
	}
	else if (cursorPosition_.x == Details::Columns)
	{
		--cursorPosition_.x;
		UpdateCursor(true);
	}
	else
	{
		UpdateCursor();
	}
}
#pragma endregion




#pragma region Color_Control
void TerminalWindow::SetBackgroundColor(Color color)
{
	backgroundColor_ = color;
}


void TerminalWindow::SetTextColor(Color color)
{
	textColor_ = color;
}
#pragma endregion




#pragma region Erase_Support
void TerminalWindow::ClearScreen()
{
	textBuffer_.clear();
	textBuffer_.resize(Details::Rows);
	for (auto& row : textBuffer_)
	{
		row = std::vector<TermChar>(Details::Columns, TermChar(' ', backgroundColor_, textColor_));
	}

	//	Clear the backbuffer
	::SetBkColor(renderDC_, Details::Palette[static_cast<unsigned>(backgroundColor_)]);
	::ExtTextOut(renderDC_, 0, 0, ETO_OPAQUE, &renderRect_, NULL, 0, NULL);

	cursorPosition_ = { 0, 0 };
	lastCursorPosition_ = cursorPosition_;

	UpdateCursor(true);
	InvalidateRect(nullptr, false);
}


void TerminalWindow::EraseCurrentLine()
{
	EraseOnCurrentLine(0, Details::Columns, true);	
}


void TerminalWindow::EraseToBeginningOfCurrentLine()
{
	EraseOnCurrentLine(0, cursorPosition_.x, false);
}


void TerminalWindow::EraseToEndOfCurrentLine()
{
	EraseOnCurrentLine(cursorPosition_.x, Details::Columns, false);
}


void TerminalWindow::EraseOnCurrentLine(int xStart, int xEnd, bool moveCursor)
{
	auto& line(textBuffer_[cursorPosition_.y]);

	auto begin(line.begin());
	std::advance(begin, xStart);

	auto end(line.begin());
	std::advance(end, xEnd);

	std::fill(begin, end, TermChar(' ', backgroundColor_, textColor_));
	
	auto rect(Rect(
		CharToScreenPosition(Point(xStart, cursorPosition_.y)),
		CharToScreenPosition(Point(xEnd, cursorPosition_.y + 1))));

	EraseCursor();
	::SetBkColor(renderDC_, Details::Palette[static_cast<unsigned>(backgroundColor_)]);
	::ExtTextOut(renderDC_, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);	//	Erase the area with the character (for fonts that don't)

	if (moveCursor)
	{
		cursorPosition_.x = 0;
	}

	UpdateCursor(true);	//	We need to force the update since the cursor position and last position may be the same

	InvalidateRect(rect, false);
}
#pragma endregion




#pragma region Scrolling
void TerminalWindow::ScrollUp()
{
	auto dstRect(renderRect_);
	dstRect.Shrink(Details::Margin, Details::Margin, Details::Margin, Details::Margin + Details::FontHeight);

	EraseCursor();
	BitBlt(
		renderDC_,
		dstRect.left,
		dstRect.top,
		dstRect.Width(),
		dstRect.Height(),
		renderDC_,
		dstRect.left,
		dstRect.top + Details::FontHeight,
		SRCCOPY);

	dstRect.top = dstRect.bottom;
	dstRect.bottom += Details::FontHeight;

	::SetBkColor(renderDC_, Details::Palette[static_cast<unsigned>(backgroundColor_)]);
	::ExtTextOut(renderDC_, 0, 0, ETO_OPAQUE, &dstRect, NULL, 0, NULL);	//	Erase the area with the character (for fonts that don't)

	std::move(++textBuffer_.begin(), textBuffer_.end(), textBuffer_.begin());
	textBuffer_.back() = std::vector<TermChar>(Details::Columns, TermChar(' ', backgroundColor_, textColor_));

	UpdateCursor(true);

	InvalidateRect(nullptr, false);
}
#pragma endregion




#pragma region Character_Rendering_And_Position_Support
void TerminalWindow::SetCharacter(unsigned char ch, Point position)
{
	auto newChar(TermChar(ch, backgroundColor_, textColor_));
	textBuffer_[position.y][position.x] = newChar;

	DrawCharacter(newChar, position);
}


void TerminalWindow::DrawCharacter(const TermChar& ch, Point position) const
{
	position = CharToScreenPosition(position);

	auto drawRect = Rect(position, Details::FontWidth, Details::FontHeight);

	::SetTextColor(renderDC_, Details::Palette[static_cast<unsigned>(ch.textColor)]);
	::SetBkColor(renderDC_, Details::Palette[static_cast<unsigned>(ch.bkgColor)]);

	::ExtTextOut(renderDC_, 0, 0, ETO_OPAQUE, &drawRect, NULL, 0, NULL);	//	Erase the area with the character (for fonts that don't)
	::DrawTextA(renderDC_, reinterpret_cast<const char*>(&ch.ch), 1, &drawRect, DT_SINGLELINE | DT_NOPREFIX);

	InvalidateRect(drawRect, false);
}


void TerminalWindow::EraseCursor()
{
	if (enableCursor_)
	{
		const auto& cursorChar = textBuffer_[lastCursorPosition_.y][lastCursorPosition_.x];
		DrawCharacter(cursorChar, lastCursorPosition_);
	}
}


void TerminalWindow::DrawCursor()
{
	if (enableCursor_)
	{
		auto cursorChar = textBuffer_[lastCursorPosition_.y][lastCursorPosition_.x];
		std::swap(cursorChar.bkgColor, cursorChar.textColor);
		DrawCharacter(cursorChar, lastCursorPosition_);
	}
}


void TerminalWindow::UpdateCursor(bool forceUpdate)
{
	if (enableCursor_ && (forceUpdate || lastCursorPosition_ != cursorPosition_))
	{
		EraseCursor();
		lastCursorPosition_ = cursorPosition_;
		DrawCursor();
	}
	else
	{
		lastCursorPosition_ = cursorPosition_;
	}
}


Point TerminalWindow::CharToScreenPosition(Point charPosition) const
{
	charPosition.x = (charPosition.x * Details::FontWidth) + Details::Margin;
	charPosition.y = (charPosition.y * Details::FontHeight) + Details::Margin;

	return charPosition;
}
#pragma endregion

