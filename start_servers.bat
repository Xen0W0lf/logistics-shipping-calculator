@echo off
echo Starting Logistics Microservices...
start "DataService (8081)" DataService.exe
timeout /t 2 /nobreak >nul
start "CalculationService (8082)" CalculationService.exe
start "FrontendGateway (8080)" FrontendGateway.exe
echo Services are running in separate windows!
