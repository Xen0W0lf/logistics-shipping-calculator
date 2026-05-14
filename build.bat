@echo off
echo Building Microservices...

echo Compiling DataService (8081)...
g++ src/services/DataService.cpp src/DataReader.cpp -o DataService.exe -Iinclude -std=c++17 -lws2_32
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling CalculationService (8082)...
g++ src/services/CalculationService.cpp -o CalculationService.exe -Iinclude -std=c++17 -lws2_32
if %errorlevel% neq 0 exit /b %errorlevel%

echo Compiling FrontendGateway (8080)...
g++ src/services/FrontendGateway.cpp -o FrontendGateway.exe -Iinclude -std=c++17 -lws2_32
if %errorlevel% neq 0 exit /b %errorlevel%

echo Build successful.
echo Launching Microservices...
start "DataService (8081)" DataService.exe
timeout /t 2 /nobreak >nul
start "CalculationService (8082)" CalculationService.exe
start "FrontendGateway (8080)" FrontendGateway.exe

echo All services launched!
