#include "Drawing.h"
#include <windows.h>
#include <pdh.h>
#include <PdhMsg.h>
#pragma comment(lib, "pdh.lib")

LPCSTR Drawing::lpWindowName = "To Do List!";
ImVec2 Drawing::vWindowSize = { 350, 50 };
ImGuiWindowFlags Drawing::WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar;
// Vars
bool Drawing::bDraw = true;
bool Draw1 = true;
bool Draw_List = true;
bool Draw_Add_Task = true;

// LIST
ID3D11ShaderResourceView* ListIcon = nullptr;
ID3D11ShaderResourceView* ListIconClicked = nullptr;
ID3D11ShaderResourceView* LIST_ICO = nullptr;

// ADD
ID3D11ShaderResourceView* AddIcon = nullptr;
ID3D11ShaderResourceView* AddIconClicked = nullptr;
ID3D11ShaderResourceView* ADD_ICO = nullptr;

// Task vars 
char task[512] = "";
int minute = 0;
int hour = 0;

// Hour limits
int hour_min = 0;
int hour_max = 24;

// minutes limits
int minute_min = 0;
int minute_max = 59;

// Alerta 
static bool showNotification = false;
static float notificationTime = 0.0f;
static std::string notificationMsg;

// CPU
PDH_HQUERY cpuQuery;
PDH_HCOUNTER cpuTotal;

std::string notificationText = "";

void Drawing::CleanupCPUUsage()
{
	if (cpuQuery) {
		PdhCloseQuery(cpuQuery);
		cpuQuery = nullptr;
		cpuTotal = nullptr;
	}
}


struct ToDo {
	char task[512];
	int min;
	int hour;
};

std::vector<ToDo> toDoList;

void Drawing::Active()
{
	bDraw = true;
}

bool Drawing::isActive()
{
	return bDraw == true;
}

void ShowNotification(const std::string& msg) {
	notificationMsg = msg;
	showNotification = true;
	notificationTime = ImGui::GetTime(); // Hora de inicio
}

void Drawing::InitCPUUsage() {
	PdhOpenQuery(NULL, NULL, &cpuQuery);
	PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);
}

float Drawing::GetCPUUsagePercent() {
	static float lastCPU = 0.0f;
	static double lastUpdateTime = 0.0;
	double now = ImGui::GetTime();

	// Solo actualiza cada 0.8 seg para suavidad y evitar Sleep en UI
	const double UPDATE_INTERVAL = 0.8;

	if (cpuQuery && cpuTotal && (now - lastUpdateTime > UPDATE_INTERVAL))
	{
		PdhCollectQueryData(cpuQuery);
		// Recolecta dos veces: primera sin formatear, segunda valida
		Sleep(100); // Necesario para un delta válido entre samples (PDH necesita al menos 100ms)
		PdhCollectQueryData(cpuQuery);

		PDH_FMT_COUNTERVALUE counterVal;
		if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal) == ERROR_SUCCESS)
			lastCPU = static_cast<float>(counterVal.doubleValue);

		lastUpdateTime = now;
	}
	return lastCPU;
}

float GetRamUsagePercent() {
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	DWORDLONG total = memInfo.ullTotalPhys;
	DWORDLONG used = total - memInfo.ullAvailPhys;
	return static_cast<float>(used * 100.0 / total);
}

// Función para dibujar la notificación, llámala cada frame en tu render loop
void RenderNotification() {
	const float displayDuration = 3.0f; // Total seconds
	const float fadeTime = 0.25f;       // Fade in/out time en segundos

	if (showNotification) {
		float elapsed = ImGui::GetTime() - notificationTime;
		if (elapsed > displayDuration) {
			showNotification = false;
			return;
		}

		// Fade-in, stay, fade-out
		float alpha = 1.0f;
		if (elapsed < fadeTime) alpha = elapsed / fadeTime;
		else if (elapsed > displayDuration - fadeTime) alpha = (displayDuration - elapsed) / fadeTime;

		// Fondo semi-claro más visible
		ImU32 bgColor = IM_COL32(255, 236, 150, int(230 * alpha)); // Amarillo pastel claro y opacidad animada
		ImU32 borderColor = IM_COL32(228, 183, 18, int(255 * alpha)); // Amarillo fuerte para el borde

		ImGui::SetNextWindowBgAlpha(0.97f * alpha);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 3.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
		ImGui::PushStyleColor(ImGuiCol_Border, borderColor);
		ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(70, 49, 6, int(255 * alpha))); // Marrón oscuro

		// Ubicación: centro-base (puedes ajustar "y" para subir/bajar)
		ImVec2 windowSize(400, 70);
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		float y = center.y + ImGui::GetMainViewport()->Size.y * 0.35f; // Más cerca de la base
		ImGui::SetNextWindowPos(ImVec2(center.x - windowSize.x / 2, 900), ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize);

		ImGui::Begin("##Notificacion", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize);

		// Centrar texto y hacerlo grande
		ImGui::SetCursorPosY(28);
		ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize(notificationMsg.c_str()).x) / 2);
		ImGui::SetWindowFontScale(1.3f);    // Hacer el texto grande
		ImGui::Text("%s", notificationMsg.c_str());
		ImGui::SetWindowFontScale(1.0f);    // Restaurar font scale

		ImGui::End();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(3);
	}
}


