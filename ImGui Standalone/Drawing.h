#ifndef DRAWING_H
#define DRAWING_H

#include "pch.h"
#include "UI.h"
#include <vector>
#include <string>
#include <ctime>

class Drawing
{
private:
	static LPCSTR lpWindowName;
	static ImVec2 vWindowSize;
	static ImGuiWindowFlags WindowFlags;
	static bool bDraw;

public:
	static void CleanupCPUUsage();
	static void Active();
	static bool isActive();
	static void InitCPUUsage();
	static float GetCPUUsagePercent();
	static void Draw();
	static void LoadImages();
};

#endif
