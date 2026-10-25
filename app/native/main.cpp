// CyUI demo: a complete application shell (menu bar, top bar, sidebar,
// status bar, dock space, panels) with the CyUI gallery and theme editor.
// Win32 + Direct3D 11, from Dear ImGui's own example.

#include "Editor.h"
#include "Ipc.h"
#include <ImGuizmo.h>
#include "MaterialPreview.h"
#include "MeshPreview.h"
#include "TexturePreview.h"
#include "ThumbnailPreview.h"
#include "UiSmokeTest.h"
#include "GraphSmokeTest.h"
#include "FunctionSmokeTest.h"
#include "InterfaceSmokeTest.h"
#include "ModelSmokeTest.h"
#include "BlueprintSmokeTest.h"
#include "MotionSmokeTest.h"
#include "NamedTypeSmokeTest.h"
#include "GraphInterfaceSmokeTest.h"
#include "MigrationSmokeTest.h"
#include "TypeMigrationSmokeTest.h"
#include "GraphCollapseSmokeTest.h"
#include "GraphExpandSmokeTest.h"
#include <chrono>
using namespace cygame;

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>

#include <d3d11.h>
#include <tchar.h>
#include <windows.h>

#include <shellapi.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

ID3D11Device* gDevice {};
ID3D11DeviceContext* gContext {};
IDXGISwapChain* gSwapChain {};
ID3D11RenderTargetView* gTarget {};
UINT gResizeWidth {}, gResizeHeight {};
bool gCloseRequested=false;

void createTarget()
{
    ID3D11Texture2D* back {};
    gSwapChain->GetBuffer(0, IID_PPV_ARGS(&back));
    gDevice->CreateRenderTargetView(back, nullptr, &gTarget);
    back->Release();
}

void releaseTarget()
{
    if (gTarget != nullptr) {
        gTarget->Release();
        gTarget = nullptr;
    }
}

bool createDevice(HWND hwnd)
{
    DXGI_SWAP_CHAIN_DESC sd {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    const D3D_FEATURE_LEVEL levels[] { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL level {};
    auto result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd,
                                                &gSwapChain, &gDevice, &level, &gContext);
    if (result == DXGI_ERROR_UNSUPPORTED)
        result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &sd,
                                               &gSwapChain, &gDevice, &level, &gContext);
    if (result != S_OK)
        return false;
    createTarget();
    return true;
}

void releaseDevice()
{
    releaseTarget();
    if (gSwapChain != nullptr)
        gSwapChain->Release();
    if (gContext != nullptr)
        gContext->Release();
    if (gDevice != nullptr)
        gDevice->Release();
}

// Writes the back buffer to a 32-bit BMP (the --capture option, for checks).
bool captureBackBuffer(const wchar_t* path)
{
    ID3D11Texture2D* back {};
    if (FAILED(gSwapChain->GetBuffer(0, IID_PPV_ARGS(&back))))
        return false;
    D3D11_TEXTURE2D_DESC desc {};
    back->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;
    ID3D11Texture2D* staging {};
    if (FAILED(gDevice->CreateTexture2D(&desc, nullptr, &staging))) {
        back->Release();
        return false;
    }
    gContext->CopyResource(staging, back);
    back->Release();
    D3D11_MAPPED_SUBRESOURCE mapped {};
    if (FAILED(gContext->Map(staging, 0, D3D11_MAP_READ, 0, &mapped))) {
        staging->Release();
        return false;
    }
    const auto width = static_cast<int>(desc.Width);
    const auto height = static_cast<int>(desc.Height);
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 4);
    for (auto y = 0; y < height; ++y) {
        const auto* row = static_cast<const unsigned char*>(mapped.pData) + static_cast<std::size_t>(y) * mapped.RowPitch;
        for (auto x = 0; x < width; ++x) {
            auto* out = &pixels[(static_cast<std::size_t>(y) * width + x) * 4];
            out[0] = row[x * 4 + 2];
            out[1] = row[x * 4 + 1];
            out[2] = row[x * 4 + 0];
            out[3] = 255;
        }
    }
    gContext->Unmap(staging, 0);
    staging->Release();
    BITMAPFILEHEADER file {};
    BITMAPINFOHEADER info {};
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = -height;
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    file.bfType = 0x4d42;
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + static_cast<DWORD>(pixels.size());
    FILE* out {};
    if (_wfopen_s(&out, path, L"wb") != 0 || out == nullptr)
        return false;
    std::fwrite(&file, sizeof(file), 1, out);
    std::fwrite(&info, sizeof(info), 1, out);
    std::fwrite(pixels.data(), 1, pixels.size(), out);
    std::fclose(out);
    return true;
}

