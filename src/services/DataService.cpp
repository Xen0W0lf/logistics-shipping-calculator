#define WIN32_LEAN_AND_MEAN
#include "../include/httplib.h"
#include "../include/json.hpp"
#include "../src/DataReader.h"
#include <iostream>
#include <fstream>

int main() {
    std::cout << "[DataService] Starting on port 8081...\n";
    DataReader reader("data/config.json", "data/locations.csv", "data/requests.json");
    if (!reader.loadAll()) {
        std::cerr << "[DataService] Could not load local data files!\n";
        return 1;
    }

    httplib::Server svr;

    svr.Get("/locations", [&reader](const httplib::Request&, httplib::Response& res) {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [city, loc] : reader.getLocations()) {
            j[city] = {
                {"state", loc.state},
                {"region", loc.region},
                {"lat", loc.lat},
                {"lon", loc.lon}
            };
        }
        res.set_content(j.dump(), "application/json");
    });

    svr.Get("/rates", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream file("data/config.json");
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        res.set_content(content, "application/json");
    });

    std::cout << "[DataService] Listening at http://0.0.0.0:8081\n";
    svr.listen("0.0.0.0", 8081);
    return 0;
}
