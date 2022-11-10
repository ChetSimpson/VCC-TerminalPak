#pragma once
#include <string>
#include <windows.h>


#ifdef TERMINALPAK_EXPORTS
#define TERMINALPAK_EXPORT extern "C" __declspec(dllexport)
#else
#define TERMINALPAK_EXPORT
#endif


enum class IOPorts
{
	ConfigStatus = 0xe0,
	CommandArgument = 0xe1,
	Command = 0xe2,
	WriteCharacter = 0xe3,
};


enum class IOCommands
{
	ShowConsole,
	HideConsole,
};


enum class ItemType
{
	Head,
	Slave,
	StandAlone
};


enum MenuItemIdentifier
{
	ID_MENUITEMBEGIN = 5001,
	ID_SEPARATOR = ID_MENUITEMBEGIN,
	ID_OPENCONSOLE,
	ID_CLOSECONSOLE
};


typedef void(*DYNAMICMENUCALLBACK)(const char *, int, int);

extern DYNAMICMENUCALLBACK DyanamicMenuCallback;
extern HINSTANCE HtxCurrentModuleInstance;

void AddMenuSeparator();
void AddMenuItem(const std::string& name, int id, ItemType type);
std::string HtxLoadString(UINT id);

