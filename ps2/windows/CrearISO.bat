@echo off
rem Abe's Exoddus / Abe's Oddysee PS2: builds the PS2 ISO from your Steam copy of the game (the one whose ELF is next to this file). No extra software needed.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0CrearISO.ps1" %*
echo.
pause
