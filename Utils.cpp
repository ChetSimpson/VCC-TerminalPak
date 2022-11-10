#include "TerminalPak.h"
#include <Windows.h>
#include <stdexcept>


void AddMenuItem(const std::string& name, int id, ItemType type)
{
	DyanamicMenuCallback(name.c_str(), id, static_cast<int>(type));
}

void AddMenuSeparator()
{
	AddMenuItem("", 0, ItemType::Head);
}

std::string HtxLoadString(UINT id)
{
	std::string text;
	text.resize(256);

	auto size(LoadString(HtxCurrentModuleInstance, id, &text[0], text.size()));

	text.resize(size);

	return text;
}