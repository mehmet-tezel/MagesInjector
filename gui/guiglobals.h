#pragma once

// imgui
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "imgui_stdlib.h" // inputtext std::string support

// std
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

// os
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

#include <tchar.h>
#include <tlhelp32.h>
#include <WtsApi32.h>
#pragma comment(lib, "Wtsapi32.lib")

// addons

static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static ImVec4 dllColor, exeColor, pidColor, statusColor;

static bool darkMode = false;
static bool showProcessList = false;
static bool processWindow = false;
static int injectStatus = 2;

static std::string dllPath, exePath;
static std::string dllName, exeName;
static std::string pidStr, statusStr = "Waiting for injection...";

static bool dllButton = false, exeButton = false;
static bool isDllOK = false, isExeOK = false;
static bool injectButton = false;
static bool refreshButton = false;

static DWORD pid;
static int intPid;

static std::vector<std::string> processName;
static std::vector<std::string> storeProcessName;

// wrapper from https://git.epicsparrow.com/epicsparrow/imgui/commit/46a96af3df414dac429f00968538b76f15909011
static auto vector_getter = [](void* vec, int idx, const char** out_text)
{
	auto& vector = *static_cast<std::vector<std::string>*>(vec);
	if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
	*out_text = vector.at(idx).c_str();
	return true;
};

namespace ImGui {

	bool ListBox(const char* label, int* current_item, std::vector<std::string>& items)
	{
		if (items.empty()) { return false; }
		return ListBox(label, current_item, vector_getter, static_cast<void*>(&items), items.size());
	}
}

static std::vector<std::string> search(std::string& src, const std::vector<std::string>& proc)
{
	std::vector<std::string> res;
	for (const std::string& str : proc)
		if (str.find(src) != std::string::npos)
			res.push_back(str);
	return res;
}

static void getProcId(HWND hWnd) {

	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry))
		{
			do
			{
				if (!_stricmp(procEntry.szExeFile, exeName.data()))
				{
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));

		}
	}

	CloseHandle(hSnap);

	std::ostringstream ostr;
	ostr << procId;

	if (procId <= 0) {
		MessageBox(hWnd, "Run the application first!", "WARNING", MB_OK | MB_ICONERROR);
		//pidColor = ImVec4(1.f, 0.f, 0.f, 1.f);
		pidStr = "?";
		isExeOK = false;
	}
	else {
		//pidColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		pid = procId;
		pidStr = ostr.str();
		isExeOK = true;
	}
}

static void listProcess() {
	
	WTS_PROCESS_INFO* pWPIs = NULL;
	DWORD dwProcCount = 0;
	if (WTSEnumerateProcesses(WTS_CURRENT_SERVER_HANDLE, NULL, 1, &pWPIs, &dwProcCount))
	{
		for (DWORD i = 0; i < dwProcCount; i++)
			processName.push_back(pWPIs[i].pProcessName);
		
		storeProcessName = processName;
	}

	//Free memory
	if (pWPIs)
	{
		WTSFreeMemory(pWPIs);
		pWPIs = NULL;
	}
}

