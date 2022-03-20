#include <iostream>
#include "WindowsDesktopDuplicationManager.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

int main()
{
	WindowsDesktopDuplicationManager manager;
	manager.Update();
	manager.Update();
	manager.Update();
	manager.Update();
	manager.Update();
	manager.Release();

	std::getchar();
}