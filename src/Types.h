#pragma once
#include <string>

struct Location {
    std::string city;
    std::string state;
    std::string region;
    double lat;
    double lon;
};

struct ShippingRequest {
    std::string id;
    std::string origin_city;
    std::string destination_city;
    double weight_kg;
    bool is_express;
    bool is_fragile;
};

struct CalculationResult {
    std::string request_id;
    std::string origin_city;
    std::string destination_city;
    double distance_km;
    double cost;
};