LRESULT WINAPI windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    switch (msg) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            gResizeWidth = LOWORD(lParam);
            gResizeHeight = HIWORD(lParam);
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_CLOSE:
        gCloseRequested=true;
        return 0;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(hwnd, msg, wParam, lParam);
}


} // namespace
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    fs::path failureRoot;bool automated=false;
    try {
        wchar_t executable[32768]{};GetModuleFileNameW(nullptr,executable,32768);
        fs::path root=fs::path(executable).parent_path();
        std::wstring capturePath,captureReadyPath,gizmoTestPath,graphTestPath,functionTestPath,interfaceTestPath,modelTestPath,blueprintTestPath,motionTestPath,typesTestPath,interfacesTestPath,migrationTestPath,collapseTestPath,expandTestPath,typeMigrationTestPath;std::string theme,language,panel,workspace,asset,function,modelTab;double exitAfter=0,modelTime=-1;int captureAfter=40;
        int count=0;auto** args=CommandLineToArgvW(GetCommandLineW(),&count);
        for(int i=1;i<count;++i){
            const std::wstring arg=args[i];
            const auto next=[&](){return i+1<count?std::wstring(args[++i]):std::wstring();};
            if(arg==L"--root")root=next();
            else if(arg==L"--test-expand")expandTestPath=next();
            else if(arg==L"--test-collapse")collapseTestPath=next();
            else if(arg==L"--test-type-migration")typeMigrationTestPath=next();
            else if(arg==L"--test-migration")migrationTestPath=next();
            else if(arg==L"--test-interfaces")interfacesTestPath=next();
            else if(arg==L"--test-types")typesTestPath=next();
            else if(arg==L"--test-motion")motionTestPath=next();
            else if(arg==L"--test-blueprint")blueprintTestPath=next();
            else if(arg==L"--test-model")modelTestPath=next();
            else if(arg==L"--test-interface")interfaceTestPath=next();
            else if(arg==L"--test-functions")functionTestPath=next();
            else if(arg==L"--test-graph")graphTestPath=next();
            else if(arg==L"--test-gizmo")gizmoTestPath=next();
            else if(arg==L"--capture")capturePath=next();
            else if(arg==L"--capture-ready")captureReadyPath=next();
            else if(arg==L"--capture-after")captureAfter=std::clamp(std::stoi(next()),40,1200);
            else if(arg==L"--model-tab")modelTab=fs::path(next()).u8string();
            else if(arg==L"--model-time")modelTime=_wtof(next().c_str());
            else if(arg==L"--asset")asset=fs::path(next()).u8string();
            else if(arg==L"--function")function=fs::path(next()).u8string();
            else if(arg==L"--theme"){auto t=next();theme=std::string(t.begin(),t.end());}
            else if(arg==L"--lang"){auto t=next();language=std::string(t.begin(),t.end());}
            else if(arg==L"--workspace"){auto t=next();workspace=std::string(t.begin(),t.end());panel=workspace=="Scripting"?"logic":workspace=="Materials"?"material":"viewport";}
            else if(arg==L"--panel"){auto t=next();panel=std::string(t.begin(),t.end());}
            else if(arg==L"--exit-after")exitAfter=_wtof(next().c_str());
        }LocalFree(args);
        root=fs::weakly_canonical(root);failureRoot=root;automated=exitAfter>0||!capturePath.empty()||!gizmoTestPath.empty()||!graphTestPath.empty()||!functionTestPath.empty()||!interfaceTestPath.empty()||!modelTestPath.empty()||!expandTestPath.empty()||!collapseTestPath.empty()||!typeMigrationTestPath.empty()||!migrationTestPath.empty()||!interfacesTestPath.empty()||!typesTestPath.empty()||!motionTestPath.empty()||!blueprintTestPath.empty();
        const std::wstring mutexName=L"Local\\CyGameSDK-"+std::to_wstring(std::hash<std::wstring>{}(root.wstring()));
        HANDLE mutex=CreateMutexW(nullptr,FALSE,mutexName.c_str());
        if(!mutex||GetLastError()==ERROR_ALREADY_EXISTS){if(mutex)CloseHandle(mutex);MessageBoxW(nullptr,L"An editor is already open for this workspace.",L"CyGameSDK",MB_OK);return 1;}
        fs::create_directories(root/"data/settings");
        ImGui_ImplWin32_EnableDpiAwareness();
        auto dpi=ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint({0,0},MONITOR_DEFAULTTOPRIMARY));
        WNDCLASSEXW wc{sizeof(wc),CS_CLASSDC,windowProc,0L,0L,GetModuleHandle(nullptr),nullptr,nullptr,nullptr,nullptr,L"CyGameEditor",nullptr};
        RegisterClassExW(&wc);
        HWND hwnd=CreateWindowW(wc.lpszClassName,L"CyGameSDK - Sandbox Editor",WS_OVERLAPPEDWINDOW,80,60,static_cast<int>(1600*dpi),static_cast<int>(1000*dpi),nullptr,nullptr,wc.hInstance,nullptr);
        if(!createDevice(hwnd)){releaseDevice();DestroyWindow(hwnd);UnregisterClassW(wc.lpszClassName,wc.hInstance);CloseHandle(mutex);return 1;}
        ShowWindow(hwnd,SW_SHOWDEFAULT);UpdateWindow(hwnd);
        IMGUI_CHECKVERSION();ImGui::CreateContext();auto& io=ImGui::GetIO();
        io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_DockingEnable|ImGuiConfigFlags_ViewportsEnable;
        io.ConfigWindowsMoveFromTitleBarOnly=true;io.ConfigDpiScaleFonts=true;
        const auto ini=(root/"data/settings/editor.ini").u8string();
        io.IniFilename=capturePath.empty()&&gizmoTestPath.empty()&&graphTestPath.empty()&&functionTestPath.empty()&&interfaceTestPath.empty()&&modelTestPath.empty()&&expandTestPath.empty()&&collapseTestPath.empty()&&typeMigrationTestPath.empty()&&migrationTestPath.empty()&&interfacesTestPath.empty()&&typesTestPath.empty()&&motionTestPath.empty()&&blueprintTestPath.empty()?ini.c_str():nullptr;
        EditorCore core(root);
        EditorApp app(core);if(panel.empty())panel=app.startupPanel();if(!capturePath.empty()||!language.empty()||!theme.empty())app.previewSettings(language,theme);
        if(!gizmoTestPath.empty()||!graphTestPath.empty()||!functionTestPath.empty()||!interfaceTestPath.empty()||!modelTestPath.empty()||!expandTestPath.empty()||!collapseTestPath.empty()||!typeMigrationTestPath.empty()||!migrationTestPath.empty()||!interfacesTestPath.empty()||!typesTestPath.empty()||!motionTestPath.empty()||!blueprintTestPath.empty())app.persistSettings=false;
        InterfaceSmokeTest interfaceTest(core,app,interfaceTestPath);if(interfaceTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){interfaceTest.probe(id,a,b);};
        GraphSmokeTest graphTest(core,graphTestPath);if(graphTest.enabled())app.graphProbe=[&](const std::string& key,const Json& boxes,const Json& selection){graphTest.probe(key,boxes,selection);};
        GizmoSmokeTest gizmoTest(core,gizmoTestPath);if(gizmoTest.enabled())app.viewportSelectionProbe=[&](const std::string& id,ImVec2 point){gizmoTest.projected(id,point);};
        app.configureStyle(dpi);cyui::loadFonts();
        int result=0;
        { // Preview resources and IPC must end before the DX11 device/window.
        MeshPreview meshPreview(gDevice,gContext);app.meshRenderer=[&](const std::string& key,const ModelFrame& frame,const ModelCamera& camera,ImVec2 size){return meshPreview.render(key,frame,camera,size);};
        MaterialPreview materialPreview(gDevice,gContext);
        app.materialRenderer=[&materialPreview](const std::string& key,const Json& material,ImVec2 size){return materialPreview.render(key,material,size);};
        app.materialShaderInstaller=[&](const std::string& key,const Json& report){materialPreview.install(key,report);};
        TexturePreview texturePreview(gDevice,core.root());app.textureRenderer=[&](const std::string& path,int channel,bool refresh){return texturePreview.render(path,channel,refresh);};
        ThumbnailPreview thumbnails(gDevice,core.root());app.thumbnailRenderer=[&](const std::string& path){return thumbnails.render(path);};app.clearThumbnails=[&]{thumbnails.clear();};
        EditorIpc ipc(core);if(!workspace.empty())core.execute("workspace_layout",{{"preset",workspace}});
        if(!asset.empty()){
            auto info=core.execute("asset_open",{{"path",asset}});
            if(panel=="texture")panel=core.execute("texture_open",{{"path",asset}}).at("panelId");
            else if(panel=="compiler"){if(info.contains("graphId"))core.execute("material_compiler_view",{{"graphId",info.at("graphId")}});}
            else if(panel=="changes")core.execute("graph_diff_view",{{"graphId",info.at("graphId")}});
            else if(panel=="references")core.execute("asset_reference_view",{{"path",asset}});
            else if(info.contains("jsonId")){auto opened=app.openJson(asset);panel=opened.at("panelId");}
            else if(info.contains("modelId")){auto opened=app.openModel(asset);panel=opened.at("panelId");if(modelTime>=0||!modelTab.empty()){Json settings={{"modelId",info.at("modelId")}};if(modelTime>=0){settings["animation"]=0;settings["time"]=modelTime;}if(!modelTab.empty())settings["tab"]=modelTab;core.execute("model_view_settings",settings);}}
            else if(info.contains("graphId")){auto opened=function.empty()?app.open(info.at("graphId")):app.openFunction(info.at("graphId"),function);panel=opened.at("panelId");}
        }app.open(panel);
        ModelSmokeTest modelTest(core,app,modelTestPath);if(modelTest.enabled())app.interfaceProbe=[&](const std::string& key,ImVec2 a,ImVec2 b){modelTest.probe(key,a,b);};
        BlueprintSmokeTest blueprintTest(core,app,blueprintTestPath);if(blueprintTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){blueprintTest.probe(id,a,b);};
        MotionSmokeTest motionTest(core,motionTestPath);if(motionTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){motionTest.probe(id,a,b);};
        NamedTypeSmokeTest typesTest(core,typesTestPath);if(typesTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){typesTest.probe(id,a,b);};
        GraphInterfaceSmokeTest interfacesTest(core,interfacesTestPath);if(interfacesTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){interfacesTest.probe(id,a,b);};
        MigrationSmokeTest migrationTest(core,app,migrationTestPath);if(migrationTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){migrationTest.probe(id,a,b);};
        TypeMigrationSmokeTest typeMigrationTest(core,app,typeMigrationTestPath);if(typeMigrationTest.enabled())app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){typeMigrationTest.probe(id,a,b);};
        GraphExpandSmokeTest expandTest(core,app,expandTestPath);if(expandTest.enabled()){app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){expandTest.probe(id,a,b);};app.graphProbe=[&](const std::string& key,const Json& boxes,const Json& selection){expandTest.graphProbe(key,boxes,selection);};}
        GraphCollapseSmokeTest collapseTest(core,app,collapseTestPath);if(collapseTest.enabled()){app.interfaceProbe=[&](const std::string& id,ImVec2 a,ImVec2 b){collapseTest.probe(id,a,b);};app.graphProbe=[&](const std::string& key,const Json& boxes,const Json& selection){collapseTest.graphProbe(key,boxes,selection);};}
        FunctionSmokeTest functionTest(core,app,functionTestPath,asset.empty()?"logic":"asset:"+asset);
        ImGui_ImplWin32_Init(hwnd);ImGui_ImplDX11_Init(gDevice,gContext);
        const auto started=std::chrono::steady_clock::now();bool done=false;int frames=0;
        while(!done){
            const auto frameStarted=std::chrono::steady_clock::now();
            MSG msg{};while(PeekMessage(&msg,nullptr,0U,0U,PM_REMOVE)){TranslateMessage(&msg);DispatchMessage(&msg);if(msg.message==WM_QUIT)done=true;}
            if(done)break;
            if(gCloseRequested){app.requestClose();gCloseRequested=false;}
            if(gResizeWidth&&gResizeHeight){
                releaseTarget();
                if(FAILED(gSwapChain->ResizeBuffers(0,gResizeWidth,gResizeHeight,DXGI_FORMAT_UNKNOWN,0)))throw std::runtime_error("DX11 resize failed");
                gResizeWidth=gResizeHeight=0;createTarget();
            }
            ImGui_ImplDX11_NewFrame();ImGui_ImplWin32_NewFrame();gizmoTest.input(io,frames+1);graphTest.input(io,frames+1);functionTest.input(io,frames+1);interfaceTest.input(io,frames+1);modelTest.input(io,frames+1);blueprintTest.input(io,frames+1);motionTest.input(io,frames+1);typesTest.input(io,frames+1);interfacesTest.input(io,frames+1);migrationTest.input(io,frames+1);typeMigrationTest.input(io,frames+1);collapseTest.input(io,frames+1);expandTest.input(io,frames+1);ImGui::NewFrame();ImGuizmo::BeginFrame();
            ipc.poll();meshPreview.prune(core.models());++frames;if(frames<=3)app.open(panel);app.frame();
            const auto expandResult=expandTest.finish(frames);if(expandResult>=0){result=expandResult;done=true;}
            const auto collapseResult=collapseTest.finish(frames);if(collapseResult>=0){result=collapseResult;done=true;}
            const auto typeMigrationResult=typeMigrationTest.finish(frames);if(typeMigrationResult>=0){result=typeMigrationResult;done=true;}
            const auto migrationResult=migrationTest.finish(frames);if(migrationResult>=0){result=migrationResult;done=true;}
            const auto interfacesResult=interfacesTest.finish(frames);if(interfacesResult>=0){result=interfacesResult;done=true;}
            const auto typesResult=typesTest.finish(frames);if(typesResult>=0){result=typesResult;done=true;}
            const auto motionResult=motionTest.finish();if(motionResult>=0){result=motionResult;done=true;}
            const auto blueprintResult=blueprintTest.finish(frames);if(blueprintResult>=0){result=blueprintResult;done=true;}
            const auto modelResult=modelTest.finish(frames);if(modelResult>=0){result=modelResult;done=true;}
            const auto interfaceResult=interfaceTest.finish(frames);if(interfaceResult>=0){result=interfaceResult;done=true;}
            const auto functionResult=functionTest.finish(frames);if(functionResult>=0){result=functionResult;done=true;}
            const auto graphResult=graphTest.finish(frames);if(graphResult>=0){result=graphResult;done=true;}
            const auto testResult=gizmoTest.finish(frames);if(testResult>=0){result=testResult;done=true;}ImGui::Render();
            const auto ground=cyui::vec(cyui::palette().bg0);const float clear[4]{ground.x,ground.y,ground.z,1};
            gContext->OMSetRenderTargets(1,&gTarget,nullptr);gContext->ClearRenderTargetView(gTarget,clear);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            ImGui::UpdatePlatformWindows();ImGui::RenderPlatformWindowsDefault();
            if(!capturePath.empty()&&frames>=(functionTest.enabled()?90:captureAfter)&&(captureReadyPath.empty()||fs::is_regular_file(captureReadyPath))){if(!captureBackBuffer(capturePath.c_str()))result=2;done=true;}
            if(exitAfter>0&&std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()>exitAfter)done=true;
            done|=app.shouldClose;gSwapChain->Present(1,0);
            // Remote/virtual swap chains may ignore vsync. Keep editor idle cost bounded.
            const auto frameMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-frameStarted).count();
            if(frameMs<16.0)Sleep(static_cast<DWORD>(16.0-frameMs));
        }
        if(app.persistSettings)ImGui::SaveIniSettingsToDisk(ini.c_str());
        ImGui_ImplDX11_Shutdown();ImGui_ImplWin32_Shutdown();ImGui::DestroyContext();
        app.meshRenderer={};app.materialRenderer={};app.materialShaderInstaller={};app.textureRenderer={};app.thumbnailRenderer={};app.clearThumbnails={};
        gContext->ClearState();gContext->Flush();
        }
        releaseDevice();DestroyWindow(hwnd);UnregisterClassW(wc.lpszClassName,wc.hInstance);CloseHandle(mutex);
        return result;
    }catch(const std::exception& e){try{if(!failureRoot.empty())writeJson(failureRoot/"data/logs/native-fatal.json",{{"error",e.what()}});}catch(...){}if(!automated)MessageBoxA(nullptr,e.what(),"CyGameSDK error",MB_OK|MB_ICONERROR);return 1;}
}