void DrawGlowingRectRelative(ImVec2 p0, ImVec2 p1, ImColor color, float glow_intensity, int glow_layers)
{
	// p0 y p1: coordenadas relativas al área interna de la ventana ImGui

	// Calcula la posición absoluta en pantalla desde el área cliente de la ventana
	ImVec2 window_pos = ImGui::GetWindowPos();     // Esquina superior izquierda de la ventana (incluido el título y bordes)
	ImVec2 content_pos = ImGui::GetCursorScreenPos(); // Esquina superior izquierda del área interna de la ventana (donde empiezan los widgets)

	ImVec2 base_p0 = ImVec2(content_pos.x + p0.x, content_pos.y + p0.y);
	ImVec2 base_p1 = ImVec2(content_pos.x + p1.x, content_pos.y + p1.y);

	// Dibuja el glow como antes, pero usando el draw list de la ventana actual (¡esto mantiene la posición relativa correcta!)
	ImDrawList* window_draw_list = ImGui::GetWindowDrawList();

	// Rectángulo base
	window_draw_list->AddRect(base_p0, base_p1, color, 0.0f, 0, 2.0f);

	// Glow por capas, igual que antes
	for (int i = 0; i < glow_layers; ++i)
	{
		float thickness = glow_intensity * (1.0f + i * 0.35f);
		float alpha = color.Value.w * (1.0f - (float)i / glow_layers) * 0.3f;
		ImColor glow_color = ImColor(color.Value.x, color.Value.y, color.Value.z, alpha);

		window_draw_list->AddRect(
			ImVec2(base_p0.x - i, base_p0.y - i),
			ImVec2(base_p1.x + i, base_p1.y + i),
			glow_color,
			0.0f, 0, thickness);
	}

	// Opcional: reserva espacio en el layout de ImGui para este rectángulo
	ImGui::Dummy(ImVec2(p1.x - p0.x, p1.y - p0.y));
}

void drawList() {
	if (Draw_List) {
		ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowPos(ImVec2(50, 100), ImGuiCond_Once);
		ImGuiWindowClass window_class;
		window_class.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&window_class);
		ImGui::Begin("Your To-Do's!", &Draw_List);

		// Si no hay tareas
		if (toDoList.empty()) {
			ImGui::Text("No tienes tareas pendientes!");
		}
		else {
			// Copia para saber si alguna tarea se elimina (índice a borrar)
			int taskToDelete = -1;

			// Recorre todas las tareas
			for (size_t i = 0; i < toDoList.size(); ++i) {
				ImGui::PushID((int)i);  // Evita ID duplicados

				// Dibuja la tarea
				ImGui::Text("%s", toDoList[i].task);

				ImGui::SameLine();

				// Muestra hora:minuto alineada y con formato (ejemplo: 08:09)
				char timeStr[16];
				snprintf(timeStr, sizeof(timeStr), "%02d:%02d", toDoList[i].hour, toDoList[i].min);
				ImGui::TextDisabled(" %s", timeStr);

				ImGui::SameLine();

				// Botón "Eliminar" (puedes cambiar el texto a un ícono con unicode o usar imágenes)
				if (ImGui::Button("Eliminar")) {
					taskToDelete = (int)i;   // Marca la tarea para borrar
				}

				ImGui::PopID();
			}

			// Elimina la tarea fuera del bucle por seguridad
			if (taskToDelete >= 0 && taskToDelete < (int)toDoList.size()) {
				toDoList.erase(toDoList.begin() + taskToDelete);
				ShowNotification("Tarea eliminada!");
			}
		}

		ImGui::End();
	}
}

