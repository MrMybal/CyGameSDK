@echo off
setlocal
if not exist "%~dp0CyGameEditor.exe" (
  echo CyGameEditor.exe was not found in "%~dp0".
  pause
  exit /b 1
)
start "" /D "%~dp0" "%~dp0CyGameEditor.exe"
if errorlevel 1 (
  echo The editor could not start. See app\Docs\LaunchRecovery.md.
  pause
  exit /b 1
)
