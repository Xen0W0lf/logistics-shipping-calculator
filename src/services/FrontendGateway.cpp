#define WIN32_LEAN_AND_MEAN
#include "../include/httplib.h"
#include "../include/json.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

using json = nlohmann::json;

json fetchServiceJson(const std::string& host, int port, const std::string& path) {
    httplib::Client cli(host, port);
    cli.set_connection_timeout(0, 500000); // 500ms
    cli.set_read_timeout(1, 0); // 1s
    if (auto res = cli.Get(path)) {
        if (res->status == 200) {
            return json::parse(res->body);
        }
    }
    return json::object();
}

json postServiceJson(const std::string& host, int port, const std::string& path, const json& body) {
    httplib::Client cli(host, port);
    if (auto res = cli.Post(path, body.dump(), "application/json")) {
        if (res->status == 200) {
            return json::parse(res->body);
        }
    }
    return json::object();
}

std::string generateHTML(const json& locations) {
    std::string html = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Logistics Gateway (Microservices)</title>
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css"/>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
    <style>
        :root { --bg: #0f172a; --panel: rgba(30, 41, 59, 0.85); --text: #f8fafc; --accent: #3b82f6; --accent-glow: rgba(59, 130, 246, 0.5); }
        body { margin: 0; padding: 0; font-family: 'Inter', sans-serif; background: var(--bg); color: var(--text); overflow: hidden; }
        #map { width: 100vw; height: 100vh; z-index: 1; }
        .overlay { position: absolute; top: 20px; left: 20px; z-index: 1000; width: 350px; background: var(--panel); backdrop-filter: blur(12px); border-radius: 16px; border: 1px solid rgba(255,255,255,0.1); padding: 24px; box-shadow: 0 25px 50px -12px rgba(0,0,0,0.5); max-height: 85vh; overflow-y: auto; }
        h1 { margin: 0 0 10px 0; font-size: 22px; font-weight: 800; }
        .badge { display: inline-block; background: #10b981; color: #0f172a; font-size: 11px; font-weight: 800; padding: 3px 8px; border-radius: 12px; margin-bottom: 20px; }
        label { font-size: 14px; font-weight: 600; color: #cbd5e1; margin-bottom: 4px; display: block; }
        select, input[type="number"] { width: 100%; padding: 10px; border-radius: 8px; border: 1px solid rgba(255,255,255,0.2); background: rgba(0,0,0,0.3); color: white; margin-bottom: 12px; font-family: 'Inter', sans-serif; box-sizing: border-box; }
        select option { background: var(--bg); color: white; }
        button { width: 100%; padding: 12px; border-radius: 8px; border: none; background: var(--accent); color: white; font-weight: 800; font-size: 16px; cursor: pointer; transition: all 0.2s ease; margin-top: 10px; }
        button:hover { background: #2563eb; transform: translateY(-1px); box-shadow: 0 4px 12px var(--accent-glow); }
        .checkbox-group { display: flex; gap: 15px; margin-bottom: 15px; }
        .checkbox-group label { display: flex; align-items: center; gap: 6px; cursor: pointer; margin: 0; font-weight: 400; }
        .shipment { background: rgba(0,0,0,0.4); border-radius: 12px; padding: 16px; margin-top: 20px; border: 1px solid var(--accent); box-shadow: 0 0 20px var(--accent-glow); display: none; }
        .cost { font-size: 24px; font-weight: 800; color: #10b981; margin-bottom: 8px; }
        .route { font-size: 16px; font-weight: 600; margin-bottom: 4px; }
        .leaflet-container { background: #0f172a !important; }
        .package-icon { background: none; border: none; }
        .package-dot { width: 16px; height: 16px; background: #3b82f6; border-radius: 50%; box-shadow: 0 0 15px #3b82f6, 0 0 30px #3b82f6; border: 2px solid white; }
    </style>
</head>
<body>
    <div id="map"></div>
    <div class="overlay">
        <h1>📦 Create Shipment</h1>
        <div class="badge">MICROSERVICES EDITION</div>
        
        <label>Origin City</label>
        <select id="origin">
)HTML";

    for (auto it = locations.begin(); it != locations.end(); ++it) {
        html += "            <option value=\"" + it.key() + "\">" + it.key() + ", " + it.value()["state"].get<std::string>() + "</option>\n";
    }

    html += R"HTML(        </select>
        
        <label>Destination City</label>
        <select id="dest">
)HTML";

    for (auto it = locations.begin(); it != locations.end(); ++it) {
        html += "            <option value=\"" + it.key() + "\">" + it.key() + ", " + it.value()["state"].get<std::string>() + "</option>\n";
    }

    html += R"HTML(        </select>
        
        <label>Weight (kg)</label>
        <input type="number" id="weight" value="10" min="0.1" step="0.1">
        
        <div class="checkbox-group">
            <label><input type="checkbox" id="express"> Express</label>
            <label><input type="checkbox" id="fragile"> Fragile</label>
        </div>
        
        <button onclick="calculateTariff()">Calculate via Gateway</button>

        <div id="result-box" class="shipment">
            <div class="cost" id="res-cost">$0.00</div>
            <div class="route" id="res-route">City ➔ City</div>
            <div style="font-size: 14px; color: #cbd5e1;" id="res-dist">Distance: 0 km</div>
        </div>
    </div>
    
    <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
    <script>
        const map = L.map('map', {zoomControl: false}).setView([39.8283, -98.5795], 4);
        L.tileLayer('https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', {
            attribution: '&copy; CARTO'
        }).addTo(map);
        
        let currentPath = null;
        let currentMarker = null;
        let animationInterval = null;

        const packageIcon = L.divIcon({
            className: 'package-icon',
            html: '<div class="package-dot"></div>',
            iconSize: [16, 16],
            iconAnchor: [8, 8]
        });

        async function calculateTariff() {
            const payload = {
                origin_city: document.getElementById('origin').value,
                destination_city: document.getElementById('dest').value,
                weight_kg: parseFloat(document.getElementById('weight').value),
                is_express: document.getElementById('express').checked,
                is_fragile: document.getElementById('fragile').checked
            };
            
            const apiUrl = window.location.protocol === 'file:' ? 'http://127.0.0.1:8080/api/calculate' : '/api/calculate';
            try {
                const response = await fetch(apiUrl, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(payload)
                });
                
                const data = await response.json();
                
                if (data.error) {
                    alert("Gateway Error: " + data.error);
                    return;
                }
                
                document.getElementById('result-box').style.display = 'block';
                document.getElementById('res-cost').innerText = '$' + data.cost.toFixed(2);
                document.getElementById('res-route').innerText = data.origin_city + ' ➔ ' + data.destination_city;
                document.getElementById('res-dist').innerText = 'Distance: ' + data.distance_km.toFixed(0) + ' km';
                
                animateRoute(data.origin_coord, data.dest_coord);
            } catch (err) {
                alert("Failed to connect to Frontend Gateway.");
            }
        }
        
        function animateRoute(origin, dest) {
            if (currentPath) map.removeLayer(currentPath);
            if (currentMarker) map.removeLayer(currentMarker);
            if (animationInterval) clearInterval(animationInterval);

            currentPath = L.polyline([origin, dest], {
                color: '#3b82f6',
                weight: 3,
                opacity: 0.5,
                dashArray: '10, 10',
                lineCap: 'round'
            }).addTo(map);

            map.fitBounds(currentPath.getBounds(), { padding: [100, 100] });

            currentMarker = L.marker(origin, { icon: packageIcon }).addTo(map);

            let progress = 0;
            const steps = 150;
            const intervalTime = 16;

            animationInterval = setInterval(() => {
                progress += 1;
                if (progress > steps) progress = 0;
                
                const t = progress / steps;
                const easeT = t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
                
                const lat = origin[0] + (dest[0] - origin[0]) * easeT;
                const lng = origin[1] + (dest[1] - origin[1]) * easeT;
                
                currentMarker.setLatLng([lat, lng]);
            }, intervalTime);
        }
    </script>
</body>
</html>)HTML";
    return html;
}

int main() {
    std::cout << "[FrontendGateway] Starting on port 8080...\n";
    
    // Wait for DataService to boot
    json locs;
    std::cout << "[FrontendGateway] Contacting DataService at 8081...\n";
    for (int i = 0; i < 5; ++i) {
        locs = fetchServiceJson("localhost", 8081, "/locations");
        if (!locs.empty()) break;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    if (locs.empty()) {
        std::cerr << "[FrontendGateway] WARNING: Could not connect to DataService. The UI will have empty locations.\n";
    } else {
        std::cout << "[FrontendGateway] Successfully synced locations from DataService.\n";
    }

    std::string index_html = generateHTML(locs);

    // Save to disk so the user can open it locally via file://
    std::ofstream out_html("index.html");
    out_html << index_html;
    out_html.close();
    std::cout << "[FrontendGateway] Exported local index.html\n";

    httplib::Server svr;

    svr.Get("/", [&index_html](const httplib::Request&, httplib::Response& res) {
        res.set_content(index_html, "text/html");
    });

    // Handle CORS Preflight for file:// access
    svr.Options("/api/calculate", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 200;
    });

    svr.Post("/api/calculate", [](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        try {
            auto j = json::parse(req.body);
            std::string origin_city = j["origin_city"];
            std::string dest_city = j["destination_city"];
            
            // 1. Fetch Location Coordinates from DataService
            json locs = fetchServiceJson("localhost", 8081, "/locations");
            if (!locs.contains(origin_city) || !locs.contains(dest_city)) {
                res.set_content(R"({"error": "City not found in DataService"})", "application/json");
                return;
            }
            auto origin = locs[origin_city];
            auto dest = locs[dest_city];
            
            // 2. Fetch Rates from DataService
            json rates = fetchServiceJson("localhost", 8081, "/rates");
            
            // 3. Prepare payload for CalculationService
            json calc_req;
            calc_req["weight_kg"] = j["weight_kg"];
            calc_req["origin_lat"] = origin["lat"];
            calc_req["origin_lon"] = origin["lon"];
            calc_req["dest_lat"] = dest["lat"];
            calc_req["dest_lon"] = dest["lon"];
            calc_req["is_express"] = j["is_express"];
            calc_req["is_fragile"] = j["is_fragile"];
            
            calc_req["base_rate"] = rates["base_rate_per_kg"];
            calc_req["express_multiplier"] = rates["express_multiplier"];
            calc_req["fragile_multiplier"] = rates["fragile_multiplier"];
            
            // Default region multipliers
            calc_req["origin_region_multiplier"] = 1.0;
            calc_req["dest_region_multiplier"] = 1.0;
            
            std::string o_reg = origin["region"];
            std::string d_reg = dest["region"];
            
            if (rates["regions"].contains(o_reg)) calc_req["origin_region_multiplier"] = rates["regions"][o_reg];
            if (rates["regions"].contains(d_reg)) calc_req["dest_region_multiplier"] = rates["regions"][d_reg];
            
            // 4. Send to CalculationService
            json calc_res = postServiceJson("localhost", 8082, "/compute", calc_req);
            
            if (calc_res.empty() || calc_res.contains("error")) {
                res.set_content(R"({"error": "CalculationService failed"})", "application/json");
                return;
            }
            
            // 5. Combine and return to browser
            json final_out;
            final_out["cost"] = calc_res["cost"];
            final_out["distance_km"] = calc_res["distance_km"];
            final_out["origin_city"] = origin_city;
            final_out["destination_city"] = dest_city;
            final_out["origin_coord"] = {origin["lat"], origin["lon"]};
            final_out["dest_coord"] = {dest["lat"], dest["lon"]};
            
            res.set_content(final_out.dump(), "application/json");
        } catch (std::exception& e) {
            res.set_content(R"({"error": "Gateway Processing Error"})", "application/json");
        }
    });

    int port = 8080;
    if (const char* env_p = std::getenv("PORT")) {
        port = std::stoi(env_p);
    }

    std::cout << "[FrontendGateway] Listening on all network interfaces (http://0.0.0.0:" << port << ")\n";
    std::cout << "[FrontendGateway] Other devices on your Wi-Fi can access this by visiting your computer's IP address (e.g., http://192.168.x.x:" << port << ")\n";
    svr.listen("0.0.0.0", port);
    return 0;
}
