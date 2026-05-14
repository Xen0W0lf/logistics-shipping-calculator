#define WIN32_LEAN_AND_MEAN
#include "../include/httplib.h"
#include "../include/json.hpp"
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using json = nlohmann::json;

double calcDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

int main() {
    std::cout << "[CalculationService] Starting on port 8082...\n";
    httplib::Server svr;

    svr.Post("/compute", [](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            double dist = calcDistance(j["origin_lat"], j["origin_lon"], j["dest_lat"], j["dest_lon"]);
            
            double cost = (double)j["base_rate"] * (double)j["weight_kg"] * (dist / 100.0);
            cost *= (double)j["origin_region_multiplier"];
            cost *= (double)j["dest_region_multiplier"];
            
            if (j.value("is_express", false)) cost *= (double)j["express_multiplier"];
            if (j.value("is_fragile", false)) cost *= (double)j["fragile_multiplier"];
            
            json out;
            out["distance_km"] = dist;
            out["cost"] = cost;
            res.set_content(out.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error": "Bad Math Request"})", "application/json");
        }
    });

    std::cout << "[CalculationService] Listening at http://0.0.0.0:8082\n";
    svr.listen("0.0.0.0", 8082);
    return 0;
}
