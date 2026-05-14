#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <map>

class DataReader {
public:
    DataReader(const std::string& config_path, const std::string& locations_path, const std::string& requests_path);
    bool loadAll();
    
    double getBaseRate() const;
    double getExpressMultiplier() const;
    double getFragileMultiplier() const;
    double getRegionMultiplier(const std::string& region) const;
    
    const std::map<std::string, Location>& getLocations() const;
    const std::vector<ShippingRequest>& getRequests() const;

private:
    std::string config_path;
    std::string locations_path;
    std::string requests_path;
    
    double base_rate;
    double express_multiplier;
    double fragile_multiplier;
    std::map<std::string, double> region_multipliers;
    
    std::map<std::string, Location> locations;
    std::vector<ShippingRequest> requests;
    
    bool loadConfig();
    bool loadLocations();
    bool loadRequests();
};