void soundAlarm() {
	int cont = 0;
	ShowNotification(notificationText);
	while (cont < 6) {
		Beep(2000, 300);
		Sleep(200);
		cont++;
	}
}

void checkForTasks() {
	int hora, minutos;
	std::time_t t = std::time(nullptr);
	std::tm now;

	localtime_s(&now, &t);
	hora = now.tm_hour;
	minutos = now.tm_min;

	for (size_t i = 0; i < toDoList.size(); ++i) {
		if (toDoList[i].min == minutos && toDoList[i].hour == hora) {
			notificationText = toDoList[i].task;
			toDoList.erase(toDoList.begin() + i);
			std::thread t(soundAlarm);
			t.detach();
		}

	}
}

void addTask() {
	if (Draw_Add_Task) {
		int hora, minutos;

		std::time_t t = std::time(nullptr);
		std::tm now;

		localtime_s(&now, &t);

		hora = now.tm_hour;
		hour_min = hora;
		if (hour > hora) {
			minute_min = 0;
		}
		else {
			minute_min = now.tm_min + 1;
		}

		ImGui::SetNextWindowSize(ImVec2(300, 250), ImGuiCond_Once);
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowPos(ImVec2(400, 100), ImGuiCond_Once);
		ImGuiWindowClass window_class;
		window_class.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
		ImGui::SetNextWindowClass(&window_class);
		ImGui::Begin("Add a task!", &Draw_Add_Task);
		{
			ImGui::Text("Nombre de la tarea: ");
			ImGui::Dummy(ImVec2(0, 5));
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 20);
			if (ImGui::InputText("##hol3mundo", task, sizeof(task))) {
				// Manejar el texto, pero por ahora no...
			}
			ImGui::Dummy(ImVec2(0, 5));
			ImGui::Text("Hora y minuto: ");
			ImGui::Dummy(ImVec2(0, 5));
			ImGui::SetNextItemWidth((ImGui::GetWindowWidth() / 2) - 10);
			ImGui::InputInt("##holamundo", &hour, 1);
			ImGui::SameLine();
			ImGui::SetNextItemWidth((ImGui::GetWindowWidth() / 2) - 10);
			ImGui::InputInt("##hol2mundo", &minute, 1);

			// Comprobar limites
			if (hour <= hour_min) { // hora
				hour = hour_min;
			}
			else if (hour >= hour_max) {
				hour = hour_max;
			}

			if (minute <= minute_min) { // Minutos
				minute = minute_min;
			}
			else if (minute >= minute_max) {
				minute = minute_max;
			}

			ImGui::Dummy(ImVec2(0, 10));

			// Botón de agregar la tarea
			ImGui::SetCursorPos(ImVec2(10 , ImGui::GetWindowHeight() - 40));
			if (ImGui::Button("Agregar tarea!", ImVec2(ImGui::GetWindowWidth() - 20, 20))) {
				if (!strcmp(task, "")) {
					ShowNotification("Por favor introduce un nombre...");
				}
				else {
					ToDo newTask;
					strcpy_s(newTask.task, task);
					newTask.min = minute;
					newTask.hour = hour;

					toDoList.push_back(newTask);
					ShowNotification("Tarea agregada!");
				}

			}

		}ImGui::End();
	}
}

// Prototipo, pásale tus datos reales
void waterMark(float cpuUsagePercent, float ramUsagePercent)
{
	// ------ PARÁMETROS ------
	static const char* markText = "To Do List";
	ImVec4 accent = ImVec4(77 / 255.f, 134 / 255.f, 240 / 255.f, 1.f);   // Azul moderno
	ImVec4 bg = ImVec4(25 / 255.f, 28 / 255.f, 40 / 255.f, 0.88f);       // Fondo oscuro
	ImVec4 shadow = ImVec4(25 / 255.f, 28 / 255.f, 40 / 255.f, 0.2f);    // Sombra bajo la card

	ImVec2 size(280, 30); 

	// Shadow "card"
	ImVec2 pos((1980 - size.x) - 60, 20);
	ImGui::GetForegroundDrawList()->AddRectFilled(
		ImVec2(pos.x + 2, pos.y + 4), ImVec2(pos.x + size.x + 2, pos.y + size.y + 4),
		ImGui::ColorConvertFloat4ToU32(shadow), 14, ImDrawFlags_RoundCornersAll);

	// ----- VENTANA -----
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(size);
	ImGui::SetNextWindowBgAlpha(bg.w);
	ImGuiWindowClass window_class;
	window_class.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
	ImGui::SetNextWindowClass(&window_class);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
	ImGui::Begin("Watermark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);

	// --- Texto de la app ---
	ImGui::TextColored(accent, "%s", markText);
	ImGui::SameLine();
	// Separador decorativo
	ImGui::TextDisabled("|");

	// --- Porcentaje de CPU y RAM ---
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(210, 255, 210, 255));
	ImGui::Text("CPU: %.1f%%  ", cpuUsagePercent);
	ImGui::SameLine();
	ImGui::Text("RAM: %.1f%%", ramUsagePercent);
	ImGui::PopStyleColor();

	ImGui::End();
	ImGui::PopStyleVar(2);
}



