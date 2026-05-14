#define WIN32_LEAN_AND_MEAN
#include "../include/httplib.h"
#include "../include/json.hpp"
#include <iostream>

using json = nlohmann::json;

int main() {
    std::cout << "[CalculationService] Starting on port 8082...\n";
    httplib::Server svr;

    svr.Post("/compute", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            
            double total_dist = 0.0;
            double total_cost = 0.0;
            double base_rate = j["base_rate"].get<double>();
            double weight_kg = j["weight_kg"].get<double>();
            
            json mode_breakdown;
            mode_breakdown["car"] = 0.0;
            mode_breakdown["boat"] = 0.0;
            mode_breakdown["airplane"] = 0.0;
            
            if (j.contains("legs") && j["legs"].is_array()) {
                for (auto& leg : j["legs"]) {
                    std::string mode = leg.value("mode", "car");
                    double dist = leg.value("distance_km", 0.0);
                    
                    double multiplier = 1.0;
                    if (mode == "airplane") multiplier = 5.0;
                    else if (mode == "boat") multiplier = 0.5;
                    else if (mode == "car") multiplier = 1.0;
                    
                    double leg_cost = base_rate * weight_kg * (dist / 100.0) * multiplier;
                    
                    total_dist += dist;
                    total_cost += leg_cost;
                    if (mode_breakdown.contains(mode)) {
                        mode_breakdown[mode] = mode_breakdown[mode].get<double>() + dist;
                    }
                }
            } else {
                // Fallback if legs not provided, not strictly needed but safe
                total_dist = 0;
            }
            
            // Apply other multipliers
            if (j.contains("origin_region_multiplier")) total_cost *= j["origin_region_multiplier"].get<double>();
            if (j.contains("dest_region_multiplier")) total_cost *= j["dest_region_multiplier"].get<double>();
            
            if (j.value("is_express", false)) total_cost *= j["express_multiplier"].get<double>();
            if (j.value("is_fragile", false)) total_cost *= j["fragile_multiplier"].get<double>();
            
            json out;
            out["distance_km"] = total_dist;
            out["cost"] = total_cost;
            out["mode_breakdown"] = mode_breakdown;
            res.set_content(out.dump(), "application/json");
        } catch (const std::exception& e) {
            std::cerr << "Calc Error: " << e.what() << "\n";
            res.status = 400;
            res.set_content(R"({"error": "Bad Math Request"})", "application/json");
        }
    });

    std::cout << "[CalculationService] Listening at http://0.0.0.0:8082\n";
    svr.listen("0.0.0.0", 8082);
    return 0;
}
