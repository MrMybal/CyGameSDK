@echo off
setlocal
set APP=%~dp0
set ROOT=%APP%..
if not defined VCINSTALLDIR (
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
)
if "%CYUI_ROOT%"=="" set CYUI_ROOT=%ROOT%\..\CyImgui_Interface
set IMGUI=%CYUI_ROOT%\third_party\imgui
if not exist "%ROOT%\runtime\build\native" mkdir "%ROOT%\runtime\build\native"
pushd "%ROOT%\runtime\build\native"
cl /nologo /std:c++17 /O2 /EHsc /W4 /utf-8 /MP /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN ^
 /I"%APP%native" /I"%APP%third_party\nlohmann" /I"%APP%third_party\imguizmo" /I"%CYUI_ROOT%\include" /I"%IMGUI%" /I"%IMGUI%\backends" ^
 "%APP%native\Core.cpp" "%APP%native\Scene3D.cpp" "%APP%native\ModelCommands.cpp" "%APP%native\ModelAuthoring.cpp" "%APP%native\JsonDocuments.cpp" "%APP%native\Build.cpp" "%APP%native\Assets.cpp" "%APP%native\Imports.cpp" "%APP%native\AssetReferences.cpp" "%APP%native\Scene.cpp" "%APP%native\Graph.cpp" "%APP%native\Blueprint.cpp" "%APP%native\Interfaces.cpp" "%APP%native\SignatureMigration.cpp" "%APP%native\TypeLibrary.cpp" "%APP%native\TypeMigration.cpp" "%APP%native\GraphCollapse.cpp" "%APP%native\GraphExpand.cpp" "%APP%native\Macros.cpp" "%APP%native\GraphRuntime.cpp" "%APP%native\Functions.cpp" "%APP%native\Materials.cpp" "%APP%native\MaterialProgram.cpp" "%APP%native\Image.cpp" "%APP%native\GraphBatch.cpp" "%APP%native\GraphNavigation.cpp" "%APP%native\GraphDiff.cpp" "%APP%native\Editor.cpp" "%APP%native\MeshPreview.cpp" "%APP%native\ModelView.cpp" "%APP%native\ModelAuthoringView.cpp" "%APP%native\MotionToolsView.cpp" "%APP%native\JsonView.cpp" "%APP%native\InterfaceOverview.cpp" "%APP%native\BuildClient.cpp" "%APP%native\BuildView.cpp" "%APP%native\Content.cpp" "%APP%native\Outliner.cpp" "%APP%native\Workspace.cpp" "%APP%third_party\imguizmo\ImGuizmo.cpp" "%APP%native\Viewport.cpp" "%APP%native\Gizmo.cpp" "%APP%native\GraphView.cpp" "%APP%native\GraphPanels.cpp" "%APP%native\GraphNavigatorView.cpp" "%APP%native\GraphDiffView.cpp" "%APP%native\FunctionsView.cpp" "%APP%native\BlueprintView.cpp" "%APP%native\TypesView.cpp" "%APP%native\InterfacesView.cpp" "%APP%native\SignatureMigrationView.cpp" "%APP%native\GraphCollapseView.cpp" "%APP%native\GraphExpandView.cpp" "%APP%native\GameCodecView.cpp" "%APP%native\MaterialInstanceView.cpp" "%APP%native\MaterialTools.cpp" "%APP%native\TextureView.cpp" "%APP%native\ImportView.cpp" "%APP%native\ThumbnailPreview.cpp" "%APP%native\TexturePreview.cpp" "%APP%native\ReferenceView.cpp" "%APP%native\Ipc.cpp" "%APP%native\main.cpp" "%APP%native\MaterialPreview.cpp" ^
 "%CYUI_ROOT%\src\Theme.cpp" "%CYUI_ROOT%\src\Widgets.cpp" "%CYUI_ROOT%\src\Layout.cpp" "%CYUI_ROOT%\src\Gallery.cpp" ^
 "%IMGUI%\imgui.cpp" "%IMGUI%\imgui_draw.cpp" "%IMGUI%\imgui_tables.cpp" "%IMGUI%\imgui_widgets.cpp" "%IMGUI%\imgui_demo.cpp" ^
 "%IMGUI%\backends\imgui_impl_win32.cpp" "%IMGUI%\backends\imgui_impl_dx11.cpp" ^
 /Fe:"%ROOT%\CyGameEditor.exe" /link /SUBSYSTEM:WINDOWS d3d11.lib dxgi.lib d3dcompiler.lib dwmapi.lib user32.lib gdi32.lib shell32.lib ole32.lib advapi32.lib bcrypt.lib windowscodecs.lib comdlg32.lib
if errorlevel 1 (
  popd
  exit /b 1
)
cl /nologo /std:c++17 /O2 /EHsc /W4 /utf-8 /DNOMINMAX /I"%APP%native" /I"%APP%third_party\nlohmann" "%APP%tests\core_tests.cpp" Core.obj JsonDocuments.obj Scene3D.obj ModelCommands.obj ModelAuthoring.obj Build.obj Assets.obj Imports.obj AssetReferences.obj Scene.obj Graph.obj Blueprint.obj Interfaces.obj SignatureMigration.obj TypeLibrary.obj TypeMigration.obj GraphCollapse.obj GraphExpand.obj Macros.obj GraphRuntime.obj Functions.obj Materials.obj MaterialProgram.obj Image.obj GraphBatch.obj GraphNavigation.obj GraphDiff.obj Ipc.obj /Fe:"%ROOT%\runtime\build\CyGameSDKTests.exe" /link ole32.lib advapi32.lib bcrypt.lib windowscodecs.lib comdlg32.lib
set ERR=%ERRORLEVEL%
popd
exit /b %ERR%
