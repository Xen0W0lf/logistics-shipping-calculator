#include "DataReader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "../include/json.hpp"

using json = nlohmann::json;

DataReader::DataReader(const std::string& config_path, const std::string& locations_path, const std::string& requests_path)
    : config_path(config_path), locations_path(locations_path), requests_path(requests_path) {}

bool DataReader::loadAll() {
    return loadConfig() && loadLocations() && loadRequests();
}

bool DataReader::loadConfig() {
    std::ifstream file(config_path);
    if (!file.is_open()) return false;
    
    json j;
    file >> j;
    
    base_rate = j["base_rate_per_kg"];
    express_multiplier = j["express_multiplier"];
    fragile_multiplier = j["fragile_multiplier"];
    
    for (auto& [key, value] : j["regions"].items()) {
        region_multipliers[key] = value;
    }
    return true;
}

bool DataReader::loadLocations() {
    std::ifstream file(locations_path);
    if (!file.is_open()) return false;
    
    std::string line;
    std::getline(file, line); // header
    
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string city, state, region, lat_str, lon_str;
        
        std::getline(ss, city, ',');
        std::getline(ss, state, ',');
        std::getline(ss, region, ',');
        std::getline(ss, lat_str, ',');
        std::getline(ss, lon_str, ',');
        
        Location loc;
        loc.city = city;
        loc.state = state;
        loc.region = region;
        loc.lat = std::stod(lat_str);
        loc.lon = std::stod(lon_str);
        
        locations[city] = loc;
    }
    return true;
}

bool DataReader::loadRequests() {
    std::ifstream file(requests_path);
    if (!file.is_open()) return false;
    
    json j;
    file >> j;
    
    for (const auto& item : j) {
        ShippingRequest req;
        req.id = item["id"];
        req.origin_city = item["origin_city"];
        req.destination_city = item["destination_city"];
        req.weight_kg = item["weight_kg"];
        req.is_express = item["is_express"];
        req.is_fragile = item["is_fragile"];
        requests.push_back(req);
    }
    return true;
}

double DataReader::getBaseRate() const { return base_rate; }
double DataReader::getExpressMultiplier() const { return express_multiplier; }
double DataReader::getFragileMultiplier() const { return fragile_multiplier; }
double DataReader::getRegionMultiplier(const std::string& region) const {
    auto it = region_multipliers.find(region);
    return it != region_multipliers.end() ? it->second : 1.0;
}
const std::map<std::string, Location>& DataReader::getLocations() const { return locations; }
const std::vector<ShippingRequest>& DataReader::getRequests() const { return requests; }
