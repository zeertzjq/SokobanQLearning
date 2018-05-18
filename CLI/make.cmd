@echo off
cd /D "%~dp0"
where /Q cl.exe >nul 2>&1 && (
    cl.exe /EHsc /Ox CLI.cpp
    exit /b
)
make.exe %*
