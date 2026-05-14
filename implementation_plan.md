# Microservices Architecture Implementation Plan

## Goal Description
The goal is to refactor the current monolithic C++ application into a **Microservices Architecture**. Instead of having one program do everything, we will split the responsibilities across multiple independent C++ services that communicate with each other over HTTP (REST APIs) using JSON. This simulates a modern enterprise cloud environment while keeping the implementation strictly in C++.

## Proposed Architecture

We will split the application into three distinct microservices, each running as its own executable on a different port:

1. **`DataService` (Port 8081)**
   - **Responsibility**: Manages all file I/O and acts as the "database". It reads `config.json` and `locations.csv`.
   - **API Endpoints**: 
     - `GET /locations` (Returns all available cities and coordinates)
     - `GET /rates` (Returns the base rate and multipliers for express/fragile/regions)

2. **`CalculationService` (Port 8082)**
   - **Responsibility**: Pure, stateless mathematical engine.
   - **API Endpoints**:
     - `POST /compute` (Accepts origin coordinates, destination coordinates, weight, and multipliers. Calculates the Haversine distance and the final tariff cost. Returns the math result).

3. **`FrontendGateway` (Port 8080)**
   - **Responsibility**: The user-facing API Gateway and UI server.
   - **Flow**: 
     - Serves `index.html` to the browser.
     - When the user presses "Calculate", the Gateway intercepts the request.
     - It fetches city data from the `DataService` (8081).
     - It proxies the math to the `CalculationService` (8082).
     - It combines the responses and returns the final JSON to the browser animation.

## User Review Required
> [!IMPORTANT]
> **Execution Strategy**: Because this requires running three separate C++ programs at the same time, the `build.bat` script will be updated to compile three separate executables (`DataService.exe`, `CalculationService.exe`, and `Gateway.exe`). It will also include a startup script to launch all three server windows simultaneously.
> 
> Does this 3-tier microservice split align with what you had in mind for the architecture?

## Proposed Changes
- **[NEW]** `src/services/DataService.cpp`
- **[NEW]** `src/services/CalculationService.cpp`
- **[NEW]** `src/services/FrontendGateway.cpp`
- **[DELETE]** `src/main.cpp` (Replaced by the three service entry points)
- **[MODIFY]** `build.bat` (Updated to compile all three services)
- **[MODIFY]** `src/ReportGenerator.cpp` (Will be simplified, as the Gateway will handle serving the HTML).

## Verification Plan
1. Compile all three executables.
2. Launch them on ports 8080, 8081, and 8082.
3. Test the browser at `http://localhost:8080` to verify that the HTTP requests successfully cascade through the Gateway, to the Data Service, to the Calculation Service, and back to the browser without failure.
