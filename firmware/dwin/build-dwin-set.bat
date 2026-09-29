@echo off
setlocal
if not exist ".\obj\ParcelTerminal_DWIN.hex" (
  echo Missing .\obj\ParcelTerminal_DWIN.hex
  exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File ".\tools\hex_to_bin.ps1" -InputPath ".\obj\ParcelTerminal_DWIN.hex" -OutputPath ".\DWIN_SET\T5L51.bin"
exit /b %ERRORLEVEL%