void Drawing::Draw()
{
	RenderNotification();
	drawList();
	addTask();
	waterMark(GetCPUUsagePercent() , GetRamUsagePercent());
	checkForTasks();
	if (Draw1) {
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 display_size = io.DisplaySize;
		float width = 1920;
		float height = 1080;
		if (isActive())
		{
			ImGui::SetNextWindowSize(vWindowSize, ImGuiCond_Once);
			ImGui::SetNextWindowBgAlpha(1.0f);
			ImGui::SetNextWindowPos(ImVec2((width * 0.5f) - (vWindowSize.x / 2.0f), 10.0f) , ImGuiCond_Always);
			ImGuiWindowClass window_class;
			window_class.ViewportFlagsOverrideSet = ImGuiViewportFlags_TopMost;
			ImGui::SetNextWindowClass(&window_class);
			ImGui::Begin(lpWindowName, &bDraw, WindowFlags);
			{
				// Borders
				DrawGlowingRectRelative(ImVec2(-10, ImGui::GetWindowHeight()), ImVec2(350, 40), ImColor(0.0f, 0.0f, 0.90f, 0.9f), 1.0f, 12);

				if (LIST_ICO) {
					ImVec2 listSize = ImVec2(22.5f, 22.5f);
					ImGui::SetCursorPos(ImVec2(25 , (ImGui::GetWindowHeight()/2) - (listSize.y / 2)));
					ImGui::Image((void*)LIST_ICO, listSize);
					if (ImGui::IsItemHovered()) {
						LIST_ICO = ListIconClicked;
						ImGui::SetTooltip("Alterna la vista de tareas!");
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					}
					else if (!ImGui::IsItemHovered() && LIST_ICO != ListIcon){
						LIST_ICO = ListIcon;
					}
					if (ImGui::IsItemClicked()) {
						Draw_List = !Draw_List;
					}
				}
				if (ADD_ICO) {
					ImVec2 addSize = ImVec2(22.5, 22.5f);
					ImGui::SetCursorPos(ImVec2(65, (ImGui::GetWindowHeight() / 2) - (addSize.y / 2)));
					ImGui::Image((void*)ADD_ICO, addSize);
					if (ImGui::IsItemHovered()) {
						ADD_ICO = AddIconClicked;
						ImGui::SetTooltip("Agregar una tarea!");
						ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
					}
					else if (!ImGui::IsItemHovered() && ADD_ICO != AddIcon) {
						ADD_ICO = AddIcon;
					}

					if (ImGui::IsItemClicked()) {
						Draw_Add_Task = !Draw_Add_Task;
					}
				}
			}
			ImGui::End();
		}
	}

	if (GetAsyncKeyState(VK_INSERT) & 1)
		Draw1 = !Draw1;
}

void Drawing::LoadImages()
{
	ListIcon = UI::LoadTextureFromFile("D:/Icons/menu.png",UI::pd3dDevice);
	IM_ASSERT(ListIcon != NULL);
	ListIconClicked = UI::LoadTextureFromFile("D:/Icons/menu_hov.png", UI::pd3dDevice);
	IM_ASSERT(ListIcon != NULL);
	LIST_ICO = ListIcon;

	// Add
	AddIcon = UI::LoadTextureFromFile("D:/Icons/add.png", UI::pd3dDevice);
	IM_ASSERT(AddIcon != NULL);
	AddIconClicked = UI::LoadTextureFromFile("D:/Icons/add_hov.png", UI::pd3dDevice);
	IM_ASSERT(AddIconClicked != NULL);
	ADD_ICO = AddIcon;
}