static bool inject() {

	HANDLE hEXE, hThread;

	PVOID address;
	SIZE_T nbytesWritten;

	hEXE = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

	if (hEXE == NULL)
	{
		std::cout << GetLastError() << std::endl;
		std::cout << "Error 1" << std::endl;
		return 0;
	}

	address = VirtualAllocEx(hEXE, NULL, strlen(dllPath.c_str()) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (address == NULL)
	{
		std::cout << GetLastError() << std::endl;
		std::cout << "Error 2" << std::endl;
		return 0;
	}

	if (!WriteProcessMemory(hEXE, address, dllPath.c_str(), strlen(dllPath.c_str()) + 1, &nbytesWritten))
	{
		std::cout << GetLastError() << std::endl;
		std::cout << "Error 3" << std::endl;
		return 0;
	}

	hThread = CreateRemoteThread(hEXE, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, address, 0, NULL);

	if (hThread == NULL)
	{
		std::cout << GetLastError() << std::endl;
		std::cout << "Error 4" << std::endl;
		return 0;
	}

	//MessageBoxA(NULL, "DLL is Activated!", exeName.c_str(), MB_OK | MB_ICONINFORMATION);
}

static void setClient(){

	if (darkMode) {
		ImGui::StyleColorsDark();
		clear_color = ImVec4(0.f, 0.1f, 0.2f, 1.00f);
		dllColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		exeColor = ImVec4(0.f, 1.f, 0.f, 1.f);

		if (pid > 0)
			pidColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		else
			pidColor = ImVec4(1.f, 0.f, 0.f, 1.f);

		if (injectStatus == 0)
			statusColor = ImVec4(1.f, 0.f, 0.f, 1.f);
		else if (injectStatus == 1)
			statusColor = ImVec4(0.f, 1.f, 0.f, 1.f);
		else
			statusColor = ImVec4(1.f, 1.f, 0.f, 1.f);
	}
	else {
		ImGui::StyleColorsLight();
		clear_color = ImVec4(1.f, 1.f, 1.f, 1.00f);
		dllColor = ImVec4(0.f, 0.f, 1.f, 1.f);
		exeColor = ImVec4(0.f, 0.f, 1.f, 1.f);

		if (pid > 0)
			pidColor = ImVec4(0.f, 0.f, 1.f, 1.f);
		else
			pidColor = ImVec4(1.f, 0.f, 0.f, 1.f);

		if (injectStatus == 0)
			statusColor = ImVec4(1.f, 0.f, 0.f, 1.f);
		else if (injectStatus == 1)
			statusColor = ImVec4(0.f, 0.f, 1.f, 1.f);
		else
			statusColor = ImVec4(0.f, 0.f, 0.f, 1.f);
	}
}

static void readData() {

	std::ifstream file("data.ini");
	if (!file.is_open()) {

		std::ofstream createFile("data.ini");
		createFile << "Dark Mode: " << darkMode;
		createFile.close();
	}
	else {

		std::string fileStr, var1;
		std::getline(file, fileStr);
		std::stringstream sStream(fileStr);

		sStream >> var1 >> var1 >> var1;
		darkMode = stoi(var1);
	}

	file.close();

	setClient();
}

static void saveData() {

	std::ofstream saveFile("data.ini");
	saveFile << "Dark Mode: " << darkMode;
	saveFile.close();
}

static void openFileDLL(HWND hWnd) {

	OPENFILENAMEA ofn = { 0 };
	char szFile[200] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));

	// Initialize remaining fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "DLL Files (*.dll)\0*.dll\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		dllPath = ofn.lpstrFile;

		std::string::size_type filePos = dllPath.rfind('\\');
		if (filePos != std::string::npos)
			++filePos;
		else
			filePos = 0;

		dllName = dllPath.substr(filePos);
		isDllOK = true;
	}
	else
		isDllOK = false;
}

static void openFileEXE(HWND hWnd) {

	OPENFILENAMEA ofn = { 0 };
	char szFile[200] = { 0 };
	ZeroMemory(&ofn, sizeof(OPENFILENAMEA));

	// Initialize remaining fields of OPENFILENAME structure
	ofn.lStructSize = sizeof(OPENFILENAMEA);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "Exe Files (*.exe)\0*.exe\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileNameA(&ofn) == TRUE)
	{
		exePath = ofn.lpstrFile;

		std::string::size_type filePos = exePath.rfind('\\');
		if (filePos != std::string::npos)
			++filePos;
		else
			filePos = 0;

		exeName = exePath.substr(filePos);
	}
}

static bool checkButtons(HWND hWnd) {

	if (showProcessList)
		processWindow = true;
	else
		processWindow = false;

	if (dllButton) {
		openFileDLL(hWnd);
		dllButton = false;
	}

	if (exeButton) {
		openFileEXE(hWnd);
		getProcId(hWnd);
		exeButton = false;
	}

	if (refreshButton) {
		processName.clear();
		listProcess();
	}

	if (injectButton) {

		if (pid <= 0) {
			MessageBox(hWnd, "Run the application first!", "WARNING", MB_OK | MB_ICONERROR);
			return false;
		}

		if (isDllOK && isExeOK) {
			if (inject()) {
				injectStatus = 1;
				statusStr = "INJ3CT3D!";
			}
			else {
				injectStatus = 0;
				statusStr = "Failed to inject dll!";
				return false;
			}
		}
		else
			return false;
	}

	return injectButton;
}