@echo off
echo Stopping Microservices...
taskkill /IM FrontendGateway.exe /F >nul 2>&1
taskkill /IM CalculationService.exe /F >nul 2>&1
taskkill /IM DataService.exe /F >nul 2>&1
echo All services stopped!
pause
