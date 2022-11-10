#include <windows.h>
#include "TerminalWindow.h"
#include "TerminalPak.h"
#include "Resources/resource.h"

static TerminalWindow MainTerminalWindow;
DYNAMICMENUCALLBACK DyanamicMenuCallback = [](const char*, int, int) {};

static const int MAX_LOADSTRING = 100;	//	FIXME: This is defined everywhere!

BOOL WINAPI DllMain(HINSTANCE moduleInstance, DWORD fdwReason, LPVOID /*lpReserved*/)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		HtxCurrentModuleInstance = moduleInstance;
		MainTerminalWindow.Create();
		break;

	case DLL_PROCESS_DETACH:
		MainTerminalWindow.DestroyWindow();
		TerminalWindow::UnregisterWindowClasses();
		break;
	}

	return TRUE;
}



TERMINALPAK_EXPORT void ModuleName(char *moduleName, char *catalogId, DYNAMICMENUCALLBACK dyanamicMenuCallback)
{
	DyanamicMenuCallback = dyanamicMenuCallback;
	LoadString(HtxCurrentModuleInstance, IDS_MODULENAME, moduleName, MAX_LOADSTRING);
	LoadString(HtxCurrentModuleInstance, IDS_CATALOGID, catalogId, MAX_LOADSTRING);
}


TERMINALPAK_EXPORT void SetIniPath(const char* /*iniFilePath*/)
{
	AddMenuSeparator();

	AddMenuItem(HtxLoadString(IDS_MAINMENUNAME), 5000, ItemType::Head);
	AddMenuItem(HtxLoadString(IDS_OPENCONSOLE), ID_OPENCONSOLE, ItemType::Slave);
	AddMenuItem(HtxLoadString(IDS_CLOSECONSOLE), ID_CLOSECONSOLE, ItemType::Slave);
}


TERMINALPAK_EXPORT void ModuleConfig(unsigned char menuId)
{
	switch (menuId + ID_MENUITEMBEGIN - 1)
	{
	case ID_OPENCONSOLE:
		MainTerminalWindow.ShowWindow(SW_SHOW);
		break;

	case ID_CLOSECONSOLE:
		MainTerminalWindow.ShowWindow(SW_HIDE);
		break;
	}
}


TERMINALPAK_EXPORT void ModuleReset()
{
	MainTerminalWindow.QueueReset();
}


TERMINALPAK_EXPORT void PackPortWrite(unsigned char port, unsigned char data)
{
	static unsigned char commandArgument = 0;

	switch (static_cast<IOPorts>(port))
	{
	case IOPorts::ConfigStatus:
		break;

	case IOPorts::CommandArgument:
		commandArgument = data;
		break;

	case IOPorts::Command:
		switch (static_cast<IOCommands>(data))
		{
		case IOCommands::ShowConsole:
			MainTerminalWindow.ShowWindow(SW_SHOW);
			break;

		case IOCommands::HideConsole:
			MainTerminalWindow.ShowWindow(SW_HIDE);
			break;
		}
		break;

	case IOPorts::WriteCharacter:
		//	We need to queue the character instead of directly writing is because the CPU 
		//	is emulated in a different thread
		MainTerminalWindow.QueueCharacter(data);
		break;
	}
}
