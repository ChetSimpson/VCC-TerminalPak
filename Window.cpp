#include "Windows.h"
#include "Window.h"
#include <stdexcept>

HINSTANCE HtxCurrentModuleInstance = nullptr;

void Window::Attach(HWND windowHandle)
{
	if (handle_ != nullptr)
	{
		throw std::runtime_error("Window already attached");
	}

	handle_ = windowHandle;
	::SetWindowLongPtr(windowHandle, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(ThunkWndProc));
	::SetWindowLongPtr(windowHandle, GWL_USERDATA, reinterpret_cast<LONG_PTR>(this));
}


void Window::Detach()
{
	if (handle_ == nullptr)
	{
		throw std::runtime_error("No window handle is attached");
	}

	::SetWindowLongPtr(handle_, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(DefWindowProc));
	::SetWindowLongPtr(handle_, GWL_USERDATA, 0);
	handle_ = nullptr;
}




LRESULT Window::DefaultProc()
{
	return ::DefWindowProc(handle_, defaultProcMessage_, defaultProcWParam_, defaultProcLParam_);
}


void Window::DestroyWindow()
{
	::DestroyWindow(handle_);
}


bool Window::ShowWindow(int cmdShow)
{
	return ::ShowWindow(handle_, cmdShow) != 0;
}


bool Window::IsVisible() const
{
	return ::IsWindowVisible(handle_) != 0;
}


bool Window::GetClientRect(RECT& rect) const
{
	return ::GetClientRect(handle_, &rect) != 0;
}


Rect Window::GetClientRect() const
{
	Rect rect;

	GetClientRect(rect);

	return rect;
}


void Window::InvalidateRect(const Rect& rect, bool erase) const
{
	::InvalidateRect(handle_, &rect, erase);
}


void Window::InvalidateRect(const std::nullptr_t rect, bool erase) const
{
	::InvalidateRect(handle_, rect, erase);
}



LRESULT Window::ProcMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	defaultProcMessage_ = message;
	defaultProcWParam_ = wParam;
	defaultProcLParam_ = lParam;

	switch (message)
	{
	case WM_CREATE:
		if (!OnCreate(*reinterpret_cast<CREATESTRUCT*>(lParam)))
		{
			return -1;
		}
		break;

	case WM_CLOSE:
		if (!OnClose())
		{
			return 0;
		}
		break;

	case WM_ERASEBKGND:
		if(!OnEraseBackground(reinterpret_cast<HDC>(wParam)))
		{ 
			return 1;
		}
		break;

	case WM_PAINT:
		if (OnPaint())
		{
			return 0;
		}
		break;

	case WM_DESTROY:
		OnDestroy();
		break;

	case WM_CHAR:
		OnChar(wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;

	case WM_TIMER:
		OnTimer(wParam);
		return 0;

	case WM_KEYDOWN:
		OnKeyDown(wParam, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_KEYUP:
		OnKeyUp(wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	}

	return DefaultProc();
}


bool Window::OnCreate(CREATESTRUCT& /*createStruct*/)
{
	return true;
}


bool Window::OnClose()
{
	return true;
}


void Window::OnDestroy()
{}


bool Window::OnEraseBackground(HDC /*dc*/)
{
	return true;
}


bool Window::OnPaint()
{
	return false;
}


void Window::OnChar(UINT /*ch*/, UINT /*repeatCount*/, UINT /*flags*/)
{}


void Window::OnKeyDown(UINT /*ch*/, UINT /*repeatCount*/, UINT /*flags*/)
{}


void Window::OnKeyUp(UINT /*ch*/, UINT /*repeatCount*/, UINT /*flags*/)
{}


void Window::OnTimer(UINT /*id*/)
{}


LRESULT CALLBACK Window::StartWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CREATE)
	{
		auto createStruct(reinterpret_cast<CREATESTRUCT*>(lParam));
		auto terminalProcObject(static_cast<Window*>(createStruct->lpCreateParams));

		terminalProcObject->Attach(hwnd);

		return ThunkWndProc(hwnd, message, wParam, lParam);
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK Window::ThunkWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto window = reinterpret_cast<Window*>(::GetWindowLongPtr(hwnd, GWL_USERDATA));

	auto result(window->ProcMessage(message, wParam, lParam));
	
	if (message == WM_DESTROY)
	{
		window->Detach();
	}

	return result;
}
