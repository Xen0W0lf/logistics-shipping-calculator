#include <iostream>
#include <fstream>
#include "DataReader.h"
#include "Calculator.h"
#include "ReportGenerator.h"

// Define this macro to ensure sockets are used
#define WIN32_LEAN_AND_MEAN
#include "httplib.h"
#include "../include/json.hpp" // nlohmann/json

using json = nlohmann::json;

int main() {
    std::cout << "Loading logistics data...\n";
    DataReader reader("data/config.json", "data/locations.csv", "data/requests.json");
    
    if (!reader.loadAll()) {
        std::cerr << "Failed to load one or more data files.\n";
        return 1;
    }
    
    // Generate the static index.html with populated dropdowns
    ReportGenerator::generateHTMLMap("index.html", reader);
    std::cout << "Generated index.html template.\n";
    
    Calculator calculator(reader);
    httplib::Server svr;
    
    // Serve the index.html on root
    svr.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        std::ifstream file("index.html");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        res.set_content(content, "text/html");
    });
    
    // API endpoint for calculations
    svr.Post("/api/calculate", [&calculator, &reader](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            ShippingRequest sreq;
            sreq.id = j.value("id", "DYN-0");
            sreq.origin_city = j.value("origin_city", "");
            sreq.destination_city = j.value("destination_city", "");
            sreq.weight_kg = j.value("weight_kg", 0.0);
            sreq.is_express = j.value("is_express", false);
            sreq.is_fragile = j.value("is_fragile", false);
            
            CalculationResult calc_res = calculator.calculateSingle(sreq);
            
            json response_json;
            if (calc_res.cost < 0) {
                response_json["error"] = "Invalid origin or destination.";
            } else {
                response_json["cost"] = calc_res.cost;
                response_json["distance_km"] = calc_res.distance_km;
                response_json["origin_city"] = calc_res.origin_city;
                response_json["destination_city"] = calc_res.destination_city;
                
                const auto& locs = reader.getLocations();
                response_json["origin_coord"] = {locs.at(calc_res.origin_city).lat, locs.at(calc_res.origin_city).lon};
                response_json["dest_coord"] = {locs.at(calc_res.destination_city).lat, locs.at(calc_res.destination_city).lon};
            }
            
            res.set_content(response_json.dump(), "application/json");
        } catch (std::exception& e) {
            res.set_content(R"({"error": "Invalid request"})", "application/json");
        }
    });
    
    std::cout << "Starting Local Web Server...\n";
    std::cout << "Please open http://localhost:8080 in your browser to interact with the Calculator.\n";
    std::cout << "Press Ctrl+C to stop the server.\n";
    
    svr.listen("localhost", 8080);
    
    return 0;
}
