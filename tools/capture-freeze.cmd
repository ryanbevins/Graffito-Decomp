@echo off
cd /d "%~dp0.."
python tools\capture-freeze.py %*
pause
