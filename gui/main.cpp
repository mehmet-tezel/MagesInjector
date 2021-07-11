#include "guiglobals.h"
#include "resource.h"

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HICON hWindowIcon = NULL;

// Main code
//int main(int argc, char** argv)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    hWindowIcon = LoadIcon(GetModuleHandle(NULL), (LPCSTR)IDI_ICON1);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), hWindowIcon, NULL, NULL, NULL, _T("D11 1NJ3CT0R"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Mage's Injector"), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 435 - 1, 255, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL;

    readData();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    bool setMainWindowprops = true, setProcessWindowprops = true;
    bool mainWindow = true;

    listProcess();

    int selectedItem = 0;

    std::string* searchProcess = new std::string[200];

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Main Window
        {
            ImGui::Begin("MAGE'S D11 1NJ3CT0R", &mainWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

            if (setMainWindowprops) {
                ImGui::SetWindowPos(ImVec2(-1, 0));
                ImGui::SetWindowSize(ImVec2(420, 300));
                setMainWindowprops = false;
            }

            ImGui::SameLine(170);
            ImGui::Checkbox("Show Processes", &showProcessList);
            ImGui::SameLine(320);
            ImGui::Checkbox("Dark Mode", &darkMode);
            ImGui::SameLine(8);
            ImGui::Text("Load DLL");

            ImGui::PushItemWidth(380);
            ImGui::InputText("###1", &dllPath[0], sizeof(dllPath), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            dllButton = ImGui::Button("X##1"); // dllButton, opens operating system's file choose menu

            ImGui::Text("Choose Running Application (.exe)");
            ImGui::InputText("", &exePath[0], sizeof(exePath), ImGuiInputTextFlags_ReadOnly);
            ImGui::SameLine();
            exeButton = ImGui::Button("X##2"); // exeButton, opens operating system's file choose menu

            ImGui::Text("\n");
            ImGui::Text("DLL: ");
            ImGui::SameLine();
            ImGui::TextColored(dllColor, dllName.data());
            ImGui::Text("EXE: ");
            ImGui::SameLine();
            ImGui::TextColored(exeColor, exeName.data());
            ImGui::SameLine();
            ImGui::Text("PID:");
            ImGui::SameLine();
            ImGui::TextColored(pidColor, pidStr.data());
            ImGui::SameLine();
            ImGui::Text("Status:");
            ImGui::SameLine();
            ImGui::TextColored(statusColor, statusStr.data());
            ImGui::Text("\n");

            injectButton = ImGui::Button("INJECT", ImVec2(405, 30));

            ImGui::End();
        }

        // Process Window
        {
            if (processWindow) {
                
                ImGui::Begin("Process List", &processWindow);
                
                if (setProcessWindowprops) {
                    ImGui::SetWindowSize(ImVec2(417, 0));
                    ImGui::SetWindowPos(ImVec2(1, 0));
                    setProcessWindowprops = false;
                }

                ImGui::PushItemWidth(340);
                if (ImGui::InputText("", searchProcess))
                {
                    processName = storeProcessName;
                    processName = search(searchProcess[0], processName);
                }

                /*
                if (searchProcess->empty())
                {
                    processName = storeProcessName;
                }
                */

                ImGui::SameLine();
                refreshButton = ImGui::Button("Refresh");

                ImGui::PushItemWidth(405);
                if (ImGui::ListBox("", &selectedItem, processName))
                {
                    exeName = processName[selectedItem];
                    getProcId(hwnd);
                }

                ImGui::End();
            }
        }

        setClient();
        checkButtons(hwnd);

        if (!mainWindow)
            done = true;

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    saveData();
    delete[] searchProcess;

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
