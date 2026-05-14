#include "Calculator.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Calculator::Calculator(const DataReader& data) : data(data) {}

double Calculator::calculateDistance(double lat1, double lon1, double lat2, double lon2) const {
    const double R = 6371.0; // Earth radius in km
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = std::sin(dLat/2) * std::sin(dLat/2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dLon/2) * std::sin(dLon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return R * c;
}

std::vector<CalculationResult> Calculator::calculateAll() const {
    std::vector<CalculationResult> results;
    for (const auto& req : data.getRequests()) {
        results.push_back(calculateSingle(req));
    }
    return results;
}

CalculationResult Calculator::calculateSingle(const ShippingRequest& req) const {
    CalculationResult res;
    res.request_id = req.id;
    res.origin_city = req.origin_city;
    res.destination_city = req.destination_city;
    
    const auto& locs = data.getLocations();
    
    if (locs.find(req.origin_city) == locs.end() || locs.find(req.destination_city) == locs.end()) {
        res.cost = -1.0; // Error
        res.distance_km = 0.0;
        return res;
    }
    
    auto origin = locs.at(req.origin_city);
    auto dest = locs.at(req.destination_city);
    
    res.distance_km = calculateDistance(origin.lat, origin.lon, dest.lat, dest.lon);
    
    double cost = data.getBaseRate() * req.weight_kg * (res.distance_km / 100.0);
    cost *= data.getRegionMultiplier(origin.region);
    cost *= data.getRegionMultiplier(dest.region);
    
    if (req.is_express) cost *= data.getExpressMultiplier();
    if (req.is_fragile) cost *= data.getFragileMultiplier();
    
    res.cost = cost;
    return res;
}
