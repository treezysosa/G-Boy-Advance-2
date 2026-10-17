@echo off
setlocal
set "GBA_REPORT_PS=%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe"
if exist "%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe" set "GBA_REPORT_PS=%SystemRoot%\Sysnative\WindowsPowerShell\v1.0\powershell.exe"
if /I "%~1"=="--no-open" (
    "%GBA_REPORT_PS%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-report.ps1" -OutputPath "%~2"
    if errorlevel 1 exit /b 1
    exit /b 0
)
"%GBA_REPORT_PS%" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Collect-report.ps1" -OpenReport
if errorlevel 1 (
    echo Could not create the report. Please send a screenshot of this window.
    pause
    exit /b 1
)
exit /b 0
